#pragma once

#include "IPluginInterface.h"
#include "SafeDataHolder.h"
#include "StringTable.h"
#include "CDXTextureRenderer.h"
#include <unordered_map>
#include <cstdint>
namespace RE
{
	class RE::TESObjectREFR;
}
namespace SkinLayerData
{

struct Data
{
	StringTableItem texturePath;
	StringTableItem blendMode;
	CDXTextureRenderer::TextureType textureType;
	std::uint32_t color;
};

namespace ChangeFlags
{
	static const int TEXTURE_PATH		= (1 << 0);
	static const int BLEND_MODE			= (1 << 1);
	static const int TEXTURE_TYPE		= (1 << 2);
	static const int COLOR				= (1 << 3);
};

enum class Target : std::uint8_t
{
	Face = 0,
	Body,
	Hands,
	Feet,
	Hair
};

class LayerMap : public std::map<std::int32_t, Data>
{
public:
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class TargetToLayer : public std::unordered_map<Target, LayerMap>
{
public:
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class PerspectiveToTarget : public std::unordered_map<bool, TargetToLayer>
{
public:
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class GenderToPersepective : public std::unordered_map<bool, PerspectiveToTarget>
{
public:
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};

class ActorToGender : public std::unordered_map<std::uint32_t, GenderToPersepective>
{
public:
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
};
};

class SkinLayerInterface
	: public IPluginInterface
{
public:
	enum
	{
		kCurrentPluginVersion = 1,
		kSerializationVersion1 = 1,
		kSerializationVersion = kSerializationVersion1
	};

	virtual skee_u32 GetVersion();

	// Serialization
	void Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion);
	bool Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable);
	virtual void Revert() override;

	virtual void SetSkinLayerTexture(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const char* texturePath);
	virtual void SetSkinLayerBlendMode(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const char* blendMode);
	virtual void SetSkinLayerTextureType(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const std::uint8_t textureType);
	virtual void SetSkinLayerColor(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const std::uint32_t color);
private:
	SafeDataHolder<SkinLayerData::ActorToGender> data;
};