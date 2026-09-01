#pragma once

#include "IPluginInterface.h"
#include "SafeDataHolder.h"
#include "IHashType.h"

#include "StringTable.h"

#include <RE/B/BSFixedString.h>
#include <RE/N/NiSmartPointer.h>
#include <RE/T/TESForm.h>
#include <RE/T/TESObjectREFR.h>
#include <RE/A/Actor.h>
#include <RE/T/TESObjectARMO.h>
#include <RE/T/TESObjectARMA.h>
#include <RE/T/TESObjectWEAP.h>
#include <RE/N/NiAVObject.h>
#include <RE/N/NiNode.h>
#include <RE/B/BSGeometry.h>
#include <RE/B/BGSTextureSet.h>
#include <SKSE/API.h>

#include <set>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <mutex>

class OverrideVariant;

using OverrideHandle = std::uint32_t;

class OverrideSet : public std::set<OverrideVariant>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);

	virtual void Visit(std::function<bool(OverrideVariant*)> functor);
};

template<typename T>
bool ReadKey(SKSE::SerializationInterface* intfc, T& key, std::uint32_t kVersion, const StringIdMap& stringTable);

template<typename T>
void WriteKey(SKSE::SerializationInterface* intfc, const T key, std::uint32_t kVersion);

template<typename T>
class OverrideRegistration : public std::unordered_map<T, OverrideSet>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);

	virtual void Visit(std::function<bool(const T& key, OverrideSet*)> functor);
};

class AddonRegistration : public std::unordered_map<OverrideHandle, OverrideRegistration<StringTableItem>>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class ArmorRegistration : public std::unordered_map<OverrideHandle, AddonRegistration>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class WeaponRegistration : public std::unordered_map<OverrideHandle, OverrideRegistration<StringTableItem>>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class SkinRegistration : public std::unordered_map<OverrideHandle, OverrideSet>
{
public:
	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

template<typename T, std::uint32_t N>
class MultiRegistration
{
public:
	// Serialization

	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable)
	{
		bool error = false;

		std::uint8_t size = 0;
		if (!intfc->ReadRecordData(&size, sizeof(size)))
		{
			SKSE::log::info("{} - Error loading multi-registrations", __FUNCTION__);
			error = true;
			return error;
		}

		for (std::uint32_t i = 0; i < size; i++)
		{
			std::uint8_t index = 0;
			if (!intfc->ReadRecordData(&index, sizeof(index)))
			{
				SKSE::log::info("{} - Error loading multi-registration index ({}/{})", __FUNCTION__, i + 1, size);
				error = true;
				return error;
			}

			T regs;
			if (regs.Load(intfc, kVersion, stringTable))
			{
				SKSE::log::info("{} - Error loading multi-registrations ({}/{})", __FUNCTION__, i + 1, size);
				error = true;
				return error;
			}

			table[index] = regs;

#ifdef _DEBUG
			SKSE::log::info("{} - Loaded multi-reg ({})", __FUNCTION__, index);
#endif
		}

		return error;
	}

	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion)
	{
		std::uint8_t size = 0;
		for (std::uint8_t i = 0; i < N; i++)
		{
			if (!table[i].empty())
				size++;
		}

		intfc->WriteRecordData(&size, sizeof(size));

		for (std::uint8_t i = 0; i < N; i++)
		{
			if (!table[i].empty())
			{
#ifdef _DEBUG
				SKSE::log::info("{} - Saving Multi-Reg {}", __FUNCTION__, i);
#endif
				intfc->WriteRecordData(&i, sizeof(i));
				table[i].Save(intfc, kVersion);
			}
		}
	}

	T& operator[] (const int index)
	{
		if (index > static_cast<int>(N) - 1)
			return table[0];

		return table[index];
	}

	bool empty()
	{
		std::uint8_t emptyCount = 0;
		for (std::uint8_t i = 0; i < N; i++)
		{
			if (table[i].empty())
				emptyCount++;
		}
		return emptyCount == N;
	}

	T table[N];
};

class ActorRegistrationMapHolder : public SafeDataHolder<std::unordered_map<OverrideHandle, MultiRegistration<ArmorRegistration, 2>>>
{
public:
	typedef std::unordered_map<OverrideHandle, MultiRegistration<ArmorRegistration, 2>> RegMap;

	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, OverrideHandle* outFormId, const StringIdMap& stringTable);

	friend class OverrideInterface;
};

class NodeRegistrationMapHolder : public SafeDataHolder<std::unordered_map<OverrideHandle, MultiRegistration<OverrideRegistration<StringTableItem>, 2>>>
{
public:
	typedef std::unordered_map<OverrideHandle, MultiRegistration<OverrideRegistration<StringTableItem>, 2>> RegMap;

	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, OverrideHandle* outFormId, const StringIdMap& stringTable);

	friend class OverrideInterface;
};

class WeaponRegistrationMapHolder : public SafeDataHolder<std::unordered_map<OverrideHandle, MultiRegistration<MultiRegistration<WeaponRegistration, 2>, 2>>>
{
public:
	typedef std::unordered_map<OverrideHandle, MultiRegistration<MultiRegistration<WeaponRegistration, 2>, 2>> RegMap;

	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, OverrideHandle* outFormId, const StringIdMap& stringTable);

	friend class OverrideInterface;
};

class SkinRegistrationMapHolder : public SafeDataHolder<std::unordered_map<OverrideHandle, MultiRegistration<MultiRegistration<SkinRegistration, 2>, 2>>>
{
public:
	typedef std::unordered_map<OverrideHandle, MultiRegistration<MultiRegistration<SkinRegistration, 2>, 2>> RegMap;

	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, OverrideHandle* outFormId, const StringIdMap& stringTable);

	friend class OverrideInterface;
};

class OverrideInterface
	: public IOverrideInterface
	, public IAddonAttachmentInterface
{
public:
	virtual std::uint32_t GetVersion();

	virtual bool HasArmorAddonNode(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, bool debug) override;

	virtual bool HasArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index) override;
	virtual void AddArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index, SetVariant& value) override;
	virtual bool GetArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index, GetVariant& visitor) override;
	virtual void RemoveArmorOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index) override;
	virtual void SetArmorProperties(RE::TESObjectREFR* refr, bool immediate) override;
	virtual void SetArmorProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index, SetVariant& value, bool immediate) override;
	virtual bool GetArmorProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName, std::uint16_t key, std::uint8_t index, GetVariant& value) override;
	virtual void ApplyArmorOverrides(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::NiAVObject* object, bool immediate) override;
	virtual void RemoveAllArmorOverrides() override;
	virtual void RemoveAllArmorOverridesByReference(RE::TESObjectREFR* reference) override;
	virtual void RemoveAllArmorOverridesByArmor(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor) override;
	virtual void RemoveAllArmorOverridesByAddon(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon) override;
	virtual void RemoveAllArmorOverridesByNode(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, const char* nodeName) override;

	virtual bool HasNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, std::uint16_t key, std::uint8_t index) override;
	virtual void AddNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, std::uint16_t key, std::uint8_t index, SetVariant& value) override;
	virtual bool GetNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, std::uint16_t key, std::uint8_t index, GetVariant& visitor) override;
	virtual void RemoveNodeOverride(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName, std::uint16_t key, std::uint8_t index) override;
	virtual void SetNodeProperties(RE::TESObjectREFR* refr, bool immediate) override;
	virtual void SetNodeProperty(RE::TESObjectREFR* refr, bool firstPerson, const char* nodeName, std::uint16_t key, std::uint8_t index, SetVariant& value, bool immediate) override;
	virtual bool GetNodeProperty(RE::TESObjectREFR* refr, bool firstPerson, const char* nodeName, std::uint16_t key, std::uint8_t index, GetVariant& value) override;
	virtual void ApplyNodeOverrides(RE::TESObjectREFR* refr, RE::NiAVObject* object, bool immediate) override;
	virtual void RemoveAllNodeOverrides() override;
	virtual void RemoveAllNodeOverridesByReference(RE::TESObjectREFR* reference) override;
	virtual void RemoveAllNodeOverridesByNode(RE::TESObjectREFR* refr, bool isFemale, const char* nodeName) override;

	virtual bool HasSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index) override;
	virtual void AddSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index, SetVariant& value) override;
	virtual bool GetSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index, GetVariant& visitor) override;
	virtual void RemoveSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index) override;
	virtual void SetSkinProperties(RE::TESObjectREFR* refr, bool immediate) override;
	virtual void SetSkinProperty(RE::TESObjectREFR* refr, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index, SetVariant& value, bool immediate) override;
	virtual bool GetSkinProperty(RE::TESObjectREFR* refr, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index, GetVariant& value) override;
	virtual void ApplySkinOverrides(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, std::uint32_t slotMask, RE::NiAVObject* object, bool immediate) override;
	virtual void RemoveAllSkinOverridesBySlot(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask) override;
	virtual void RemoveAllSkinOverrides() override;
	virtual void RemoveAllSkinOverridesByReference(RE::TESObjectREFR* reference) override;

	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	void Revert();

	bool LoadOverrides(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
	bool LoadNodeOverrides(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
	bool LoadWeaponOverrides(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
	bool LoadSkinOverrides(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);

	// Specific overrides
	void Impl_AddRawOverride(OverrideHandle formId, bool isFemale, OverrideHandle armorHandle, OverrideHandle addonHandle, RE::BSFixedString nodeName, OverrideVariant& value);
	void Impl_AddOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, OverrideVariant& value);

	// Non-specific overrides
	void Impl_AddRawNodeOverride(OverrideHandle handle, bool isFemale, RE::BSFixedString nodeName, OverrideVariant& value);
	void Impl_AddNodeOverride(RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, OverrideVariant& value);

	// Applies all properties for a handle
	void Impl_SetProperties(OverrideHandle handle, bool immediate);

	// Applies node properties for a handle
	void Impl_SetNodeProperties(OverrideHandle handle, bool immediate);

	// Set/Get a single property
	void Impl_SetArmorAddonProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, OverrideVariant* value, bool immediate);
	void Impl_GetArmorAddonProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, OverrideVariant* value);

	// Applies a single node property
	void Impl_SetNodeProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::BSFixedString nodeName, OverrideVariant* value, bool immediate);
	void Impl_GetNodeProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::BSFixedString nodeName, OverrideVariant* value);

	// Determines whether the node could be found
	bool Impl_HasArmorAddonNode(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, bool debug);

	// Applies all node overrides to a particular node
	void Impl_ApplyNodeOverrides(RE::TESObjectREFR* refr, RE::NiAVObject* object, bool immediate);

	// Applies all armor overrides to a particular armor
	void Impl_ApplyOverrides(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::NiAVObject* object, bool immediate);

	void Impl_RemoveAllOverrides();
	void Impl_RemoveAllReferenceOverrides(RE::TESObjectREFR* reference);

	void Impl_RemoveAllArmorOverrides(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor);
	void Impl_RemoveAllArmorAddonOverrides(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon);
	void Impl_RemoveAllArmorAddonNodeOverrides(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName);
	void Impl_RemoveArmorAddonOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);

	void Impl_RemoveAllNodeOverrides();
	void Impl_RemoveAllReferenceNodeOverrides(RE::TESObjectREFR* reference);

	void Impl_RemoveAllNodeNameOverrides(RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName);
	void Impl_RemoveNodeOverride(RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);

	OverrideVariant* Impl_GetOverride(RE::TESObjectREFR* refr, bool isFemale, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);
	OverrideVariant* Impl_GetNodeOverride(RE::TESObjectREFR* refr, bool isFemale, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);

	void Impl_AddRawWeaponOverride(OverrideHandle handle, bool isFemale, bool firstPerson, OverrideHandle weaponHandle, RE::BSFixedString nodeName, OverrideVariant& value);
	void Impl_AddWeaponOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, OverrideVariant& value);
	OverrideVariant* Impl_GetWeaponOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);
	void Impl_ApplyWeaponOverrides(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectWEAP* weapon, RE::NiAVObject* object, bool immediate);

	void Impl_RemoveAllWeaponBasedOverrides();
	void Impl_RemoveAllReferenceWeaponOverrides(RE::TESObjectREFR* reference);

	void Impl_RemoveAllWeaponOverrides(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon);
	void Impl_RemoveAllWeaponNodeOverrides(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName);
	void Impl_RemoveWeaponOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, std::uint16_t key, std::uint8_t index);

	bool Impl_HasWeaponNode(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, bool debug);
	void Impl_SetWeaponProperties(OverrideHandle handle, bool immediate);

	void Impl_SetWeaponProperty(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, OverrideVariant* value, bool immediate);
	void Impl_GetWeaponProperty(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectWEAP* weapon, RE::BSFixedString nodeName, OverrideVariant* value);

	// Skin API
	void Impl_AddRawSkinOverride(OverrideHandle handle, bool isFemale, bool firstPerson, std::uint32_t slotMask, OverrideVariant& value);
	void Impl_AddSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, OverrideVariant& value);
	OverrideVariant* Impl_GetSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index);
	void Impl_ApplySkinOverrides(RE::TESObjectREFR* refr, bool firstPerson, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, std::uint32_t slotMask, RE::NiAVObject* object, bool immediate);
	void Impl_RemoveAllSkinOverrides(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask);
	void Impl_RemoveSkinOverride(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::uint32_t slotMask, std::uint16_t key, std::uint8_t index);
	void Impl_SetSkinProperties(OverrideHandle handle, bool immediate);
	void Impl_RemoveAllSkinBasedOverrides();
	void Impl_RemoveAllReferenceSkinOverrides(RE::TESObjectREFR* reference);
	void Impl_SetSkinProperty(RE::TESObjectREFR* refr, bool firstPerson, std::uint32_t slotMask, OverrideVariant* value, bool immediate);
	void Impl_GetSkinProperty(RE::TESObjectREFR* refr, bool firstPerson, std::uint32_t slotMask, OverrideVariant* value);

	void VisitNodes(RE::TESObjectREFR* refr, std::function<void(SKEEFixedString, OverrideVariant&)> functor);
	void VisitSkin(RE::TESObjectREFR* refr, bool isFemale, bool firstPerson, std::function<void(std::uint32_t, OverrideVariant&)> functor);
	void VisitStrings(std::function<void(SKEEFixedString)> functor);

	void SetValueVariant(OverrideVariant& variant, std::uint16_t key, std::uint8_t index, SetVariant& value);
	bool GetValueVariant(OverrideVariant& variant, std::uint16_t key, std::uint8_t index, GetVariant& value);

	void Dump();
	void PrintDiagnostics();

private:
	ActorRegistrationMapHolder armorData;
	NodeRegistrationMapHolder nodeData;
	WeaponRegistrationMapHolder weaponData;
	SkinRegistrationMapHolder skinData;

	// Inherited via IAddonAttachmentInterface
	virtual void OnAttach(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::NiAVObject* object, bool isFirstPerson, RE::NiNode* skeleton, RE::NiNode* root) override;
};