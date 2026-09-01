#include "PapyrusCharGen.h"
#include "SKEETasks.h"
#include <filesystem>
#include "FaceMorphInterface.h"
#include "NifUtils.h"
#include "Win32ErrorCodes.h"

#include "OverrideVariant.h"
#include "OverrideInterface.h"
#include "NiTransformInterface.h"
#include "BodyMorphInterface.h"
#include "OverlayInterface.h"
#include "PresetInterface.h"

#include "FileUtils.h"
#include <cstdint>


extern FaceMorphInterface g_morphInterface;
extern bool			g_externalHeads;
extern bool			g_enableHeadExport;

extern const SKSE::TaskInterface* g_task;
extern OverrideInterface				g_overrideInterface;
extern NiTransformInterface				g_transformInterface;
extern BodyMorphInterface				g_bodyMorphInterface;
extern OverlayInterface					g_overlayInterface;
extern PresetInterface					g_presetInterface;

namespace papyrusCharGen
{
	void SaveCharacter(RE::StaticFunctionTag*, RE::BSFixedString fileName)
	{
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());
		char tintPath[REX::W32::MAX_PATH];
		sprintf_s(tintPath, "Data\\Textures\\CharGen\\Exported\\");

		g_presetInterface.SaveJsonPreset(slotPath, RE::PlayerCharacter::GetSingleton());

		if(g_enableHeadExport)
			SKEE_AddTask(g_task, new SKSETaskExportTintMask(tintPath, fileName.c_str()));
	}

	void DeleteCharacter(RE::StaticFunctionTag*, RE::BSFixedString fileName)
	{
		char tempPath[REX::W32::MAX_PATH];
		sprintf_s(tempPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.slot", fileName.c_str());
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
				case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			}
		}
		sprintf_s(tempPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
			case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			}
		}
		sprintf_s(tempPath, "Data\\Textures\\CharGen\\Exported\\%s.dds", fileName.c_str());
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
			case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			}
		}
		sprintf_s(tempPath, "Data\\Meshes\\CharGen\\Exported\\%s.nif", fileName.c_str());
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
			case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			}
		}
	}
	
	std::int32_t DeleteFaceGenData(RE::StaticFunctionTag*, RE::TESNPC * npc)
	{
		std::int32_t ret = 0;
		if (!npc) {
			SKSE::log::error("{} - invalid actorbase.", __FUNCTION__);
			return -1;
		}

		const RE::TESFile* modInfo = GetModInfoByFormID(npc->formID);
		if (!modInfo) {
			SKSE::log::error("{} - failed to find mod for {:08X}.", __FUNCTION__, npc->formID);
			return false;
		}

		enum
		{
			kReturnDeletedNif = 1,
			kReturnDeletedDDS = 2
		};
		
		char tempPath[REX::W32::MAX_PATH];
		sprintf_s(tempPath, "Data\\Meshes\\Actors\\Character\\FaceGenData\\FaceGeom\\%s\\%08X.nif", modInfo->GetFilename().data(), modInfo->IsLight() ? (npc->formID & 0xFFF) : (npc->formID & 0xFFFFFF));
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
			case skee::W32::ERROR_FILE_NOT_FOUND: // We don't need to display a message for this
				break;
			case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			default:
				SKSE::log::error("{} - error deleting file {} (Error {})", __FUNCTION__, tempPath, lastError);
				break;
			}
		}
		else
			ret |= kReturnDeletedNif;

		sprintf_s(tempPath, "Data\\Textures\\Actors\\Character\\FaceGenData\\FaceTint\\%s\\%08X.dds", modInfo->GetFilename().data(), modInfo->IsLight() ? (npc->formID & 0xFFF) : (npc->formID & 0xFFFFFF));
		if (!std::filesystem::remove(tempPath)) {
			std::uint32_t lastError = REX::W32::GetLastError();
			switch (lastError) {
			case skee::W32::ERROR_FILE_NOT_FOUND: // We don't need to display a message for this
				break;
			case skee::W32::ERROR_ACCESS_DENIED:
				SKSE::log::error("{} - access denied could not delete {}", __FUNCTION__, tempPath);
				break;
			default:
				SKSE::log::error("{} - error deleting file {} (Error {})", __FUNCTION__, tempPath, lastError);
				break;
			}
		}
		else
			ret |= kReturnDeletedDDS;

		return ret;
	}
	
	bool LoadCharacterEx(RE::StaticFunctionTag*, RE::Actor * actor, RE::TESRace * race, RE::BSFixedString fileName, std::uint32_t flags)
	{
		if (!actor) {
			SKSE::log::error("{} - No actor found.", __FUNCTION__);
			return false;
		}
		if (!race) {
			SKSE::log::error("{} - No race found.", __FUNCTION__);
			return false;
		}
		RE::TESNPC * npc = actor->GetActorBase() ? actor->GetActorBase()->As<RE::TESNPC>() : nullptr;
		if (!npc) {
			SKSE::log::error("{} - failed acquire ActorBase.", __FUNCTION__);
			return false;
		}
		
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());
		char tintPath[REX::W32::MAX_PATH];
		sprintf_s(tintPath, "Textures\\CharGen\\Exported\\%s.dds", fileName.c_str());

		auto presetData = std::make_shared<PresetData>();
		bool loadError = g_presetInterface.LoadJsonPreset(slotPath, presetData);
		if (loadError) {
			sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Exported\\%s.slot", fileName.c_str());
			loadError = g_presetInterface.LoadBinaryPreset(slotPath, presetData);
		}

		if (loadError) {
			SKSE::log::error("{} - failed to load preset.", __FUNCTION__);
			return false;
		}

		presetData->tintTexture = tintPath;
		g_presetInterface.AssignMappedPreset(npc, presetData);

		g_presetInterface.ApplyPreset(actor, race, npc, presetData, (PresetInterface::ApplyTypes)flags);
		return true;
	}

	void SaveExternalCharacter(RE::StaticFunctionTag*, RE::BSFixedString fileName)
	{
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());
		char nifPath[REX::W32::MAX_PATH];
		sprintf_s(nifPath, "Data\\Meshes\\CharGen\\Exported\\%s.nif", fileName.c_str());
		char tintPath[REX::W32::MAX_PATH];
		sprintf_s(tintPath, "Data\\Textures\\CharGen\\Exported\\%s.dds", fileName.c_str());

		g_presetInterface.SaveJsonPreset(slotPath, RE::PlayerCharacter::GetSingleton());

		if(g_enableHeadExport)
			SKEE_AddTask(g_task, new SKSETaskExportHead(RE::PlayerCharacter::GetSingleton(), nifPath, tintPath));
	}

	bool LoadExternalCharacterEx(RE::StaticFunctionTag*, RE::Actor * actor, RE::TESRace * race, RE::BSFixedString fileName, std::uint32_t flags)
	{
		if (!actor) {
			SKSE::log::error("{} - No actor found.", __FUNCTION__);
			return false;
		}
		if (!race) {
			SKSE::log::error("{} - No race found.", __FUNCTION__);
			return false;
		}
		RE::TESNPC * npc = actor->GetActorBase() ? actor->GetActorBase()->As<RE::TESNPC>() : nullptr;
		if (!npc) {
			SKSE::log::error("{} - failed acquire ActorBase.", __FUNCTION__);
			return false;
		}

		g_presetInterface.EraseMappedPreset(npc);

		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());

		auto presetData = std::make_shared<PresetData>();
		bool loadError = g_presetInterface.LoadJsonPreset(slotPath, presetData);
		if (loadError) {
			sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Exported\\%s.slot", fileName.c_str());
			loadError = g_presetInterface.LoadBinaryPreset(slotPath, presetData);
		}

		if (loadError) {
			SKSE::log::error("{} - failed to load preset.", __FUNCTION__);
			return false;
		}

		const RE::TESFile* modInfo = GetModInfoByFormID(npc->formID);
		if (!modInfo) {
			SKSE::log::error("{} - failed to find mod for {:08X}.", __FUNCTION__, npc->formID);
			return false;
		}

		char sourcePath[REX::W32::MAX_PATH];
		char destPath[REX::W32::MAX_PATH];
		sprintf_s(sourcePath, "Data\\Meshes\\CharGen\\Exported\\%s.nif", fileName.c_str());
		sprintf_s(destPath, "Data\\Meshes\\Actors\\Character\\FaceGenData\\FaceGeom\\%s\\%08X.nif", modInfo->GetFilename().data(), modInfo->IsLight() ? (npc->formID & 0xFFF) : (npc->formID & 0xFFFFFF));
		std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);
		sprintf_s(sourcePath, "Data\\Textures\\CharGen\\Exported\\%s.dds", fileName.c_str());
		sprintf_s(destPath, "Data\\Textures\\Actors\\Character\\FaceGenData\\FaceTint\\%s\\%08X.dds", modInfo->GetFilename().data(), modInfo->IsLight() ? (npc->formID & 0xFFF) : (npc->formID & 0xFFFFFF));
		std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing);

		g_presetInterface.ApplyPreset(actor, race, npc, presetData, (PresetInterface::ApplyTypes)flags);
		SKEE_AddTask(g_task, new SKSETaskRefreshTintMask(actor, sourcePath));
		return true;
	}

	bool LoadCharacterPresetEx(RE::StaticFunctionTag*, RE::Actor * actor, RE::BSFixedString fileName, RE::BGSColorForm* hairColor, std::uint32_t flags)
	{
		if (!actor) {
			SKSE::log::error("{} - No actor found.", __FUNCTION__);
			return false;
		}
		RE::TESNPC * npc = actor->GetActorBase() ? actor->GetActorBase()->As<RE::TESNPC>() : nullptr;
		if (!npc) {
			SKSE::log::error("{} - failed acquire ActorBase.", __FUNCTION__);
			return false;
		}

		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Presets\\%s.jslot", fileName.c_str());

		auto presetData = std::make_shared<PresetData>();
		bool loadError = g_presetInterface.LoadJsonPreset(slotPath, presetData);
		if (loadError) {
			sprintf_s(slotPath, "SKSE\\Plugins\\CharGen\\Presets\\%s.slot", fileName.c_str());
			loadError = g_presetInterface.LoadBinaryPreset(slotPath, presetData);
		}

		if (loadError) {
			SKSE::log::error("{} - failed to load preset.", __FUNCTION__);
			return false;
		}

		if (hairColor)
		{
			hairColor->color.red = (presetData->hairColor >> 16) & 0xFF;
			hairColor->color.green = (presetData->hairColor >> 8) & 0xFF;
			hairColor->color.blue = presetData->hairColor & 0xFF;

			npc->SetHairColor(hairColor);
		}

		g_presetInterface.ApplyPresetData(actor, presetData, true, (PresetInterface::ApplyTypes)flags);

		// Queue a node update
		actor->DoReset3D(true);
		return true;
	}

	void SaveCharacterPreset(RE::StaticFunctionTag*, RE::Actor * actor, RE::BSFixedString fileName)
	{
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "Data\\SKSE\\Plugins\\CharGen\\Presets\\%s.jslot", fileName.c_str());
		g_presetInterface.SaveJsonPreset(slotPath, actor);
	}

	bool IsExternalEnabled(RE::StaticFunctionTag*)
	{
		return g_externalHeads;
	}

	bool ClearPreset(RE::StaticFunctionTag*, RE::TESNPC * npc)
	{
		if (!npc) {
			SKSE::log::error("{} - failed acquire ActorBase.", __FUNCTION__);
			return false;
		}

		return g_presetInterface.EraseMappedPreset(npc);
	}

	void ClearPresets(RE::StaticFunctionTag*)
	{
		g_presetInterface.ClearMappedPresets();
	}

	void ExportHead(RE::StaticFunctionTag*, RE::BSFixedString fileName)
	{
		if (g_enableHeadExport)
		{
			char nifPath[REX::W32::MAX_PATH];
			sprintf_s(nifPath, "Data\\SKSE\\Plugins\\CharGen\\%s.nif", fileName.c_str());
			char tintPath[REX::W32::MAX_PATH];
			sprintf_s(tintPath, "Data\\SKSE\\Plugins\\CharGen\\%s.dds", fileName.c_str());

			SKEE_AddTask(g_task, new SKSETaskExportHead(RE::PlayerCharacter::GetSingleton(), nifPath, tintPath));
		}
	}

	void ExportSlot(RE::StaticFunctionTag*, RE::BSFixedString fileName)
	{
		char slotPath[REX::W32::MAX_PATH];
		sprintf_s(slotPath, "Data\\SKSE\\Plugins\\CharGen\\Exported\\%s.jslot", fileName.c_str());
		g_presetInterface.SaveJsonPreset(slotPath, RE::PlayerCharacter::GetSingleton());
	}
};

void papyrusCharGen::RegisterFuncs(RE::BSScript::IVirtualMachine* a_vm)
{
	a_vm->RegisterFunction("SaveCharacter", "CharGen", papyrusCharGen::SaveCharacter);

	a_vm->RegisterFunction("LoadCharacterEx", "CharGen", papyrusCharGen::LoadCharacterEx);

	a_vm->RegisterFunction("DeleteCharacter", "CharGen", papyrusCharGen::DeleteCharacter);

	a_vm->RegisterFunction("DeleteFaceGenData", "CharGen", papyrusCharGen::DeleteFaceGenData);

	a_vm->RegisterFunction("SaveExternalCharacter", "CharGen", papyrusCharGen::SaveExternalCharacter);

	a_vm->RegisterFunction("LoadExternalCharacterEx", "CharGen", papyrusCharGen::LoadExternalCharacterEx);

	a_vm->RegisterFunction("ClearPreset", "CharGen", papyrusCharGen::ClearPreset);

	a_vm->RegisterFunction("ClearPresets", "CharGen", papyrusCharGen::ClearPresets);

	a_vm->RegisterFunction("IsExternalEnabled", "CharGen", papyrusCharGen::IsExternalEnabled);

	a_vm->RegisterFunction("ExportHead", "CharGen", papyrusCharGen::ExportHead);

	a_vm->RegisterFunction("ExportSlot", "CharGen", papyrusCharGen::ExportSlot);

	a_vm->RegisterFunction("LoadCharacterPresetEx", "CharGen", papyrusCharGen::LoadCharacterPresetEx);

	a_vm->RegisterFunction("SaveCharacterPreset", "CharGen", papyrusCharGen::SaveCharacterPreset);
}
