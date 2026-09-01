#include "CDXNifTextureRenderer.h"
#include "CDXD3DDevice.h"
#include "CDXTypes.h"
#include "CDXBSShaderResource.h"

#include "FileUtils.h"
#include "SKEEHooks.h"

#include <cstdint>



bool CDXNifTextureRenderer::Init(CDXD3DDevice* device, CDXPixelShaderCache * cache)
{
	CDXBSShaderResource sourceFile("SKSE/Plugins/NiOverride/Shaders/texture.fx", "TextureVertex");
	CDXBSShaderResource compiledFile("SKSE/Plugins/NiOverride/Shaders/Compiled/texture.cso");
	CDXShaderFactory factory;

	return Initialize(device, &factory, &sourceFile, &compiledFile, cache);
}

bool CDXNifTextureRenderer::ApplyMasksToTexture(CDXD3DDevice* device, RE::NiPointer<RE::NiSourceTexture> texture, std::map<std::int32_t, MaskData> & masks, const RE::BSFixedString & name, RE::NiPointer<RE::NiSourceTexture> & output)
{
	auto srcTex = static_cast<RE::NiSourceTexture*>(texture.get());
	auto rendererData = srcTex ? srcTex->rendererTexture : nullptr;
	if (!rendererData)
	{
		SKSE::log::error("{} - Texture has no rendererData", __FUNCTION__);
		return false;
	}

	bool activeLayers = false;
	for (const auto & mask : masks)
	{
		float a = ((mask.second.color >> 24) & 0xFF) / 255.0f;
		if (a > 0.0f)
		{
			activeLayers = true;
			break;
		}
	}

	// There are no active layers, use the source texture instead
	if (!activeLayers)
	{
		output = texture;
		return true;
	}

	if (!SetTexture(device, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>(reinterpret_cast<REX::W32::ID3D11ShaderResourceView*>(rendererData->resourceView)), REX::W32::DXGI_FORMAT_R8G8B8A8_UNORM))
	{
		return false;
	}

	m_resources.clear();

	std::vector<RE::NiPointer<RE::NiSourceTexture>> textures;
	for (const auto & mask : masks)
	{
		RE::NiPointer<RE::NiTexture> texture;
		if (mask.second.texture.length() > 0)
		{
			RE::BSShaderManager::GetTexture(mask.second.texture.c_str(), 1, texture, false);
		}

		float a = ((mask.second.color >> 24) & 0xFF) / 255.0f;
		if (a > 0.0f)
		{
			float r = ((mask.second.color >> 16) & 0xFF) / 255.0f;
			float g = ((mask.second.color >> 8) & 0xFF) / 255.0f;
			float b = (mask.second.color & 0xFF) / 255.0f;
			REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> layerTex;
			if (auto srcTex2 = static_cast<RE::NiSourceTexture*>(texture.get())) {
				if (srcTex2->rendererTexture) {
					layerTex = REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>(reinterpret_cast<REX::W32::ID3D11ShaderResourceView*>(srcTex2->rendererTexture->resourceView));
				}
			}
			AddLayer(layerTex, mask.second.textureType, mask.second.technique.c_str(), XMFLOAT4(r, g, b, a));
		}

		if (texture) {
			textures.push_back(RE::NiPointer<RE::NiSourceTexture>(static_cast<RE::NiSourceTexture*>(texture.get())));
		}
	}

	// Will promote caching of textures for live-editing as this will hold onto them until cleanup
	m_textures = textures;

	EnterCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);
	BackupRenderState(device);
	Render(device);
	RestoreRenderState(device);
	LeaveCriticalSection(&RE::BSGraphics::Renderer::GetSingleton()->GetRendererData().lock);

	output.reset(SKEE::CreateSourceTexture(name));
	RE::NiTexture::RendererData * sourceData = new RE::NiTexture::RendererData(GetWidth(), GetHeight());
	sourceData->texture = GetTexture().Get();
	sourceData->texture->AddRef();
	sourceData->resourceView = GetResourceView().Get();
	sourceData->resourceView->AddRef();
	if (auto srcOut = output.get()) {
		srcOut->rendererTexture = reinterpret_cast<RE::BSGraphics::Texture*>(sourceData);
	}

	return true;
}