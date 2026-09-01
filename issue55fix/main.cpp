#include <RE/Skyrim.h>
#include <SKSE/SKSE.h>

#include "IPluginInterface.h"

#include <unordered_map>

namespace
{
	constexpr auto kNode = "NPC";
	constexpr auto kTransform = "internal";

	INiTransformInterface* g_transform = nullptr;

	struct SavedTransform
	{
		bool female{ false };
		bool hasPosition{ false };
		bool hasRotation{ false };
		bool hasScale{ false };
		bool hasScaleMode{ false };
		INiTransformInterface::Position position{};
		INiTransformInterface::Rotation rotation{};
		float scale{ 1.0F };
		skee_u32 scaleMode{ 0 };
	};

	std::unordered_map<RE::FormID, SavedTransform> g_suspended;

	void Restore(RE::Actor* actor, SavedTransform& saved)
	{
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

	void SetFurnitureState(RE::FormID formID, bool entering)
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
			const bool entering = event->type == RE::TESFurnitureEvent::FurnitureEventType::kEnter;
			if (auto* tasks = SKSE::GetTaskInterface()) {
				tasks->AddTask([formID, entering]() { SetFurnitureState(formID, entering); });
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
