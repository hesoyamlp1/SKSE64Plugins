#pragma once

#include "RE/B/BSFixedString.h"
#include "RE/I/IVirtualMachine.h"
#include <cstdint>
namespace RE
{
	class TESObjectREFR;
	class TESObjectARMO;
	class TESObjectARMA;
	class TESObjectWEAP;
}
namespace papyrusNiOverride
{
	void RegisterFuncs(RE::BSScript::IVirtualMachine* a_vm);

	// Armor Overrides
	bool HasOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	void AddOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index, T dataType, bool persist);

	template<typename T>
	T GetOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	T GetArmorAddonProperty(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	// Node Overrides
	bool HasNodeOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	void AddNodeOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index, T dataType, bool persist);

	template<typename T>
	T GetNodeOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	T GetNodeProperty(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool firstPerson, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	// Weapon overrides
	bool HasWeaponOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	void AddWeaponOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index, T dataType, bool persist);

	template<typename T>
	T GetWeaponOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	template<typename T>
	T GetWeaponProperty(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	void RemoveAllOverrides(RE::StaticFunctionTag* base);
	void RemoveAllReferenceOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RemoveAllArmorOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor);
	void RemoveAllArmorAddonOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon);
	void RemoveAllArmorAddonNodeOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName);
	void RemoveArmorAddonOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	void RemoveAllNodeOverrides(RE::StaticFunctionTag* base);
	void RemoveAllReferenceNodeOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RemoveAllNodeNameOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName);
	void RemoveNodeOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	void RemoveAllWeaponBasedOverrides(RE::StaticFunctionTag* base);
	void RemoveAllReferenceWeaponOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RemoveAllWeaponOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon);
	void RemoveAllWeaponNodeOverrides(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName);
	void RemoveWeaponOverride(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint32_t key, std::uint32_t index);

	std::uint32_t GetNumBodyOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumHandOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumFeetOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumFaceOverlays(RE::StaticFunctionTag* base);

	std::uint32_t GetNumSpellBodyOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumSpellHandOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumSpellFeetOverlays(RE::StaticFunctionTag* base);
	std::uint32_t GetNumSpellFaceOverlays(RE::StaticFunctionTag* base);

	void AddOverlays(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	bool HasOverlays(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RemoveOverlays(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RevertOverlays(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr);
	void RevertOverlay(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, RE::BSFixedString nodeName, std::uint32_t armorMask, std::uint32_t addonMask);

	void SetBodyMorph(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, RE::BSFixedString morphName, RE::BSFixedString keyName, float value);
	float GetBodyMorph(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, RE::BSFixedString morphName, RE::BSFixedString keyName);
	void ClearBodyMorph(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, RE::BSFixedString morphName, RE::BSFixedString keyName);
	void UpdateItemTextureLayers(RE::StaticFunctionTag* base, RE::TESObjectREFR* refr, std::uint32_t uniqueID);
}