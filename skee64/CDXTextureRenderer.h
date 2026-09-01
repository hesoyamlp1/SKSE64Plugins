#pragma once

#include "REX/W32/D3D11.h"
#include <DirectXMath.h>
#include <sstream>
#include "REX/W32/COMPTR.h"
#include <vector>
#include <memory>
#include <cstdint>



using namespace DirectX;

class CDXPixelShaderCache;
class CDXD3DDevice;
class CDXShaderFile;
class CDXShaderFactory;

class CDXTextureRenderer
{
public:
	struct VertexType
	{
		XMFLOAT3 position;
		XMFLOAT2 texture;
	};

	enum class TextureType : std::uint8_t
	{
		Normal = 0,
		Mask,
		Color,
		Unknown
	};

	struct LayerData
	{
		union Mode
		{
			struct BlendData
			{
				std::uint32_t		dummy;
				std::uint32_t		type;
			} blendData;
			XMINT2 data;
		} blending;
		XMFLOAT4 maskColor;
	};

	struct ConstantBufferType
	{
		XMMATRIX	world;
		XMMATRIX	view;
		XMMATRIX	projection;
		LayerData	layerData;
	};

	CDXTextureRenderer();
	virtual ~CDXTextureRenderer() { }

	virtual bool Initialize(CDXD3DDevice * device, CDXShaderFactory * factory, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile, CDXPixelShaderCache * cache);
	virtual void Render(CDXD3DDevice * pDevice, bool clear = true);
	virtual void Release();
	virtual bool SetTexture(CDXD3DDevice * device, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> texture, REX::W32::DXGI_FORMAT target);

	bool UpdateVertexBuffer(CDXD3DDevice * device);
	bool UpdateConstantBuffer(CDXD3DDevice * device);
	bool UpdateStructuredBuffer(CDXD3DDevice * device, const LayerData & layerData);

	int GetWidth() const { return m_dstDesc.width; }
	int GetHeight() const { return m_dstDesc.height; }
	REX::W32::ComPtr<REX::W32::ID3D11Texture2D> GetTexture();
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> GetResourceView();

	void AddLayer(const REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> & texture, const TextureType & type, const std::string & technique, const XMFLOAT4 & maskColor);

protected:
	bool InitializeVertices(CDXD3DDevice * device);
	bool InitializeVertexShader(CDXD3DDevice * device, CDXShaderFactory * factory, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile);

	CDXPixelShaderCache * m_shaderCache;

	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_source;

	void RenderShaders(CDXD3DDevice * device, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> sourceView);

	bool SplitSubresources(CDXD3DDevice * device, REX::W32::D3D11_TEXTURE2D_DESC desc, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> source, std::vector<REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>>& resources);
	bool CreateSubresourceDestination(CDXD3DDevice * device, REX::W32::D3D11_TEXTURE2D_DESC desc, REX::W32::ComPtr<REX::W32::ID3D11Texture2D>& outTexture, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>& outResource);

	struct ResourceData
	{
		LayerData											m_metadata;
		REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_resource;
		std::string											m_shader;
	};

	std::vector<ResourceData>							m_resources;

	REX::W32::ComPtr<REX::W32::ID3D11Buffer>				m_vertexBuffer;
	REX::W32::ComPtr<REX::W32::ID3D11Buffer>				m_indexBuffer;
	REX::W32::ComPtr<REX::W32::ID3D11VertexShader>			m_vertexShader;
	REX::W32::ComPtr<REX::W32::ID3D11InputLayout>			m_layout;
	REX::W32::ComPtr<REX::W32::ID3D11Buffer>				m_constantBuffer;
	REX::W32::ComPtr<REX::W32::ID3D11Buffer>				m_structuredBuffer;
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_structuredBufferView;
	REX::W32::ComPtr<REX::W32::ID3D11SamplerState>			m_sampleState;
	REX::W32::ComPtr<REX::W32::ID3D11BlendState>			m_alphaEnableBlendingState;

	REX::W32::ComPtr<REX::W32::ID3D11Texture2D>				m_renderTargetTexture;
	REX::W32::ComPtr<REX::W32::ID3D11RenderTargetView>		m_renderTargetView;
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_shaderResourceView;
	REX::W32::ComPtr<REX::W32::ID3D11Texture2D>				m_intermediateTexture;
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_intermediateResourceView;
	REX::W32::ComPtr<REX::W32::ID3D11Texture2D>				m_multiTexture;
	REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>	m_multiResourceView;

	int m_vertexCount;
	int m_indexCount;
	REX::W32::D3D11_TEXTURE2D_DESC	m_srcDesc;
	REX::W32::D3D11_TEXTURE2D_DESC	m_dstDesc;
};