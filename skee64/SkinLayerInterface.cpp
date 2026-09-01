#include "SkinLayerInterface.h"
#include "RE/N/NiGeometry.h"
#include "NifUtils.h"
#include <cstdint>

extern StringTable						g_stringTable;

skee_u32 SkinLayerInterface::GetVersion()
{
	return kCurrentPluginVersion;
}

void SkinLayerInterface::Save(SKSE::SerializationInterface* intfc, std::uint32_t kVersion)
{
}

bool SkinLayerInterface::Load(SKSE::SerializationInterface* intfc, std::uint32_t kVersion, const StringIdMap& stringTable)
{
	return false;
}

void SkinLayerInterface::Revert()
{
}

void SkinLayerInterface::SetSkinLayerTexture(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const char* texturePath)
{
	data.Lock();
	data.m_data[refr->formID][isFemale][isFirstPerson][target][layer].texturePath = g_stringTable.GetString(texturePath);
	data.Release();
}

void SkinLayerInterface::SetSkinLayerBlendMode(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const char* blendMode)
{
	data.Lock();
	data.m_data[refr->formID][isFemale][isFirstPerson][target][layer].blendMode = g_stringTable.GetString(blendMode);
	data.Release();
}

void SkinLayerInterface::SetSkinLayerTextureType(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const std::uint8_t textureType)
{
	data.Lock();
	data.m_data[refr->formID][isFemale][isFirstPerson][target][layer].textureType = static_cast<CDXTextureRenderer::TextureType>(textureType);
	data.Release();
}

void SkinLayerInterface::SetSkinLayerColor(RE::TESObjectREFR* refr, bool isFemale, bool isFirstPerson, SkinLayerData::Target target, std::int32_t layer, const std::uint32_t color)
{
	data.Lock();
	data.m_data[refr->formID][isFemale][isFirstPerson][target][layer].color = color;
	data.Release();
}
