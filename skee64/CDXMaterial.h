#ifndef __CDXMATERIAL__
#define __CDXMATERIAL__

#pragma once

#include "CDXTypes.h"

#include <mutex>
#include "REX/W32/D3D11_3.h"
#include <DirectXMath.h>
#include "REX/W32/COMPTR.h"
#include <cstdint>



#define DeclareFlags(type) \
	private: \
	type m_uFlags; \
	void SetField(type uVal, type uMask, type uPos) \
		{ \
		m_uFlags = (m_uFlags & ~uMask) | (uVal << uPos); \
		} \
		type GetField(type uMask, type uPos) const \
		{ \
		return (m_uFlags & uMask) >> uPos; \
		} \
		void SetBit(bool bVal, type uMask) \
		{ \
		if (bVal) \
			{ \
			m_uFlags |= uMask; \
			} \
			else \
			{ \
			m_uFlags &= ~uMask; \
			} \
		}\
		bool GetBit(type uMask) const \
		{ \
		return (m_uFlags & uMask) != 0; \
		}

static std::uint32_t mappedAlphaFunctions[] = {
	REX::W32::D3D11_BLEND_ONE,
	REX::W32::D3D11_BLEND_ZERO,
	REX::W32::D3D11_BLEND_SRC_COLOR,
	REX::W32::D3D11_BLEND_INV_SRC_COLOR,
	REX::W32::D3D11_BLEND_DEST_COLOR,
	REX::W32::D3D11_BLEND_INV_DEST_COLOR,
	REX::W32::D3D11_BLEND_SRC_ALPHA,
	REX::W32::D3D11_BLEND_INV_SRC_ALPHA,
	REX::W32::D3D11_BLEND_DEST_ALPHA,
	REX::W32::D3D11_BLEND_INV_DEST_ALPHA,
	REX::W32::D3D11_BLEND_SRC_ALPHA_SAT
};

static std::uint32_t mappedTestFunctions[] = {
	REX::W32::D3D11_COMPARISON_ALWAYS,
	REX::W32::D3D11_COMPARISON_LESS,
	REX::W32::D3D11_COMPARISON_EQUAL,
	REX::W32::D3D11_COMPARISON_LESS_EQUAL,
	REX::W32::D3D11_COMPARISON_GREATER,
	REX::W32::D3D11_COMPARISON_NOT_EQUAL,
	REX::W32::D3D11_COMPARISON_GREATER_EQUAL,
	REX::W32::D3D11_COMPARISON_NEVER,
};

class CDXD3DDevice;
class CDXShader;

class CDXMaterial
{
public:
	CDXMaterial();
	~CDXMaterial();

	void SetTexture(int index, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> texture);
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>* GetTextures() { return m_pTextures; }

	REX::W32::ComPtr<REX::W32::ID3D11BlendState> GetBlendingState(CDXD3DDevice * device);

	void SetWireframeColor(DirectX::XMFLOAT4 color);
	DirectX::XMFLOAT4 & GetWireframeColor();

	void SetTintColor(DirectX::XMFLOAT4 color);
	DirectX::XMFLOAT4 & GetTintColor();

	std::uint32_t GetShaderFlags1() const { return m_shaderFlags1; }
	std::uint32_t GetShaderFlags2() const { return m_shaderFlags2; }

	void SetShaderFlags1(std::uint32_t flags);
	void SetShaderFlags2(std::uint32_t flags);

	void SetFlags(std::uint16_t flags);

	enum AlphaFunction
	{
		ALPHA_ONE,
		ALPHA_ZERO,
		ALPHA_SRCCOLOR,
		ALPHA_INVSRCCOLOR,
		ALPHA_DESTCOLOR,
		ALPHA_INVDESTCOLOR,
		ALPHA_SRCALPHA,
		ALPHA_INVSRCALPHA,
		ALPHA_DESTALPHA,
		ALPHA_INVDESTALPHA,
		ALPHA_SRCALPHASAT,
		ALPHA_MAX_MODES
	};

	REX::W32::D3D11_BLEND GetD3DBlendMode(AlphaFunction alphaFunc);

	enum
	{
		ALPHA_BLEND_MASK          = 0x0001,
		SRC_BLEND_MASK      = 0x001e,
		SRC_BLEND_POS       = 1,
		DEST_BLEND_MASK     = 0x01e0,
		DEST_BLEND_POS      = 5,
		TEST_ENABLE_MASK    = 0x0200,
		TEST_FUNC_MASK      = 0x1c00,
		TEST_FUNC_POS       = 10,
		ALPHA_NOSORTER_MASK = 0x2000
	};

	enum TestFunction
	{
		TEST_ALWAYS,
		TEST_LESS,
		TEST_EQUAL,
		TEST_LESSEQUAL,
		TEST_GREATER,
		TEST_NOTEQUAL,
		TEST_GREATEREQUAL,
		TEST_NEVER,
		TEST_MAX_MODES
	};

	void SetAlphaBlending(bool bAlpha);
	bool GetAlphaBlending() const;

	void SetSrcBlendMode(AlphaFunction eSrcBlend);
	AlphaFunction GetSrcBlendMode() const;

	void SetDestBlendMode(AlphaFunction eDestBlend);
	AlphaFunction GetDestBlendMode() const;

	void SetAlphaTesting(bool bAlpha);
	bool GetAlphaTesting() const;

	void SetTestMode(TestFunction eTestFunc);
	TestFunction GetTestMode() const;

	void SetAlphaThreshold(std::uint8_t thresh) { m_alphaThreshold = thresh; }
	std::uint8_t GetAlphaThreshold() const { return m_alphaThreshold; }

	bool IsWireframe() const { return m_wireframe; }
	void SetWireframe(bool w) { m_wireframe = w; }

	bool HasDiffuse() const { return m_pTextures[0] .Get() != nullptr; }
	bool HasNormal() const { return m_pTextures[1] .Get() != nullptr; }
	bool HasSpecular() const { return m_pTextures[2] .Get() != nullptr; }
	bool HasDetail() const { return m_pTextures[3] .Get() != nullptr; }
	bool HasTintMask() const { return m_pTextures[4] .Get() != nullptr; }

protected:
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> m_pTextures[5];
	REX::W32::ComPtr<REX::W32::ID3D11BlendState> m_blendingState;
	bool m_blendingDirty;

	DirectX::XMFLOAT4 m_wireframeColor;
	DirectX::XMFLOAT4 m_tintColor;
	DeclareFlags(std::uint16_t);
	std::uint32_t	m_shaderFlags1;
	std::uint32_t	m_shaderFlags2;
	std::uint8_t	m_alphaThreshold;
	bool	m_wireframe;

#ifdef CDX_MUTEX
	std::mutex	m_mutex;
#endif
};

#endif
