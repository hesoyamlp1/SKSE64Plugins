#include "CommandInterface.h"
#include "SKEETasks.h"
#include "FileUtils.h"
#include <cstdint>

extern CommandInterface g_commandInterface;

bool CommandInterface::RegisterCommand(const char* command, const char* desc, CommandCallback cb)
{
    std::scoped_lock<lock_type> locker(m_lock);
	return m_commandMap[command].emplace(CommandData{ desc, cb }).second;
}

bool CommandInterface::ExecuteCommand(const char* command, RE::TESObjectREFR* ref, const char* argumentString)
{
	std::scoped_lock<lock_type> locker(m_lock);
    auto it = m_commandMap.find(command);
    if (it != m_commandMap.end())
    {
        for (auto data : it->second)
        {
            if (data.cb(ref, argumentString))
                return true;
        }
    }
    return false;
}

#include "ItemDataInterface.h"
#include "RE/I/InventoryEntryData.h"
#include "RE/I/InventoryChanges.h"
#include "RE/E/ExtraContainerChanges.h"
#include "TintMaskInterface.h"
#include "BodyMorphInterface.h"
#include "OverlayInterface.h"
#include "OverrideInterface.h"
#include "ActorUpdateManager.h"
#include "NiTransformInterface.h"
#include "FaceMorphInterface.h"
#include "PresetInterface.h"

#include "AttachmentInterface.h"
#include "NifUtils.h"
#include "ShaderUtilities.h"


extern const SKSE::TaskInterface* g_task;
extern ItemDataInterface	g_itemDataInterface;
extern TintMaskInterface	g_tintMaskInterface;
extern BodyMorphInterface	g_bodyMorphInterface;
extern OverlayInterface		g_overlayInterface;
extern OverrideInterface	g_overrideInterface;
extern ActorUpdateManager	g_actorUpdateManager;
extern NiTransformInterface	g_transformInterface;
extern TintMaskInterface	g_tintMaskInterface;
extern FaceMorphInterface	g_morphInterface;
extern PresetInterface		g_presetInterface;

void CommandInterface::RegisterCommands()
{
    RegisterCommand("reload", "<tints>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
    {
        if (_strnicmp(argument, "tints", REX::W32::MAX_PATH) == 0)
        {
            g_tintMaskInterface.LoadMods();
            Console_Print("Tint XMLs reloaded");
            return true;
        }
        return false;
    });

	RegisterCommand("erase", "<bodymorph|transforms|sculpt|overlays|bodymorph-cache>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (_strnicmp(argument, "bodymorph", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Erasing BodyMorphs requires a console target");
				return true;
			}

			g_bodyMorphInterface.ClearMorphs(thisObj);
			g_bodyMorphInterface.UpdateModelWeight(thisObj);
			Console_Print("Erased all bodymorphs");
			return true;
		}
		else if (_strnicmp(argument, "transforms", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Erasing transforms requires a console target");
				return true;
			}

			g_transformInterface.Impl_RemoveAllReferenceTransforms(thisObj);
			g_transformInterface.Impl_UpdateNodeAllTransforms(thisObj);
			Console_Print("Erased all transforms");
			return true;
		}
		else if (_strnicmp(argument, "sculpt", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Erasing sculpt requires a console target");
				return true;
			}
			if (thisObj->IsNot(RE::FormType::ActorCharacter)) {
				Console_Print("Console target must be an actor");
				return true;
			}
			RE::Actor* actor = static_cast<RE::Actor*>(thisObj);
			RE::TESNPC* npc = thisObj->GetBaseObject() ? thisObj->GetBaseObject()->As<RE::TESNPC>() : nullptr;
			if (!npc) {
				Console_Print("Failed to acquire ActorBase for specified reference");
				return true;
			}

			g_morphInterface.EraseSculptData(npc);
			SKEE_AddTask(g_task, new SKSEUpdateFaceModel(actor));

			Console_Print("Erased all sculpting");
			return true;
		}
		else if (_strnicmp(argument, "overlays", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Erasing overlays requires a console target");
				return true;
			}
			if (thisObj->IsNot(RE::FormType::ActorCharacter)) {
				Console_Print("Console target must be an actor");
				return true;
			}
			RE::Actor* actor = static_cast<RE::Actor*>(thisObj);
			RE::TESNPC* npc = thisObj->GetBaseObject() ? thisObj->GetBaseObject()->As<RE::TESNPC>() : nullptr;
			if (!npc) {
				Console_Print("Failed to acquire ActorBase for specified reference");
				return true;
			}

			g_overlayInterface.EraseOverlays(actor);

			Console_Print("Erased and reverted all overlays");
			return true;
		}
		else if (_strnicmp(argument, "bodymorph-cache", REX::W32::MAX_PATH) == 0)
		{
			size_t freedMem = g_bodyMorphInterface.ClearMorphCache();
			Console_Print("Erased %I64u bytes from BodyMorph Cache", freedMem);
			return true;
		}
		return false;
	});
	RegisterCommand("preset-save", "<name>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", argument);
		char tintPath[REX::W32::MAX_PATH];
		sprintf_s(tintPath, "Data\\Textures\\CharGen\\Exported\\");

		RE::Actor* actor = static_cast<RE::Actor*>(thisObj);
		RE::TESNPC* npc = thisObj->GetBaseObject() ? thisObj->GetBaseObject()->As<RE::TESNPC>() : nullptr;
		if (!npc) {
			Console_Print("Failed to acquire ActorBase for specified reference");
			return true;
		}

		g_presetInterface.SaveJsonPreset(slotPath, actor);

		SKEE_AddTask(g_task, new SKSETaskExportTintMask(tintPath, argument));
		Console_Print("Preset saved");
		return true;
	});
	RegisterCommand("preset-load", "<name>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (!thisObj) {
			Console_Print("Applying a preset requires a console target");
			return true;
		}
		if (thisObj->IsNot(RE::FormType::ActorCharacter)) {
			Console_Print("Console target must be an actor");
			return true;
		}
		RE::Actor* actor = static_cast<RE::Actor*>(thisObj);
		RE::TESNPC* npc = thisObj->GetBaseObject() ? thisObj->GetBaseObject()->As<RE::TESNPC>() : nullptr;
		if (!npc) {
			Console_Print("Failed to acquire ActorBase for specified reference");
			return true;
		}

		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", argument);
		char tintPath[REX::W32::MAX_PATH];
		sprintf_s(tintPath, "Textures\\CharGen\\Exported\\%s.dds", argument);

		auto presetData = std::make_shared<PresetData>();
		bool loadError = g_presetInterface.LoadJsonPreset(slotPath, presetData);
		if (loadError) {
			Console_Print("Failed to load preset at %s", slotPath);
			return true;
		}

		presetData->tintTexture = tintPath;
		g_presetInterface.AssignMappedPreset(npc, presetData);
		g_presetInterface.ApplyPresetData(actor, presetData, true, PresetInterface::ApplyTypes::kPresetApplyAll);

		// Queue a node update
		actor->DoReset3D(true);
		Console_Print("Preset loaded");
		return true;
	});
#if _DEBUG
	RegisterCommand("attach", "<object>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (!thisObj) {
			Console_Print("Attaching mesh requires object");
			return true;
		}

		SKEE_AddTask(g_task, new SKSEAttachSkinnedMesh(static_cast<RE::Actor*>(thisObj), argument, "TestRoot", false, true, std::vector<RE::BSFixedString>()));
		return true;
	});
#endif
	RegisterCommand("diagnostics", "<bodymorph|transforms|strings|updates|overlays>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (_strnicmp(argument, "bodymorph", REX::W32::MAX_PATH) == 0)
		{
			g_bodyMorphInterface.PrintDiagnostics();
			return true;
		}
		else if (_strnicmp(argument, "transforms", REX::W32::MAX_PATH) == 0)
		{
			g_transformInterface.PrintDiagnostics();
			return true;
		}
		else if (_strnicmp(argument, "strings", REX::W32::MAX_PATH) == 0)
		{
			g_stringTable.PrintDiagnostics();
			return true;
		}
		else if (_strnicmp(argument, "overlays", REX::W32::MAX_PATH) == 0)
		{
			g_overlayInterface.PrintDiagnostics();
			return true;
		}
		else if (_strnicmp(argument, "updates", REX::W32::MAX_PATH) == 0)
		{
			g_actorUpdateManager.PrintDiagnostics();
			return true;
		}
		return false;
	});
	RegisterCommand("dump", "<bodymorph|morphnames|transforms|tints|overrides|overlays|itemdata|itembinding|skeleton_3p|skeleton_1p|equipped>", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (_strnicmp(argument, "bodymorph", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping transforms requires a console target");
				return true;
			}

			Console_Print("Dumping body morphs for %08X", thisObj->formID);

			class Visitor : public IBodyMorphInterface::MorphValueVisitor
			{
			public:
				Visitor() { }

				virtual void Visit(RE::TESObjectREFR* ref, const char* morphKey, const char* key, float value)
				{
					m_mapping[key][morphKey] = value;
				}
				std::map<SKEEFixedString, std::map<SKEEFixedString, float>> m_mapping;
			};
			Visitor visitor;
			g_bodyMorphInterface.VisitMorphValues(thisObj, visitor);

			std::uint32_t totalMorphs = 0;
			for (auto& key : visitor.m_mapping)
			{
				Console_Print("Key: %s", key.first.c_str());
				for (auto& morph : key.second)
				{
					Console_Print("\tMorph: %s\t\tValue: %f", morph.first.c_str(), morph.second);
				}
				Console_Print("Dumped %d morphs for key %s", key.second.size(), key.first.c_str());
				totalMorphs += key.second.size();
			}
			Console_Print("Dumped %d total morphs", totalMorphs);
			return true;
		}
		else if (_strnicmp(argument, "morphnames", REX::W32::MAX_PATH) == 0)
		{
			Console_Print("Dumping morph names");
			auto morphNames = g_bodyMorphInterface.GetCachedMorphNames();
			for (auto& name : morphNames)
			{
				Console_Print("\t%s", name.c_str());
			}
			Console_Print("%d total morphs", morphNames.size());
			return true;
		}
		else if (_strnicmp(argument, "transforms", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping transforms requires a console target");
				return true;
			}
			if (thisObj->IsNot(RE::FormType::ActorCharacter)) {
				Console_Print("Console target must be an actor");
				return true;
			}
			RE::Actor* actor = static_cast<RE::Actor*>(thisObj);
			RE::TESNPC* npc = thisObj->GetBaseObject() ? thisObj->GetBaseObject()->As<RE::TESNPC>() : nullptr;
			if (!npc) {
				Console_Print("Failed to acquire ActorBase for specified reference");
				return true;
			}

			Console_Print("Dumping transforms for %08X", thisObj->formID);

			std::uint32_t totalTransforms = 0;
			g_transformInterface.Impl_VisitNodes(thisObj, false, npc->GetSex() == 1, [&](const SKEEFixedString& node, OverrideRegistration<StringTableItem>* keys)
			{
				Console_Print("Node: %s", node.c_str());
				for (auto& item : *keys)
				{
					Console_Print("\tKey: %s\t\tProperties %d", item.first ? item.first->c_str() : "", item.second.size());
					totalTransforms++;
				}
				return true;
			});
			Console_Print("Dumped %d total transforms", totalTransforms);
			return true;
		}
		else if (_strnicmp(argument, "tints", REX::W32::MAX_PATH) == 0)
		{
			if (thisObj && thisObj->IsNot(RE::FormType::ActorCharacter)) {
				Console_Print("Console target must be an actor");
				return true;
			}

			std::uint32_t mask = 1;
			for (std::uint32_t i = 0; i < 32; ++i)
			{
				IItemDataInterface::Identifier identifier;
				identifier.SetSlotMask(mask);
				SKEE_AddTask(g_task, new NIOVTaskUpdateItemDye(thisObj ? thisObj->As<RE::Actor>() : RE::PlayerCharacter::GetSingleton(), identifier, TintMaskInterface::kUpdate_All, true, [mask](RE::TESObjectARMO* armo, RE::TESObjectARMA* arma, const char* path, RE::NiTexturePtr texture, LayerTarget& layer)
				{
					char texturePath[REX::W32::MAX_PATH];
					_snprintf_s(texturePath, REX::W32::MAX_PATH, "Data\\SKSE\\Plugins\\NiOverride\\Exported\\TintMasks\\%s", path);

					FileUtils::MakeAllDirs(texturePath);

					SaveRenderedDDS(texture.get(), texturePath);

					Console_Print("Dumped result for slot %08X at %s on shape", mask, texturePath, layer.object->name.c_str());
				}));
				mask <<= 1;
			}
			return true;
		}
		else if (_strnicmp(argument, "overrides", REX::W32::MAX_PATH) == 0)
		{
			Console_Print("Dumping node overrides...");
			g_overrideInterface.Dump();
			Console_Print("Dump complete. See log file for details.");
			return true;
		}
		else if (_strnicmp(argument, "overlays", REX::W32::MAX_PATH) == 0)
		{
			Console_Print("Dumping overlays...");
			g_overlayInterface.Visit([](std::uint32_t formId) {
				RE::TESForm* form = RE::TESForm::LookupByID(formId);
				RE::TESObjectREFR* refr = form ? form->As<RE::TESObjectREFR>() : nullptr;
				SKSE::log::info("Reference: {:08X} ({}) with Overlays", formId, refr ? refr->GetName() : "");
			});
			Console_Print("Dump complete. See log file for details.");
			return true;
		}
		else if (_strnicmp(argument, "itemdata", REX::W32::MAX_PATH) == 0)
		{
			g_itemDataInterface.ForEachItemAttribute([](const ItemAttribute& item)
			{
				SKSE::log::info("Item UID: {} ID: {} Owner: {:08X} Form: {:08X}", item.uid, item.rank, item.ownerForm, item.formId);
				item.data->ForEachLayer([&](std::int32_t layerIndex, auto tintData) -> bool
				{
					SKSE::log::info("Tint Index: {}", layerIndex);
					for (auto& color : tintData.m_colorMap)
					{
						SKSE::log::info("ColorIndex: {} Color: {:08X}", color.first, color.second);
					}
					for (auto& blend : tintData.m_blendMap)
					{
						SKSE::log::info("BlendIndex: {} Blend: {}", blend.first, blend.second->c_str());
					}
					for (auto& texture : tintData.m_textureMap)
					{
						SKSE::log::info("TextureIndex: {} Texture: {}", texture.first, texture.second->c_str());
					}
					for (auto& type : tintData.m_typeMap)
					{
						SKSE::log::info("TypeIndex: {} Type: {}", type.first, type.second);
					}
					return false;
				});
				Console_Print("Item UID: %d ID: %d Owner: %08X Form: %08X", item.uid, item.rank, item.ownerForm, item.formId);
			});
			return true;
		}
		else if (_strnicmp(argument, "itembinding", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping itembinding requires a console target");
				return true;
			}

			class RankItemFinder : public RE::InventoryChanges::IItemChangeVisitor
			{
			public:
				RE::BSContainer::ForEachResult Visit(RE::InventoryEntryData* pEntryData) override
				{
					if (!pEntryData)
						return RE::BSContainer::ForEachResult::kContinue;

					auto* pExtendList = pEntryData->extraLists;
					if (!pExtendList)
						return RE::BSContainer::ForEachResult::kContinue;

					for (auto* pExtraDataList : *pExtendList)
					{
						if (pExtraDataList && pExtraDataList->HasType(RE::ExtraDataType::kRank))
						{
							RE::ExtraRank* extraRank = pExtraDataList->GetByType<RE::ExtraRank>();
							Console_Print("\tItem ID: %d Form: %08X", extraRank->rank, pEntryData->object ? pEntryData->object->formID : 0);
							auto itemData = g_itemDataInterface.GetData(extraRank->rank);
							if (itemData)
							{
								itemData->ForEachLayer([&](std::int32_t layerIndex, auto tintData) -> bool
								{
									Console_Print("\t\tTint Index: %d", layerIndex);
									for (auto& color : tintData.m_colorMap)
									{
										Console_Print("\t\t\tColorIndex: %d Color: %08X", color.first, color.second);
									}
									for (auto& blend : tintData.m_blendMap)
									{
										Console_Print("\t\t\tBlendIndex: %d Blend: %s", blend.first, blend.second->c_str());
									}
									for (auto& texture : tintData.m_textureMap)
									{
										Console_Print("\t\t\tTextureIndex: %d Texture: %s", texture.first, texture.second->c_str());
									}
									for (auto& type : tintData.m_typeMap)
									{
										Console_Print("\t\t\tTypeIndex: %d Type: %d", type.first, type.second);
									}
									return false;
								});
							}
							foundItems++;
						}
					}
					return RE::BSContainer::ForEachResult::kContinue;
				}

				std::uint32_t foundItems = 0;
			};

			Console_Print("Finding items with extended data inside %08X", thisObj->formID);

			RE::ExtraContainerChanges* pContainerChanges = thisObj->extraList.GetByType<RE::ExtraContainerChanges>();
			if (pContainerChanges) {
				RankItemFinder itemFinder;
				auto* data = pContainerChanges->changes;
				if (data) {
					data->VisitInventory(itemFinder);

					Console_Print("Found %d items with extended data inside %08X", itemFinder.foundItems, thisObj->formID);
				}
			}
			return true;
		}
		else if (_strnicmp(argument, "skeleton_3p", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping nodes requires a reference");
				return true;
			}

			Console_Print("Dumping reference third person skeleton...");
			DumpNodeChildren(thisObj->Get3D(false));
			Console_Print("Dumped reference. See log for more details.");
			return true;
		}
		else if (_strnicmp(argument, "skeleton_1p", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping nodes requires a reference");
				return true;
			}

			Console_Print("Dumping reference first person skeleton...");
			DumpNodeChildren(thisObj->Get3D(true));
			Console_Print("Dumped reference. See log for more details.");
			return true;
		}
		else if (_strnicmp(argument, "equipped", REX::W32::MAX_PATH) == 0)
		{
			if (!thisObj) {
				Console_Print("Dumping biped nodes requires a reference");
				return false;
			}
			for (int k = 0; k <= 1; ++k)
			{
				auto weightModel = thisObj->GetBiped(k == 1);
				SKSE::log::info("Biped Set {}", k);
				if (weightModel)
				{
					for (int i = 0; i < 42; ++i)
					{
						SKSE::log::info("Biped 1 Slot: {} Armor: {:08X} Arma: {:08X}", i, weightModel->objects[i].item ? weightModel->objects[i].item->formID : 0, weightModel->objects[i].addon ? weightModel->objects[i].addon->formID : 0);
						RE::TESForm* armor = weightModel->objects[i].item;
						RE::NiAVObject* node = weightModel->objects[i].partClone.get();
						if (armor && armor->IsArmor())
						{
							SKSE::log::info("Armor: {} Shape: {} [{:#x}]", armor->As<RE::TESObjectARMO>()->GetFullName(), node ? node->name.c_str() : "", (std::uint64_t)(uintptr_t)node);
						}
					}
					for (int i = 0; i < 42; ++i)
					{
						SKSE::log::info("Biped 2 Slot: {} Armor: {:08X} Arma: {:08X}", i, weightModel->bufferedObjects[i].item ? weightModel->bufferedObjects[i].item->formID : 0, weightModel->bufferedObjects[i].addon ? weightModel->bufferedObjects[i].addon->formID : 0);
						RE::TESForm* armor = weightModel->bufferedObjects[i].item;
						RE::NiAVObject* node = weightModel->bufferedObjects[i].partClone.get();
						if (armor && armor->IsArmor())
						{
							SKSE::log::info("Armor: {} Shape: {} [{:#x}]", armor->As<RE::TESObjectARMO>()->GetFullName(), node ? node->name.c_str() : "", (std::uint64_t)(uintptr_t)node);
						}
					}
				}
			}
			return true;
		}
		return false;
	});
	RegisterCommand("help", "Displays all the registered commands and their description", [](RE::TESObjectREFR* thisObj, const char* argument) -> bool
	{
		if (argument == nullptr || argument[0] == 0)
		{
			std::scoped_lock locker(g_commandInterface.m_lock);
			for (auto& cmdItem : g_commandInterface.m_commandMap)
			{
				if (_stricmp(cmdItem.first.c_str(), "help") == 0)
				{
					continue;
				}

				for (auto& cmd : cmdItem.second)
				{
					Console_Print("\t%s %s", cmdItem.first.c_str(), cmd.desc.c_str());
				}
			}
			return true;
		}

		return false;
	});
}