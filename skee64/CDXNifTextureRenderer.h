#pragma once

#include "CDXTextureRenderer.h"
#include "CDXRenderState.h"
#include "CDXShaderFactory.h"

#include <RE/B/BSFixedString.h>
#include <RE/N/NiSmartPointer.h>
#include <RE/N/NiTexture.h>

#include "StringTable.h"

#include <map>
#include <vector>
#include <cstdint>

class CDXPixelShaderCache;

class CDXNifTextureRenderer : public CDXTextureRenderer, public CDXRenderState
{
public:
	bool Init(CDXD3DDevice* device, CDXPixelShaderCache * cache);
	virtual ~CDXNifTextureRenderer() { }

	struct MaskData
	{
		SKEEFixedString texture;
		SKEEFixedString technique = "normal";
		std::uint32_t color = 0xFFFFFF;
		CDXTextureRenderer::TextureType textureType = CDXTextureRenderer::TextureType::Mask;
	};

	bool ApplyMasksToTexture(CDXD3DDevice* device, RE::NiPointer<RE::NiSourceTexture> texture, std::map<std::int32_t, MaskData> & masks, const RE::BSFixedString & name, RE::NiPointer<RE::NiSourceTexture> & output);

private:
	std::vector<RE::NiPointer<RE::NiSourceTexture>> m_textures;
};