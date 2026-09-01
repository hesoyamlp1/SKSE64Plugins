#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "IPluginInterface.h"

#include <unordered_map>

namespace
{
	constexpr auto kNode = "NPC";
	constexpr auto kTransform = "internal";
	constexpr auto kChairBiasTransform = "SKEEFurnitureFix";
	constexpr float kChairRootBias = -4.0F;

	INiTransformInterface* g_transform = nullptr;
	bool g_heelsFixLoaded = false;

	struct SavedTransform
	{
		bool female{ false };
		bool hasPosition{ false };
		bool hasRotation{ false };
		bool hasScale{ false };
		bool hasScaleMode{ false };
		bool heelsFixChairBias{ false };
		INiTransformInterface::Position position{};
		INiTransformInterface::Rotation rotation{};
		float scale{ 1.0F };
		skee_u32 scaleMode{ 0 };
	};

	std::unordered_map<RE::FormID, SavedTransform> g_suspended;

	void Restore(RE::Actor* actor, SavedTransform& saved)
	{
		if (saved.heelsFixChairBias) {
			g_transform->RemoveNodeTransformPosition(
				actor, false, saved.female, kNode, kChairBiasTransform);
			g_transform->UpdateNodeTransforms(actor, false, saved.female, kNode);
			return;
		}

		if (saved.hasPosition) {
			g_transform->AddNodeTransformPosition(actor, false, saved.female, kNode, kTransform, saved.position);
		}
		if (saved.hasRotation) {
			g_transform->AddNodeTransformRotation(actor, false, saved.female, kNode, kTransform, saved.rotation);
		}
		if (saved.hasScale) {
			g_transform->AddNodeTransformScale(actor, false, saved.female, kNode, kTransform, saved.scale);
		}
		if (saved.hasScaleMode) {
			g_transform->AddNodeTransformScaleMode(actor, false, saved.female, kNode, kTransform, saved.scaleMode);
		}
		g_transform->UpdateNodeTransforms(actor, false, saved.female, kNode);
	}

	void RestoreAll()
	{
		if (!g_transform) {
			g_suspended.clear();
			return;
		}

		for (auto& [formID, saved] : g_suspended) {
			if (auto* actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
				Restore(actor, saved);
			}
		}
		g_suspended.clear();
	}

	bool IsChairLikeFurniture(RE::FormID furnitureFormID)
	{
		auto* reference = RE::TESForm::LookupByID<RE::TESObjectREFR>(furnitureFormID);
		auto* furniture = reference && reference->GetBaseObject() ?
			reference->GetBaseObject()->As<RE::TESFurniture>() : nullptr;
		return furniture &&
			furniture->workBenchData.benchType == RE::TESFurniture::WorkBenchData::BenchType::kNone &&
			furniture->furnFlags.any(RE::TESFurniture::ActiveMarker::kCanSit) &&
			furniture->furnFlags.none(RE::TESFurniture::ActiveMarker::kCanSleep);
	}

	void SetFurnitureState(RE::FormID formID, RE::FormID furnitureFormID, bool entering)
	{
		if (!g_transform) {
			return;
		}

		auto* actor = RE::TESForm::LookupByID<RE::Actor>(formID);
		if (!actor) {
			return;
		}

		if (!entering) {
			auto it = g_suspended.find(formID);
			if (it != g_suspended.end()) {
				Restore(actor, it->second);
				g_suspended.erase(it);
			}
			return;
		}

		if (g_suspended.contains(formID)) {
			return;
		}

		auto* base = actor->GetActorBase();
		const bool female = base && base->GetSex() == RE::SEXES::kFemale;
		SavedTransform saved;
		saved.female = female;

		// HeelsFix already preserves the shoes and corrects the leg pose.  Its
		// ordinary-chair path still leaves this fixture a few units above the
		// cushion, so add only the missing root bias and leave "internal" intact.
		if (g_heelsFixLoaded && IsChairLikeFurniture(furnitureFormID) &&
			g_transform->HasNodeTransformPosition(actor, false, female, kNode, kTransform)) {
			INiTransformInterface::Position bias{ 0.0F, 0.0F, kChairRootBias };
			g_transform->AddNodeTransformPosition(
				actor, false, female, kNode, kChairBiasTransform, bias);
			g_transform->UpdateNodeTransforms(actor, false, female, kNode);
			saved.heelsFixChairBias = true;
			g_suspended.emplace(formID, saved);
			return;
		}

		saved.hasPosition = g_transform->HasNodeTransformPosition(actor, false, female, kNode, kTransform);
		saved.hasRotation = g_transform->HasNodeTransformRotation(actor, false, female, kNode, kTransform);
		saved.hasScale = g_transform->HasNodeTransformScale(actor, false, female, kNode, kTransform);
		saved.hasScaleMode = g_transform->HasNodeTransformScaleMode(actor, false, female, kNode, kTransform);

		if (!saved.hasPosition && !saved.hasRotation && !saved.hasScale && !saved.hasScaleMode) {
			return;
		}

		if (saved.hasPosition) {
			saved.position = g_transform->GetNodeTransformPosition(actor, false, female, kNode, kTransform);
		}
		if (saved.hasRotation) {
			saved.rotation = g_transform->GetNodeTransformRotation(actor, false, female, kNode, kTransform);
		}
		if (saved.hasScale) {
			saved.scale = g_transform->GetNodeTransformScale(actor, false, female, kNode, kTransform);
		}
		if (saved.hasScaleMode) {
			saved.scaleMode = g_transform->GetNodeTransformScaleMode(actor, false, female, kNode, kTransform);
		}

		if (g_transform->RemoveNodeTransform(actor, false, female, kNode, kTransform)) {
			g_transform->UpdateNodeTransforms(actor, false, female, kNode);
			g_suspended.emplace(formID, saved);
		}
	}

	class FurnitureSink final : public RE::BSTEventSink<RE::TESFurnitureEvent>
	{
	public:
		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESFurnitureEvent* event,
			RE::BSTEventSource<RE::TESFurnitureEvent>*) override
		{
			if (!event || !event->actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			const auto formID = event->actor->GetFormID();
			const auto furnitureFormID = event->targetFurniture ? event->targetFurniture->GetFormID() : 0;
			const bool entering = event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter;
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([formID, furnitureFormID, entering]() {
					SetFurnitureState(formID, furnitureFormID, entering);
				});
			}
			return RE::BSEventNotifyControl::kContinue;
		}
	};

	FurnitureSink g_furnitureSink;

	bool AcquireTransformInterface()
	{
		InterfaceExchangeMessage exchange{};
		auto* messaging = SKSE::GetMessagingInterface();
		if (!messaging || !messaging->Dispatch(
			InterfaceExchangeMessage::kMessage_ExchangeInterface,
			&exchange,
			sizeof(exchange),
			"skee") ||
			!exchange.interfaceMap) {
			return false;
		}

		g_transform = static_cast<INiTransformInterface*>(exchange.interfaceMap->QueryInterface("NiTransform"));
		return g_transform && g_transform->GetVersion() >= INiTransformInterface::kPluginVersion3;
	}

	void OnSKSEMessage(SKSE::MessagingInterface::Message* message)
	{
		switch (message->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			if (auto* data = RE::TESDataHandler::GetSingleton()) {
				g_heelsFixLoaded = data->LookupForm(0x000807, "HeelsFix.esp") != nullptr;
			}
			if (AcquireTransformInterface()) {
				if (auto* holder = RE::ScriptEventSourceHolder::GetSingleton()) {
					holder->AddEventSink(&g_furnitureSink);
				}
			}
			break;
		case SKSE::MessagingInterface::kPreLoadGame:
			RestoreAll();
			break;
		default:
			break;
		}
	}
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	if (auto* messaging = SKSE::GetMessagingInterface()) {
		return messaging->RegisterListener("SKSE", OnSKSEMessage);
	}
	return false;
}
