#pragma once

#include "CDXD3DDevice.h"



class CDXRenderState
{
public:
	void BackupRenderState(CDXD3DDevice * device);
	void RestoreRenderState(CDXD3DDevice * device);

private:
	struct BACKUP_DX11_STATE
	{
		UINT                        ScissorRectsCount, ViewportsCount;
		REX::W32::D3D11_RECT                  ScissorRects[16];
		REX::W32::D3D11_VIEWPORT              Viewports[16];
		REX::W32::ID3D11RasterizerState*      RS;
		REX::W32::ID3D11BlendState*           BlendState;
		FLOAT                       BlendFactor[4];
		UINT                        SampleMask;
		UINT                        StencilRef;
		REX::W32::ID3D11DepthStencilState*    DepthStencilState;
		REX::W32::ID3D11ShaderResourceView*   PSShaderResource;
		REX::W32::ID3D11SamplerState*         PSSampler;
		REX::W32::ID3D11ShaderResourceView*   GSShaderResource;
		REX::W32::ID3D11PixelShader*          PS;
		REX::W32::ID3D11VertexShader*         VS;
		REX::W32::ID3D11GeometryShader*       GS;
		UINT                        PSInstancesCount, VSInstancesCount, GSInstancesCount;
		REX::W32::ID3D11ClassInstance*        PSInstances[256], *VSInstances[256], *GSInstances[256];   // 256 is max according to PSSetShader documentation
		REX::W32::D3D11_PRIMITIVE_TOPOLOGY    PrimitiveTopology;
		REX::W32::ID3D11Buffer*               IndexBuffer, *VertexBuffer, *VSConstantBuffer, *GSConstantBuffer;
		UINT                        IndexBufferOffset, VertexBufferStride, VertexBufferOffset;
		REX::W32::DXGI_FORMAT                 IndexBufferFormat;
		REX::W32::ID3D11InputLayout*          InputLayout;
		REX::W32::ID3D11RenderTargetView*		RenderTargetViews[8];
		REX::W32::ID3D11DepthStencilView*		DepthStencilView;
	};

	BACKUP_DX11_STATE m_backupState;
};