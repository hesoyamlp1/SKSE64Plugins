#include "CDXD3DDevice.h"
#include "CDXMaterial.h"
#include "CDXShader.h"
#include <cstdint>



using namespace DirectX;

CDXMaterial::CDXMaterial()
{
	m_pTextures[0] = nullptr;
	m_pTextures[1] = nullptr;
	m_pTextures[2] = nullptr;
	m_pTextures[3] = nullptr;
	m_pTextures[4] = nullptr;
	m_wireframeColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
	m_tintColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
	m_alphaThreshold = 0;
	m_shaderFlags1 = 0;
	m_shaderFlags2 = 0;
	SetAlphaBlending(false);
	SetSrcBlendMode(ALPHA_SRCALPHA);
	SetDestBlendMode(ALPHA_INVSRCALPHA);
	SetTestMode(TEST_ALWAYS);
	m_wireframe = false;
	m_blendingDirty = true;
	m_blendingState = nullptr;
}

CDXMaterial::~CDXMaterial()
{

}

void CDXMaterial::SetTexture(int index, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> texture)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_pTextures[index] = texture;
}

REX::W32::ComPtr<REX::W32::ID3D11BlendState> CDXMaterial::GetBlendingState(CDXD3DDevice * device)
{
	if (m_blendingDirty)
	{
		REX::W32::D3D11_BLEND_DESC blendStateDescription;
		// Clear the blend state description.
		ZeroMemory(&blendStateDescription, sizeof(REX::W32::D3D11_BLEND_DESC));

		// Create an alpha enabled blend state description.
		blendStateDescription.renderTarget[0].blendEnable = GetAlphaBlending();
		blendStateDescription.renderTarget[0].srcBlend = GetD3DBlendMode(GetSrcBlendMode());
		blendStateDescription.renderTarget[0].destBlend = GetD3DBlendMode(GetDestBlendMode());;
		blendStateDescription.renderTarget[0].blendOp = REX::W32::D3D11_BLEND_OP_ADD;
		blendStateDescription.renderTarget[0].srcBlendAlpha = REX::W32::D3D11_BLEND_SRC_ALPHA;
		blendStateDescription.renderTarget[0].destBlendAlpha = REX::W32::D3D11_BLEND_DEST_ALPHA;
		blendStateDescription.renderTarget[0].blendOpAlpha = REX::W32::D3D11_BLEND_OP_ADD;
		blendStateDescription.renderTarget[0].renderTargetWriteMask = REX::W32::D3D11_COLOR_WRITE_ENABLE_ALL;

		// Create the blend state using the description.
		HRESULT result = device->GetDevice()->CreateBlendState(&blendStateDescription, m_blendingState.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			return nullptr;
		}

		m_blendingDirty = false;
	}

	return m_blendingState;
}

void CDXMaterial::SetWireframeColor(DirectX::XMFLOAT4 color)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_wireframeColor = color;
}

void CDXMaterial::SetTintColor(DirectX::XMFLOAT4 color)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_tintColor = color;
}

DirectX::XMFLOAT4 & CDXMaterial::GetWireframeColor()
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	return m_wireframeColor;
}

DirectX::XMFLOAT4 & CDXMaterial::GetTintColor()
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	return m_tintColor;
}

void CDXMaterial::SetShaderFlags1(std::uint32_t flags)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_shaderFlags1 = flags;
}
void CDXMaterial::SetShaderFlags2(std::uint32_t flags)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_shaderFlags2 = flags;
}

void CDXMaterial::SetFlags(std::uint16_t flags)
{
#ifdef CDX_MUTEX
	std::lock_guard<std::mutex> guard(m_mutex);
#endif
	m_uFlags = flags;
}

void CDXMaterial::SetAlphaBlending(bool bAlpha)
{
	SetBit(bAlpha, ALPHA_BLEND_MASK);
	m_blendingDirty = true;
}

bool CDXMaterial::GetAlphaBlending() const
{
	return GetBit(ALPHA_BLEND_MASK);
}

void CDXMaterial::SetSrcBlendMode(AlphaFunction eSrcBlend) 
{ 
	SetField((std::uint16_t)(eSrcBlend), SRC_BLEND_MASK, SRC_BLEND_POS);
	m_blendingDirty = true;
}

CDXMaterial::AlphaFunction CDXMaterial::GetSrcBlendMode() const
{ 
	return (CDXMaterial::AlphaFunction)GetField(SRC_BLEND_MASK, SRC_BLEND_POS);
}

void CDXMaterial::SetDestBlendMode(AlphaFunction eDestBlend)
{ 
	SetField((std::uint16_t)(eDestBlend), DEST_BLEND_MASK, DEST_BLEND_POS);
	m_blendingDirty = true;
}

CDXMaterial::AlphaFunction CDXMaterial::GetDestBlendMode() const
{ 
	return (CDXMaterial::AlphaFunction)GetField(DEST_BLEND_MASK, DEST_BLEND_POS);
}

void CDXMaterial::SetAlphaTesting(bool bAlpha)
{
	SetBit(bAlpha, TEST_ENABLE_MASK);
}
//---------------------------------------------------------------------------
bool CDXMaterial::GetAlphaTesting() const
{
	return GetBit(TEST_ENABLE_MASK);
}
//---------------------------------------------------------------------------
void CDXMaterial::SetTestMode(TestFunction eTestFunc)
{ 
	SetField((std::uint16_t)(eTestFunc), TEST_FUNC_MASK, TEST_FUNC_POS);
}
//---------------------------------------------------------------------------
CDXMaterial::TestFunction CDXMaterial::GetTestMode() const
{ 
	return (CDXMaterial::TestFunction) GetField(TEST_FUNC_MASK, TEST_FUNC_POS);
}

REX::W32::D3D11_BLEND CDXMaterial::GetD3DBlendMode(AlphaFunction alphaFunc)
{
	static std::unordered_map<AlphaFunction, REX::W32::D3D11_BLEND> testMode = {
		{ ALPHA_ONE, REX::W32::D3D11_BLEND_ONE },
		{ ALPHA_ZERO, REX::W32::D3D11_BLEND_ZERO },
		{ ALPHA_SRCCOLOR, REX::W32::D3D11_BLEND_SRC_COLOR },
		{ ALPHA_INVSRCCOLOR, REX::W32::D3D11_BLEND_INV_SRC_COLOR },
		{ ALPHA_DESTCOLOR, REX::W32::D3D11_BLEND_DEST_COLOR },
		{ ALPHA_INVDESTCOLOR, REX::W32::D3D11_BLEND_INV_DEST_COLOR },
		{ ALPHA_SRCALPHA, REX::W32::D3D11_BLEND_SRC_ALPHA },
		{ ALPHA_INVSRCALPHA, REX::W32::D3D11_BLEND_INV_SRC_ALPHA },
		{ ALPHA_DESTALPHA, REX::W32::D3D11_BLEND_DEST_ALPHA },
		{ ALPHA_INVDESTALPHA, REX::W32::D3D11_BLEND_INV_DEST_ALPHA },
		{ ALPHA_SRCALPHASAT, REX::W32::D3D11_BLEND_SRC_ALPHA_SAT }
	};
	auto it = testMode.find(alphaFunc);
	if (it != testMode.end())
	{
		return it->second;
	}
	return REX::W32::D3D11_BLEND_ZERO;
}