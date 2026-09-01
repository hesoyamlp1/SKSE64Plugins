#include "CDXD3DDevice.h"
#include "CDXShader.h"
#include "CDXShaderCompile.h"
#include "CDXMaterial.h"

#include <sstream>



using namespace DirectX;

#define USE_GEOMETRY_WIREFRAME

CDXShader::CDXShader()
{

}

bool CDXShader::Initialize(const CDXInitParams & initParams)
{
	HRESULT result;
	REX::W32::D3D11_INPUT_ELEMENT_DESC polygonLayout[4];
	unsigned int numElements;
	REX::W32::D3D11_SAMPLER_DESC samplerDesc;
	REX::W32::D3D11_BUFFER_DESC bufferDesc;

	auto pDevice = initParams.device->GetDevice();

	// Create the vertex input layout description.
	// This setup needs to match the VertexType stucture in the ModelClass and in the shader.
	polygonLayout[0].semanticName = "POSITION";
	polygonLayout[0].semanticIndex = 0;
	polygonLayout[0].format = REX::W32::DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[0].inputSlot = 0;
	polygonLayout[0].alignedByteOffset = 0;
	polygonLayout[0].inputSlotClass = REX::W32::D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[0].instanceDataStepRate = 0;

	polygonLayout[1].semanticName = "TEXCOORD";
	polygonLayout[1].semanticIndex = 0;
	polygonLayout[1].format = REX::W32::DXGI_FORMAT_R32G32_FLOAT;
	polygonLayout[1].inputSlot = 0;
	polygonLayout[1].alignedByteOffset = 0xFFFFFFFF;
	polygonLayout[1].inputSlotClass = REX::W32::D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[1].instanceDataStepRate = 0;

	polygonLayout[2].semanticName = "NORMAL";
	polygonLayout[2].semanticIndex = 0;
	polygonLayout[2].format = REX::W32::DXGI_FORMAT_R32G32B32_FLOAT;
	polygonLayout[2].inputSlot = 0;
	polygonLayout[2].alignedByteOffset = 0xFFFFFFFF;
	polygonLayout[2].inputSlotClass = REX::W32::D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[2].instanceDataStepRate = 0;

	polygonLayout[3].semanticName = "COLOR";
	polygonLayout[3].semanticIndex = 0;
	polygonLayout[3].format = REX::W32::DXGI_FORMAT_R32G32B32A32_FLOAT;
	polygonLayout[3].inputSlot = 0;
	polygonLayout[3].alignedByteOffset = 0xFFFFFFFF;
	polygonLayout[3].inputSlotClass = REX::W32::D3D11_INPUT_PER_VERTEX_DATA;
	polygonLayout[3].instanceDataStepRate = 0;

	// Get a count of the elements in the layout.
	numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	// LightVertexShader
	if (!initParams.factory->CreateVertexShader(initParams.device, initParams.vShader[0], initParams.vShader[1], polygonLayout, numElements, m_vertexShader, m_layout))
	{
		return false;
	}

	// LightPixelShader
	if (!initParams.factory->CreatePixelShader(initParams.device, initParams.pShader[0], initParams.pShader[1], m_pixelShader))
	{
		return false;
	}

	// WireframeVertexShader
	if (!initParams.factory->CreateVertexShader(initParams.device, initParams.wvShader[0], initParams.wvShader[1], polygonLayout, numElements, m_wvShader, m_layout))
	{
		return false;
	}

	// WireframeGeometryShader
	if (!initParams.factory->CreateGeometryShader(initParams.device, initParams.wgShader[0], initParams.wgShader[1], m_wgShader))
	{
		return false;
	}

	// WireframePixelShader
	if (!initParams.factory->CreatePixelShader(initParams.device, initParams.wpShader[0], initParams.wpShader[1], m_wpShader))
	{
		return false;
	}

	// Create a texture sampler state description.
	samplerDesc.filter = REX::W32::D3D11_FILTER_ANISOTROPIC;
	samplerDesc.addressU = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.addressV = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.addressW = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.mipLODBias = 0.0f;
	samplerDesc.maxAnisotropy = 16;
	samplerDesc.comparisonFunc = REX::W32::D3D11_COMPARISON_ALWAYS;
	samplerDesc.borderColor[0] = 0;
	samplerDesc.borderColor[1] = 0;
	samplerDesc.borderColor[2] = 0;
	samplerDesc.borderColor[3] = 0;
	samplerDesc.minLOD = 0;
	samplerDesc.maxLOD = REX::W32::D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = pDevice->CreateSamplerState(&samplerDesc, m_sampleState.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create sampler state", __FUNCTION__);
		return false;
	}

	// Setup the description of the matrix dynamic constant buffer that is in the vertex shader.
	bufferDesc.usage = REX::W32::D3D11_USAGE_DYNAMIC;
	bufferDesc.bindFlags = REX::W32::D3D11_BIND_CONSTANT_BUFFER;
	bufferDesc.cpuAccessFlags = REX::W32::D3D11_CPU_ACCESS_WRITE;
	bufferDesc.miscFlags = 0;
	bufferDesc.structureByteStride = 0;
	bufferDesc.byteWidth = sizeof(VertexBuffer);

	// Create the matrix constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = pDevice->CreateBuffer(&bufferDesc, NULL, m_matrixBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create matrix buffer", __FUNCTION__);
		return false;
	}

	bufferDesc.byteWidth = sizeof(TransformBuffer);

	// Create the matrix constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = pDevice->CreateBuffer(&bufferDesc, NULL, m_transformBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create transform buffer", __FUNCTION__);
		return false;
	}

	// Setup the description of the light dynamic constant buffer that is in the pixel shader.
	// Note that ByteWidth always needs to be a multiple of 16 if using REX::W32::D3D11_BIND_CONSTANT_BUFFER or CreateBuffer will fail.
	bufferDesc.byteWidth = sizeof(MaterialBuffer);

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = pDevice->CreateBuffer(&bufferDesc, NULL, m_materialBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create material buffer", __FUNCTION__);
		return false;
	}

	// Setup the raster description which will determine how and what polygons will be drawn.
	REX::W32::D3D11_RASTERIZER_DESC rasterDesc;
	rasterDesc.antialiasedLineEnable = false;
	rasterDesc.cullMode = REX::W32::D3D11_CULL_BACK;
	rasterDesc.depthBias = 0;
	rasterDesc.depthBiasClamp = 0.0f;
	rasterDesc.depthClipEnable = true;
	rasterDesc.fillMode = REX::W32::D3D11_FILL_SOLID;
	rasterDesc.frontCounterClockwise = false;
	rasterDesc.multisampleEnable = false;
	rasterDesc.scissorEnable = false;
	rasterDesc.slopeScaledDepthBias = 0.0f;

	// Create the rasterizer state from the description we just filled out.
	result = pDevice->CreateRasterizerState(&rasterDesc, m_solidState.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create solid rasterizer state", __FUNCTION__);
		return false;
	}

#ifdef USE_GEOMETRY_WIREFRAME
	rasterDesc.fillMode = REX::W32::D3D11_FILL_SOLID;
	rasterDesc.cullMode = REX::W32::D3D11_CULL_NONE;
	rasterDesc.frontCounterClockwise = FALSE;
	rasterDesc.depthBias = REX::W32::D3D11_DEFAULT_DEPTH_BIAS;
	rasterDesc.depthBiasClamp = REX::W32::D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterDesc.slopeScaledDepthBias = REX::W32::D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterDesc.depthClipEnable = TRUE;
	rasterDesc.scissorEnable = FALSE;
	rasterDesc.multisampleEnable = TRUE;
	rasterDesc.antialiasedLineEnable = TRUE;
#else
	rasterDesc.multisampleEnable = true;
	rasterDesc.antialiasedLineEnable = true;
	rasterDesc.depthClipEnable = false;
	rasterDesc.depthBias = -1000;
	rasterDesc.cullMode = REX::W32::D3D11_CULL_NONE;
	rasterDesc.fillMode = REX::W32::D3D11_FILL_WIREFRAME;
#endif

	result = pDevice->CreateRasterizerState(&rasterDesc, m_wireState.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create wire rasterizer state", __FUNCTION__);
		return false;
	}

#ifdef USE_GEOMETRY_WIREFRAME
	REX::W32::D3D11_DEPTH_STENCIL_DESC depthDesc;
	depthDesc.depthEnable = true;
	depthDesc.depthWriteMask = REX::W32::D3D11_DEPTH_WRITE_MASK_ZERO;
	depthDesc.depthFunc = REX::W32::D3D11_COMPARISON_LESS_EQUAL;

	depthDesc.stencilEnable = FALSE;
	depthDesc.stencilReadMask = REX::W32::D3D11_DEFAULT_STENCIL_READ_MASK;
	depthDesc.stencilWriteMask = REX::W32::D3D11_DEFAULT_STENCIL_WRITE_MASK;

	// Stencil operations if pixel is front-facing.
	depthDesc.frontFace.stencilFailOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.frontFace.stencilDepthFailOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.frontFace.stencilPassOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.frontFace.stencilFunc = REX::W32::D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthDesc.backFace.stencilFailOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.backFace.stencilDepthFailOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.backFace.stencilPassOp = REX::W32::D3D11_STENCIL_OP_KEEP;
	depthDesc.backFace.stencilFunc = REX::W32::D3D11_COMPARISON_ALWAYS;

	pDevice->CreateDepthStencilState(&depthDesc, m_depthSS.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create depth stencil state", __FUNCTION__);
		return false;
	}

	REX::W32::D3D11_BLEND_DESC blendStateDescription;
	// Clear the blend state description.
	ZeroMemory(&blendStateDescription, sizeof(REX::W32::D3D11_BLEND_DESC));

	// Create an alpha enabled blend state description.
	blendStateDescription.renderTarget[0].blendEnable = TRUE;
	blendStateDescription.renderTarget[0].srcBlend = REX::W32::D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.renderTarget[0].destBlend = REX::W32::D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.renderTarget[0].blendOp = REX::W32::D3D11_BLEND_OP_ADD;
	blendStateDescription.renderTarget[0].srcBlendAlpha = REX::W32::D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.renderTarget[0].destBlendAlpha = REX::W32::D3D11_BLEND_DEST_ALPHA;
	blendStateDescription.renderTarget[0].blendOpAlpha = REX::W32::D3D11_BLEND_OP_ADD;
	blendStateDescription.renderTarget[0].renderTargetWriteMask = REX::W32::D3D11_COLOR_WRITE_ENABLE_ALL;

	// Create the blend state using the description.
	result = pDevice->CreateBlendState(&blendStateDescription, m_wireBlendState.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create wire blend state", __FUNCTION__);
		return false;
	}
#endif

	return true;
}

bool CDXShader::VSSetShaderBuffer(CDXD3DDevice * device, VertexBuffer & params)
{
	HRESULT result;
	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;
	VertexBuffer* dataPtr;

	auto pDeviceContext = device->GetDeviceContext();

	// Lock the matrix constant buffer so it can be written to.
	result = pDeviceContext->Map(m_matrixBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to map matrix buffer", __FUNCTION__);
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (VertexBuffer*)mappedResource.data;

	// Transpose the matrices to prepare them for the shader.
	params.world = XMMatrixTranspose(params.world);
	params.view = XMMatrixTranspose(params.view);
	params.projection = XMMatrixTranspose(params.projection);

	*dataPtr = params;

	// Unlock the matrix constant buffer.
	pDeviceContext->Unmap(m_matrixBuffer.Get(), 0);

	REX::W32::ID3D11Buffer* buffers[] = { m_matrixBuffer.Get() };
	pDeviceContext->VSSetConstantBuffers(0, 1, buffers);
	pDeviceContext->GSSetConstantBuffers(0, 1, buffers);
	return true;
}

bool CDXShader::VSSetTransformBuffer(CDXD3DDevice * device, TransformBuffer & params)
{
	HRESULT result;
	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;
	TransformBuffer* dataPtr;

	auto pDeviceContext = device->GetDeviceContext();

	// Lock the matrix constant buffer so it can be written to.
	result = pDeviceContext->Map(m_transformBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to map transform buffer", __FUNCTION__);
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (TransformBuffer*)mappedResource.data;

	*dataPtr = params;

	// Unlock the matrix constant buffer.
	pDeviceContext->Unmap(m_transformBuffer.Get(), 0);

	REX::W32::ID3D11Buffer* buffers[] = { m_transformBuffer.Get() };
	pDeviceContext->VSSetConstantBuffers(1, 1, buffers);
	return true;
}

bool CDXShader::PSSetMaterialBuffers(CDXD3DDevice * device, MaterialBuffer & params)
{
	HRESULT result;
	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;
	MaterialBuffer* dataPtr;

	auto pDeviceContext = device->GetDeviceContext();

	// Lock the light constant buffer so it can be written to.
	result = pDeviceContext->Map(m_materialBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to map material buffer", __FUNCTION__);
		return false;
	}

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MaterialBuffer*)mappedResource.data;

	// Copy the lighting variables into the constant buffer.
	*dataPtr = params;

	// Unlock the constant buffer.
	pDeviceContext->Unmap(m_materialBuffer.Get(), 0);

	REX::W32::ID3D11Buffer* buffers[] = { m_materialBuffer.Get() };
	pDeviceContext->PSSetConstantBuffers(0, 1, buffers);
	return true;
}

void CDXShader::RenderShader(CDXD3DDevice * device, const std::shared_ptr<CDXMaterial>& material)
{
	auto pDeviceContext = device->GetDeviceContext();
	// Set the vertex input layout.
	pDeviceContext->IASetInputLayout(m_layout.Get());

	// Set the vertex and pixel shaders that will be used to render this triangle.
#ifdef USE_GEOMETRY_WIREFRAME
	if (!m_baseSS.Get()) {
		pDeviceContext->OMGetDepthStencilState(m_baseSS.GetAddressOf(), &m_baseRef);
	}
#endif
	
	CDXShader::MaterialBuffer mat;

	REX::W32::ID3D11RasterizerState* state = m_solidState.Get();
	REX::W32::ID3D11VertexShader* vshader = m_vertexShader.Get();
	REX::W32::ID3D11PixelShader* pshader = m_pixelShader.Get();
	REX::W32::ID3D11GeometryShader* gshader = nullptr;
	REX::W32::ID3D11BlendState * blendingState = material->GetBlendingState(device).Get();

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	if (material->IsWireframe())
	{
		mat.hasNormal = false;
		mat.hasSpecular = false;
		mat.hasDetailMap = false;
		mat.hasTintMask = false;
		mat.tintColor = XMFLOAT4(0, 0, 0, 0);
		mat.wireColor = material->GetWireframeColor();
		mat.alphaThreshold = material->GetAlphaBlending() ? 0.0f : material->GetAlphaThreshold() / 255.0f;
		state = m_wireState.Get();
		pshader = m_wpShader.Get();
#ifdef USE_GEOMETRY_WIREFRAME
		vshader = m_wvShader.Get();
		gshader = m_wgShader.Get();
#endif

		if (m_wireBlendState.Get())
		{
			blendingState = m_wireBlendState.Get();
		}
		

		if (m_depthSS.Get())
		{
			pDeviceContext->OMSetDepthStencilState(m_depthSS.Get(), 0);
		}
	}
	else
	{
		mat.hasNormal = material->HasNormal();
		mat.hasSpecular = material->HasSpecular();
		mat.hasDetailMap = material->HasDetail();
		mat.hasTintMask = material->HasTintMask();
		mat.tintColor = material->GetTintColor();
		mat.wireColor = XMFLOAT4(0, 0, 0, 0);
		mat.alphaThreshold = material->GetAlphaBlending() ? 0.0f : material->GetAlphaThreshold() / 255.0f;

#ifdef USE_GEOMETRY_WIREFRAME
		pDeviceContext->OMSetDepthStencilState(m_baseSS.Get(), m_baseRef);
#endif
	}

	PSSetMaterialBuffers(device, mat);

	pDeviceContext->OMSetBlendState(blendingState, blendFactor, 0xffffffff);

	pDeviceContext->RSSetState(state);

	pDeviceContext->VSSetShader(vshader, nullptr, 0);
	pDeviceContext->PSSetShader(pshader, nullptr, 0);
	pDeviceContext->GSSetShader(gshader, nullptr, 0);

	// Set shader texture resource in the pixel shader.

	auto textures = material->GetTextures();
	REX::W32::ID3D11ShaderResourceView* resources[] = {
		textures[0].Get(),
		textures[1].Get(),
		textures[2].Get(),
		textures[3].Get(),
		textures[4].Get()
	};
	pDeviceContext->PSSetShaderResources(0, 5, resources);

	// Set the sampler state in the pixel shader.

	REX::W32::ID3D11SamplerState * samplers[] = { m_sampleState.Get() };
	pDeviceContext->PSSetSamplers(0, 1, samplers);
}