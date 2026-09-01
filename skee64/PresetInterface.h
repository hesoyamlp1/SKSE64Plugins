#pragma once

#include "IPluginInterface.h"
#include "SafeDataHolder.h"
#include "StringTable.h"
#include "OverrideVariant.h"
#include <RE/B/BSFixedString.h>
#include <vector>
#include <cstdint>
namespace RE
{
	class RE::Actor;
	class RE::BGSHeadPart;
	class RE::BSFaceGenNiNode;
	class RE::TESNPC;
	class RE::TESRace;
}


class SculptData;
using SculptDataPtr = std::shared_ptr<SculptData>;

class PresetData
{
public:
	PresetData();

	struct Tint
	{
		std::uint32_t index;
		std::uint32_t color;
		SKEEFixedString name;
	};

	struct Morph
	{
		float value;
		SKEEFixedString name;
	};

	struct Texture
	{
		std::uint8_t index;
		SKEEFixedString name;
	};

	float weight;
	std::uint32_t hairColor;
	std::vector<std::string> modList;
	std::vector<RE::BGSHeadPart*> headParts;
	std::vector<std::int32_t> presets;
	std::vector<float> morphs;
	std::vector<Tint> tints;
	std::vector<Morph> customMorphs;
	std::vector<Texture> faceTextures;
	RE::BGSTextureSet* headTexture;
	RE::BSFixedString tintTexture;
	typedef std::map<SKEEFixedString, std::vector<OverrideVariant>> OverrideData;
	OverrideData overrideData;
	typedef std::map<std::uint32_t, std::vector<OverrideVariant>> SkinData;
	SkinData skinData[2];
	typedef std::map<SKEEFixedString, std::map<SKEEFixedString, std::vector<OverrideVariant>>> TransformData;
	TransformData transformData[2];
	SculptDataPtr sculptData;
	typedef std::unordered_map<SKEEFixedString, std::unordered_map<SKEEFixedString, float>> BodyMorphData;
	BodyMorphData bodyMorphData;
};
using PresetDataPtr = std::shared_ptr<PresetData>;
class PresetMap : public SafeDataHolder<std::unordered_map<RE::TESNPC*, PresetDataPtr>>
{
public:
	friend class PresetInterface;
};

class PresetInterface : public IPresetInterface
{
public:
    virtual skee_u32 GetVersion() override { return kCurrentPluginVersion; }
    virtual void Revert() { };

	PresetDataPtr GetMappedPreset(RE::TESNPC* npc);
	void AssignMappedPreset(RE::TESNPC* npc, PresetDataPtr presetData);
	void ApplyMappedPreset(RE::TESNPC* npc, RE::BSFaceGenNiNode* faceNode, RE::BGSHeadPart* headPart);
	bool EraseMappedPreset(RE::TESNPC* npc);
	void ClearMappedPresets();

	// Legacy
	bool SaveBinaryPreset(const char* filePath);
	bool LoadBinaryPreset(const char* filePath, PresetDataPtr presetData);

	void ApplyPresetData(RE::Actor* actor, PresetDataPtr presetData, bool setSkinColor = false, ApplyTypes applyType = kPresetApplyAll);
	void ApplyPreset(RE::Actor* actor, RE::TESRace* race, RE::TESNPC* npc, PresetDataPtr presetData, ApplyTypes applyType);

	bool SaveJsonPreset(const char* filePath, RE::Actor* actor);
	bool LoadJsonPreset(const char* filePath, PresetDataPtr presetData);

protected:
	PresetMap m_mappedPreset;

	// Inherited via IPresetInterface
	virtual bool SavePreset(const char* filePath, const char* tintPath, RE::Actor* actor);
	virtual bool LoadPreset(const char* filePath, const char* tintPath, RE::Actor* actor, ApplyTypes applyTypes = kPresetApplyAll);
};