#pragma once

#include "IPluginInterface.h"
#include "SafeDataHolder.h"

#include <unordered_map>
#include <RE/N/NiAVObject.h>
#include <RE/N/NiNode.h>
#include <RE/N/NiTransform.h>
#include <RE/T/TESObjectREFR.h>
#include <SKSE/Interfaces.h>

#include "OverrideInterface.h"
#include "OverrideVariant.h"
#include <cstdint>
class NodeTransformKeys : public std::unordered_map<StringTableItem, OverrideRegistration<StringTableItem>>
{
public:
	void Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface * intfc, std::uint32_t kVersion, const StringIdMap & stringTable);
};

class NodeTransformRegistrationMapHolder : public SafeDataHolder<std::unordered_map<std::uint32_t, MultiRegistration<MultiRegistration<NodeTransformKeys, 2>,2>>>
{
	friend class NiTransformInterface;
public:
	typedef std::unordered_map<std::uint32_t, MultiRegistration<MultiRegistration<NodeTransformKeys, 2>,2>> RegMap;

	void Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface * intfc, std::uint32_t kVersion, std::uint32_t * outFormId, const StringIdMap & stringTable);
};

// Node names are hashed here due to some case where the node "NPC" gets overwritten for some unknown reason
class NodeTransformCache : public SafeDataHolder<std::unordered_map<SKEEFixedString, std::unordered_map<SKEEFixedString, RE::NiTransform>>>
{
	friend class NiTransformInterface;
public:
	typedef std::unordered_map<SKEEFixedString, RE::NiTransform> NodeMap;
	typedef std::unordered_map<SKEEFixedString, NodeMap> RegMap;

	RE::NiTransform * GetBaseTransform(SKEEFixedString rootModel, SKEEFixedString nodeName, bool relative);
};

class NiTransformInterface : public INiTransformInterface
{
public:
	virtual skee_u32 GetVersion();

	virtual void Save(SKSE::SerializationInterface * intfc, std::uint32_t kVersion);
	virtual bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap & stringTable);
	virtual void Revert();

	virtual bool HasNodeTransformPosition(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool HasNodeTransformRotation(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool HasNodeTransformScale(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool HasNodeTransformScaleMode(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);

	virtual void AddNodeTransformPosition(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name, Position& position); // X,Y,Z
	virtual void AddNodeTransformRotation(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name, Rotation& rotation); // Euler angles
	virtual void AddNodeTransformScale(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name, float scale);
	virtual void AddNodeTransformScaleMode(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name, skee_u32 scaleMode);

	virtual Position GetNodeTransformPosition(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual Rotation GetNodeTransformRotation(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual float GetNodeTransformScale(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual skee_u32 GetNodeTransformScaleMode(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);

	virtual bool RemoveNodeTransformPosition(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool RemoveNodeTransformRotation(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool RemoveNodeTransformScale(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual bool RemoveNodeTransformScaleMode(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node, const char* name);

	virtual bool RemoveNodeTransform(RE::TESObjectREFR* refr, bool firstPerson, bool isFemale, const char* node, const char* name);
	virtual void RemoveAllReferenceTransforms(RE::TESObjectREFR* refr);

	virtual bool GetOverrideNodeTransform(RE::TESObjectREFR* refr, bool firstPerson, bool isFemale, const char* node, const char* name, skee_u16 key, RE::NiTransform* result);

	virtual void UpdateNodeAllTransforms(RE::TESObjectREFR* ref);

	virtual void VisitNodes(RE::TESObjectREFR* refr, bool firstPerson, bool isFemale, NodeVisitor& visitor);
	virtual void UpdateNodeTransforms(RE::TESObjectREFR* ref, bool firstPerson, bool isFemale, const char* node);

	// Internal implementations
	bool Impl_AddNodeTransform(RE::TESObjectREFR * ref, bool firstPerson, bool isFemale, SKEEFixedString node, SKEEFixedString name, OverrideVariant & value);
	bool Impl_RemoveNodeTransformComponent(RE::TESObjectREFR * ref, bool firstPerson, bool isFemale, SKEEFixedString node, SKEEFixedString name, std::uint16_t key, std::uint16_t index);
	bool Impl_RemoveNodeTransform(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale, SKEEFixedString node, SKEEFixedString name);
	void Impl_RemoveAllReferenceTransforms(RE::TESObjectREFR * refr);
	
	OverrideVariant Impl_GetOverrideNodeValue(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale, SKEEFixedString node, SKEEFixedString name, std::uint16_t key, std::int8_t index);
	bool Impl_GetOverrideNodeTransform(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale, SKEEFixedString node, SKEEFixedString name, std::uint16_t key, RE::NiTransform * result);

	void Impl_GetOverrideTransform(OverrideSet * set, std::uint16_t key, RE::NiTransform * result);
	SKEEFixedString GetRootModelPath(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale);

	void Impl_UpdateNodeAllTransforms(RE::TESObjectREFR * ref);

	void Impl_VisitNodes(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale, std::function<bool(SKEEFixedString, OverrideRegistration<StringTableItem>*)> functor);
	bool Impl_VisitNodeTransforms(RE::TESObjectREFR * refr, bool firstPerson, bool isFemale, RE::BSFixedString node, std::function<bool(OverrideRegistration<StringTableItem>*)> each_key, std::function<void(RE::NiNode*, RE::NiAVObject*, RE::NiTransform*)> finalize);
	void Impl_UpdateNodeTransforms(RE::TESObjectREFR * ref, bool firstPerson, bool isFemale, SKEEFixedString node);

	void VisitStrings(std::function<void(SKEEFixedString)> functor);
	
	void RemoveInvalidTransforms(std::uint32_t formId);
	void RemoveNamedTransforms(std::uint32_t formId, SKEEFixedString name);
	void SetTransforms(std::uint32_t formId, bool immediate = false, bool reset = false);

	void PrintDiagnostics();

	NodeTransformRegistrationMapHolder	transformData;
	NodeTransformCache					transformCache;
};