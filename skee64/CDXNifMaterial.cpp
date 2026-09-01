#include "CDXNifMaterial.h"
#include "REX/W32/D3D11_3.h"



void CDXNifMaterial::SetNiTexture(int index, RE::NiTexture* texture)
{
	m_pTextures[index].reset(texture);

	auto srcTex = static_cast<RE::NiSourceTexture*>(m_pTextures[index].get());
	if (srcTex && srcTex->rendererTexture) {
		SetTexture(index, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>(reinterpret_cast<REX::W32::ID3D11ShaderResourceView*>(srcTex->rendererTexture->resourceView)));
	}
}