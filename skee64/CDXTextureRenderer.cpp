#include "CDXTextureRenderer.h"
#include "CDXD3DDevice.h"
#include "CDXPixelShaderCache.h"
#include "CDXShaderCompile.h"
#include "CDXShaderFactory.h"
#include "CDXTypes.h"
#include <cstdint>



CDXTextureRenderer::CDXTextureRenderer()
{
	m_shaderCache = nullptr;
	m_source = nullptr;
	ZeroMemory(&m_srcDesc, sizeof(REX::W32::D3D11_TEXTURE2D_DESC));
	ZeroMemory(&m_dstDesc, sizeof(REX::W32::D3D11_TEXTURE2D_DESC));
	m_dstDesc.width = 8;
	m_dstDesc.height = 8;
	m_vertexBuffer = nullptr;
	m_indexBuffer = nullptr;
	m_vertexShader = nullptr;
	m_layout = nullptr;
	m_constantBuffer = nullptr;
	m_sampleState = nullptr;
	m_alphaEnableBlendingState = nullptr;

	m_renderTargetTexture = nullptr;
	m_renderTargetView = nullptr;
	m_shaderResourceView = nullptr;
}

bool CDXTextureRenderer::Initialize(CDXD3DDevice * device, CDXShaderFactory * factory, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile, CDXPixelShaderCache * cache)
{
	m_shaderCache = cache;

	if (!InitializeVertices(device))
	{
		return false;
	}
	if (!InitializeVertexShader(device, factory, sourceFile, precompiledFile))
	{
		return false;
	}

	REX::W32::ComPtr<REX::W32::ID3D11Device> pDevice = device->GetDevice();

	REX::W32::D3D11_BLEND_DESC blendStateDescription;
	ZeroMemory(&blendStateDescription, sizeof(REX::W32::D3D11_BLEND_DESC));

	// Create an alpha enabled blend state description.
	blendStateDescription.renderTarget[0].blendEnable = FALSE;
	blendStateDescription.renderTarget[0].srcBlend = REX::W32::D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.renderTarget[0].destBlend = REX::W32::D3D11_BLEND_INV_SRC_ALPHA;
	blendStateDescription.renderTarget[0].blendOp = REX::W32::D3D11_BLEND_OP_ADD;
	blendStateDescription.renderTarget[0].srcBlendAlpha = REX::W32::D3D11_BLEND_SRC_ALPHA;
	blendStateDescription.renderTarget[0].destBlendAlpha = REX::W32::D3D11_BLEND_DEST_ALPHA;
	blendStateDescription.renderTarget[0].blendOpAlpha = REX::W32::D3D11_BLEND_OP_ADD;
	blendStateDescription.renderTarget[0].renderTargetWriteMask = REX::W32::D3D11_COLOR_WRITE_ENABLE_ALL;

	// Create the blend state using the description.
	HRESULT result = pDevice->CreateBlendState(&blendStateDescription, m_alphaEnableBlendingState.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool CDXTextureRenderer::InitializeVertices(CDXD3DDevice * device)
{
	std::unique_ptr<VertexType[]> vertices;
	std::unique_ptr<unsigned long[]> indices;
	REX::W32::D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	REX::W32::D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int i;

	REX::W32::ComPtr<REX::W32::ID3D11Device> pDevice = device->GetDevice();

	// Set the number of vertices in the vertex array.
	m_vertexCount = 6;

	// Set the number of indices in the index array.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = std::make_unique<VertexType[]>(m_vertexCount);
	if (!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = std::make_unique<unsigned long[]>(m_indexCount);
	if (!indices)
	{
		return false;
	}

	// Initialize vertex array to zeros at first.
	memset(vertices.get(), 0, (sizeof(VertexType) * m_vertexCount));

	// Load the index array with data.
	for (i = 0; i < m_indexCount; i++)
	{
		indices[i] = i;
	}

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.usage = REX::W32::D3D11_USAGE_DYNAMIC;
	vertexBufferDesc.byteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.bindFlags = REX::W32::D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.cpuAccessFlags = REX::W32::D3D11_CPU_ACCESS_WRITE;
	vertexBufferDesc.miscFlags = 0;
	vertexBufferDesc.structureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.sysMem = vertices.get();
	vertexData.sysMemPitch = 0;
	vertexData.sysMemSlicePitch = 0;

	// Now create the vertex buffer.
	result = pDevice->CreateBuffer(&vertexBufferDesc, &vertexData, m_vertexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
	indexBufferDesc.usage = REX::W32::D3D11_USAGE_DEFAULT;
	indexBufferDesc.byteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.bindFlags = REX::W32::D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.cpuAccessFlags = 0;
	indexBufferDesc.miscFlags = 0;
	indexBufferDesc.structureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.sysMem = indices.get();
	indexData.sysMemPitch = 0;
	indexData.sysMemSlicePitch = 0;

	// Create the index buffer.
	result = pDevice->CreateBuffer(&indexBufferDesc, &indexData, m_indexBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

bool CDXTextureRenderer::InitializeVertexShader(CDXD3DDevice * device, CDXShaderFactory * factory, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile)
{
	HRESULT result;
	REX::W32::ComPtr<REX::W32::ID3D11Device> pDevice = device->GetDevice();
	std::stringstream errors;

	REX::W32::D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
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

	// Get a count of the elements in the layout.
	unsigned int numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

	if (!factory->CreateVertexShader(device, sourceFile, precompiledFile, polygonLayout, numElements, m_vertexShader, m_layout))
	{
		return false;
	}

	// Setup the description of the dynamic constant buffer that is in the vertex shader.
	REX::W32::D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.usage = REX::W32::D3D11_USAGE_DYNAMIC;
	constantBufferDesc.byteWidth = sizeof(ConstantBufferType);
	constantBufferDesc.bindFlags = REX::W32::D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.cpuAccessFlags = REX::W32::D3D11_CPU_ACCESS_WRITE;
	constantBufferDesc.miscFlags = 0;
	constantBufferDesc.structureByteStride = 0;

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	result = pDevice->CreateBuffer(&constantBufferDesc, nullptr, m_constantBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	REX::W32::D3D11_BUFFER_DESC sBufferDesc;
	sBufferDesc.usage = REX::W32::D3D11_USAGE_DYNAMIC;
	sBufferDesc.byteWidth = sizeof(LayerData);
	sBufferDesc.bindFlags = REX::W32::D3D11_BIND_SHADER_RESOURCE;
	sBufferDesc.cpuAccessFlags = REX::W32::D3D11_CPU_ACCESS_WRITE;
	sBufferDesc.miscFlags = REX::W32::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sBufferDesc.structureByteStride = sizeof(LayerData);
	result = pDevice->CreateBuffer(&sBufferDesc, nullptr, m_structuredBuffer.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	REX::W32::D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.format = REX::W32::DXGI_FORMAT_UNKNOWN;
	srvDesc.viewDimension = REX::W32::D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.buffer.elementWidth = 1;
	result = pDevice->CreateShaderResourceView(m_structuredBuffer.Get(), &srvDesc, m_structuredBufferView.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	// Create a texture sampler state description.
	REX::W32::D3D11_SAMPLER_DESC samplerDesc;
	samplerDesc.filter = REX::W32::D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.addressU = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.addressV = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.addressW = REX::W32::D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.mipLODBias = 0.0f;
	samplerDesc.maxAnisotropy = 1;
	samplerDesc.comparisonFunc = REX::W32::D3D11_COMPARISON_ALWAYS;
	samplerDesc.borderColor[0] = 0;
	samplerDesc.borderColor[1] = 0;
	samplerDesc.borderColor[2] = 0;
	samplerDesc.borderColor[3] = 0;
	samplerDesc.minLOD = 0;
	samplerDesc.maxLOD = REX::W32::D3D11_FLOAT32_MAX;

	// Create the texture sampler state.
	result = pDevice->CreateSamplerState(&samplerDesc, m_sampleState.GetAddressOf());
	if (FAILED(result))
	{
		return false;
	}

	return true;
}

void CDXTextureRenderer::Render(CDXD3DDevice * device, bool clear)
{
	unsigned int stride;
	unsigned int offset;

	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType);
	offset = 0;

	auto pDevice = device->GetDevice();
	auto pDeviceContext = device->GetDeviceContext();

	if (m_source.Get())
	{
		if (!UpdateConstantBuffer(device))
		{
			return;
		}

		REX::W32::D3D11_VIEWPORT viewport;
		viewport.width = (float)GetWidth();
		viewport.height = (float)GetHeight();
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.topLeftX = 0.0f;
		viewport.topLeftY = 0.0f;
		pDeviceContext->RSSetViewports(1, &viewport);

		REX::W32::ID3D11RenderTargetView* renderTarget[] = { m_renderTargetView.Get() };
		pDeviceContext->OMSetRenderTargets(1, renderTarget, nullptr);

		float blendFactor[4];
		blendFactor[0] = 0.0f;
		blendFactor[1] = 0.0f;
		blendFactor[2] = 0.0f;
		blendFactor[3] = 0.0f;
		pDeviceContext->OMSetBlendState(m_alphaEnableBlendingState.Get(), blendFactor, 0xffffffff);

		float color[4];
		color[0] = 0.0f;
		color[1] = 0.0f;
		color[2] = 0.0f;
		color[3] = 0.0f;

		if (clear)
		{
			// Clear the back buffer.
			pDeviceContext->ClearRenderTargetView(m_renderTargetView.Get(), color);
		}

		// Set the vertex buffer to active in the input assembler so it can be rendered.
		REX::W32::ID3D11Buffer* vertexBuffer[] = { m_vertexBuffer.Get() };
		pDeviceContext->IASetVertexBuffers(0, 1, vertexBuffer, &stride, &offset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		pDeviceContext->IASetIndexBuffer(m_indexBuffer.Get(), REX::W32::DXGI_FORMAT_R32_UINT, 0);

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		pDeviceContext->IASetPrimitiveTopology(REX::W32::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Set the vertex layout
		pDeviceContext->IASetInputLayout(m_layout.Get());

		// Now set the constant buffer in the vertex shader with the updated values.
		REX::W32::ID3D11Buffer* constBuffer[] = { m_constantBuffer.Get() };
		pDeviceContext->VSSetConstantBuffers(0, 1, constBuffer);

		// Set the vertex and pixel shaders that will be used to render this triangle.
		pDeviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);

		// Set the sampler state in the pixel shader.
		REX::W32::ID3D11SamplerState* samplers[] = { m_sampleState.Get() };
		pDeviceContext->PSSetSamplers(0, 1, samplers);

		// Get all the array slices and perform the same shader series on them all
		std::vector<REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>> subresources;
		if (SplitSubresources(device, m_srcDesc, m_source, subresources))
		{
			for (size_t j = 0; j < subresources.size(); ++j)
			{
				RenderShaders(device, subresources[j]);

				REX::W32::D3D11_BOX sourceRegion;
				sourceRegion.left = 0;
				sourceRegion.right = m_srcDesc.width;
				sourceRegion.top = 0;
				sourceRegion.bottom = m_srcDesc.height;
				sourceRegion.front = 0;
				sourceRegion.back = 1;

				pDeviceContext->CopySubresourceRegion(m_multiTexture.Get(), (0 + (j * 1)), 0, 0, 0, m_renderTargetTexture.Get(), (0 + (0 * 1)), &sourceRegion);
			}
		}
		else
		{
			RenderShaders(device, m_source);
		}
	}
}

void CDXTextureRenderer::RenderShaders(CDXD3DDevice * device, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> sourceView)
{
	auto pDevice = device->GetDevice();
	auto pDeviceContext = device->GetDeviceContext();

	size_t i = 0;
	REX::W32::ID3D11ShaderResourceView* source = sourceView.Get();
	do
	{
		ResourceData & resource = m_resources.at(i);
		auto shader = m_shaderCache->GetShader(device, resource.m_shader);
		if (!shader.Get()) {
			shader = m_shaderCache->GetShader(device, "normal");
		}

		REX::W32::ID3D11ShaderResourceView* resources[] = {
			source,
			resource.m_resource.Get(),
			m_structuredBufferView.Get()
		};

		UpdateStructuredBuffer(device, resource.m_metadata);

		pDeviceContext->PSSetShader(shader.Get(), nullptr, 0);
		pDeviceContext->PSSetShaderResources(0, 3, resources);
		pDeviceContext->DrawIndexed(m_indexCount, 0, 0);

		pDeviceContext->CopyResource(m_intermediateTexture.Get(), m_renderTargetTexture.Get());
		source = m_intermediateResourceView.Get();

		i++;
	} while (i < m_resources.size());
}

REX::W32::ComPtr<REX::W32::ID3D11Texture2D> CDXTextureRenderer::GetTexture()
{
	if (m_srcDesc.arraySize > 1)
	{
		return m_multiTexture;
	}
	return m_renderTargetTexture;
}

REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> CDXTextureRenderer::GetResourceView()
{
	if (m_srcDesc.arraySize > 1)
	{
		return m_multiResourceView;
	}
	return m_shaderResourceView;
}

void CDXTextureRenderer::AddLayer(const REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> & texture, const TextureType & type, const std::string & technique, const XMFLOAT4 & maskColor)
{
	m_resources.push_back({ { 0, static_cast<std::uint32_t>(type), maskColor }, texture, technique });
}

void CDXTextureRenderer::Release()
{
	m_source = nullptr;
	m_indexBuffer = nullptr;
	m_vertexBuffer = nullptr;
	m_sampleState = nullptr;
	m_constantBuffer = nullptr;
	m_layout = nullptr;
	m_vertexShader = nullptr;
	m_alphaEnableBlendingState = nullptr;
	m_renderTargetTexture = nullptr;
	m_renderTargetView = nullptr;
	m_shaderResourceView = nullptr;
	m_multiTexture = nullptr;
	m_multiResourceView = nullptr;
}

bool CDXTextureRenderer::SetTexture(CDXD3DDevice * device, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> texture, REX::W32::DXGI_FORMAT target)
{
	m_source = texture;

	if (m_source.Get())
	{
		REX::W32::ComPtr<REX::W32::ID3D11Device> pDevice = device->GetDevice();

		REX::W32::ComPtr<REX::W32::ID3D11Resource> pResource;
		m_source->GetResource(pResource.GetAddressOf());

		REX::W32::ComPtr<REX::W32::ID3D11Texture2D> pTexture;
		static constexpr REX::W32::IID iidTex2D{ 0x724608d1, 0xbc3e, 0x4bac, { 0x97, 0x64, 0x5b, 0x5f, 0x21, 0x0a, 0x5b, 0xb7 } };
		HRESULT result = pResource->QueryInterface(iidTex2D, (void**)pTexture.GetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to acquire texture from resource", __FUNCTION__);
			return false;
		}

		pTexture->GetDesc(&m_srcDesc);

		// Only regenerate the resources if the dimensions changed
		if (m_srcDesc.width != m_dstDesc.width || m_srcDesc.height != m_dstDesc.height || m_srcDesc.arraySize != m_dstDesc.arraySize)
		{
			m_renderTargetTexture = nullptr;
			m_renderTargetView = nullptr;
			m_shaderResourceView = nullptr;
			m_intermediateTexture = nullptr;
			m_intermediateResourceView = nullptr;
			m_multiTexture = nullptr;
			m_multiResourceView = nullptr;
		}
		else
		{
			return true;
		}

		ZeroMemory(&m_dstDesc, sizeof(m_dstDesc));

		// Setup the render target texture description.
		m_dstDesc.width = m_srcDesc.width;
		m_dstDesc.height = m_srcDesc.height;
		m_dstDesc.mipLevels = 1;
		m_dstDesc.arraySize = 1;
		m_dstDesc.format = target;
		m_dstDesc.sampleDesc.count = 1;
		m_dstDesc.sampleDesc.quality = 0;
		m_dstDesc.usage = REX::W32::D3D11_USAGE_DEFAULT;
		m_dstDesc.bindFlags = REX::W32::D3D11_BIND_RENDER_TARGET | REX::W32::D3D11_BIND_SHADER_RESOURCE;
		m_dstDesc.cpuAccessFlags = 0;
		m_dstDesc.miscFlags = 0;

		if (!UpdateVertexBuffer(device))
		{
			SKSE::log::error("{} - Failed to update vertex buffer", __FUNCTION__);
			return false;
		}

		// Create the render target texture.
		result = pDevice->CreateTexture2D(&m_dstDesc, NULL, m_renderTargetTexture.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to create render target texture", __FUNCTION__);
			return false;
		}

		result = pDevice->CreateTexture2D(&m_dstDesc, NULL, m_intermediateTexture.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to create temporary texture", __FUNCTION__);
			return false;
		}

		REX::W32::D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
		// Setup the description of the render target view.
		renderTargetViewDesc.format = m_dstDesc.format;
		renderTargetViewDesc.viewDimension = REX::W32::D3D11_RTV_DIMENSION_TEXTURE2D;
		renderTargetViewDesc.texture2D.mipSlice = 0;

		result = pDevice->CreateRenderTargetView(m_renderTargetTexture.Get(), &renderTargetViewDesc, m_renderTargetView.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to create render target", __FUNCTION__);
			return false;
		}

		REX::W32::D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.format = m_dstDesc.format;
		shaderResourceViewDesc.viewDimension = REX::W32::D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.texture2D.mostDetailedMip = 0;
		shaderResourceViewDesc.texture2D.mipLevels = 1;

		result = pDevice->CreateShaderResourceView(m_renderTargetTexture.Get(), &shaderResourceViewDesc, m_shaderResourceView.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to create render target resource view", __FUNCTION__);
			return false;
		}
		result = pDevice->CreateShaderResourceView(m_intermediateTexture.Get(), &shaderResourceViewDesc, m_intermediateResourceView.ReleaseAndGetAddressOf());
		if (FAILED(result))
		{
			SKSE::log::error("{} - Failed to create temporary resource view", __FUNCTION__);
			return false;
		}

		if (m_srcDesc.arraySize > 1)
		{
			if (!CreateSubresourceDestination(device, m_srcDesc, m_multiTexture, m_multiResourceView))
			{
				return false;
			}
		}

		return true;
	}
	
	return false;
}

bool CDXTextureRenderer::SplitSubresources(CDXD3DDevice * device, REX::W32::D3D11_TEXTURE2D_DESC desc, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> source, std::vector<REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>>& resources)
{
	if (desc.arraySize > 1)
	{
		auto pDevice = device->GetDevice();
		auto pContext = device->GetDeviceContext();

		REX::W32::ComPtr<REX::W32::ID3D11Resource> pResource;
		source->GetResource(pResource.GetAddressOf());

		REX::W32::D3D11_TEXTURE2D_DESC cubeMapDesc = desc;
		cubeMapDesc.mipLevels = 1;
		cubeMapDesc.arraySize = 1;
		cubeMapDesc.sampleDesc.count = 1;
		cubeMapDesc.sampleDesc.quality = 0;
		cubeMapDesc.cpuAccessFlags = 0;
		cubeMapDesc.miscFlags = 0;

		REX::W32::D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
		shaderResourceViewDesc.format = cubeMapDesc.format;
		shaderResourceViewDesc.viewDimension = REX::W32::D3D11_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.texture2D.mostDetailedMip = 0;
		shaderResourceViewDesc.texture2D.mipLevels = 1;

		for (UINT i = 0; i < desc.arraySize; ++i)
		{
			// Create the render target texture.
			REX::W32::ComPtr<REX::W32::ID3D11Texture2D> cubeSideTexture;
			HRESULT result = pDevice->CreateTexture2D(&cubeMapDesc, NULL, cubeSideTexture.ReleaseAndGetAddressOf());
			if (FAILED(result))
			{
				SKSE::log::error("{} - Failed to create cube side texture", __FUNCTION__);
				return false;
			}

			REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView> resourceView;
			result = pDevice->CreateShaderResourceView(cubeSideTexture.Get(), &shaderResourceViewDesc, resourceView.ReleaseAndGetAddressOf());
			if (FAILED(result))
			{
				SKSE::log::error("{} - Failed to create temporary cubemap resource view", __FUNCTION__);
				return false;
			}

			REX::W32::D3D11_BOX sourceRegion;
			sourceRegion.left = 0;
			sourceRegion.right = desc.width;
			sourceRegion.top = 0;
			sourceRegion.bottom = desc.height;
			sourceRegion.front = 0;
			sourceRegion.back = 1;

			pContext->CopySubresourceRegion(cubeSideTexture.Get(), (0 + (0 * 1)), 0, 0, 0, pResource.Get(), ((0 + (i * desc.mipLevels))), &sourceRegion);

			resources.push_back(resourceView);
		}

		return true;
	}

	return false;
}

bool CDXTextureRenderer::CreateSubresourceDestination(CDXD3DDevice * device, REX::W32::D3D11_TEXTURE2D_DESC desc, REX::W32::ComPtr<REX::W32::ID3D11Texture2D>& outTexture, REX::W32::ComPtr<REX::W32::ID3D11ShaderResourceView>& outResource)
{
	auto pDevice = device->GetDevice();
	auto pContext = device->GetDeviceContext();

	REX::W32::D3D11_TEXTURE2D_DESC cubeMapDesc = desc;
	cubeMapDesc.format = m_dstDesc.format;
	cubeMapDesc.mipLevels = 1;
	cubeMapDesc.arraySize = desc.arraySize;
	cubeMapDesc.sampleDesc.count = 1;
	cubeMapDesc.sampleDesc.quality = 0;
	cubeMapDesc.cpuAccessFlags = 0;
	cubeMapDesc.miscFlags = (desc.miscFlags & REX::W32::D3D11_RESOURCE_MISC_TEXTURECUBE) ? REX::W32::D3D11_RESOURCE_MISC_TEXTURECUBE : 0;

	// Create the render target texture.
	HRESULT result = pDevice->CreateTexture2D(&cubeMapDesc, NULL, outTexture.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create merged texture", __FUNCTION__);
		return false;
	}

	REX::W32::D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	shaderResourceViewDesc.format = cubeMapDesc.format;
	shaderResourceViewDesc.viewDimension = (desc.miscFlags & REX::W32::D3D11_RESOURCE_MISC_TEXTURECUBE) ? REX::W32::D3D11_SRV_DIMENSION_TEXTURECUBE : REX::W32::D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.texture2D.mostDetailedMip = 0;
	shaderResourceViewDesc.texture2D.mipLevels = 1;

	result = pDevice->CreateShaderResourceView(outTexture.Get(), &shaderResourceViewDesc, outResource.ReleaseAndGetAddressOf());
	if (FAILED(result))
	{
		SKSE::log::error("{} - Failed to create cubemap resource view", __FUNCTION__);
		return false;
	}

	return true;
}

bool CDXTextureRenderer::UpdateVertexBuffer(CDXD3DDevice * device)
{
	if (!m_vertexBuffer.Get())
	{
		return false;
	}
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> pDeviceContext = device->GetDeviceContext();

	HRESULT result;
	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;

	float left, right, top, bottom;
	std::unique_ptr<VertexType[]> vertices;
	VertexType* verticesPtr;

	// Calculate the screen coordinates of the left side of the bitmap.
	left = (float)((GetWidth() / 2) * -1);

	// Calculate the screen coordinates of the right side of the bitmap.
	right = left + (float)GetWidth();

	// Calculate the screen coordinates of the top of the bitmap.
	top = (float)(GetHeight() / 2);

	// Calculate the screen coordinates of the bottom of the bitmap.
	bottom = top - (float)GetHeight();

	// Create the vertex array.
	vertices = std::make_unique<VertexType[]>(m_vertexCount);
	if (!vertices)
	{
		return false;
	}

	// Load the vertex array with data.
	// First triangle.
	vertices[0].position = XMFLOAT3(left, top, 0.0f);  // Top left.
	vertices[0].texture = XMFLOAT2(0.0f, 0.0f);

	vertices[1].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	vertices[1].texture = XMFLOAT2(1.0f, 1.0f);

	vertices[2].position = XMFLOAT3(left, bottom, 0.0f);  // Bottom left.
	vertices[2].texture = XMFLOAT2(0.0f, 1.0f);

	// Second triangle.
	vertices[3].position = XMFLOAT3(left, top, 0.0f);  // Top left.
	vertices[3].texture = XMFLOAT2(0.0f, 0.0f);

	vertices[4].position = XMFLOAT3(right, top, 0.0f);  // Top right.
	vertices[4].texture = XMFLOAT2(1.0f, 0.0f);

	vertices[5].position = XMFLOAT3(right, bottom, 0.0f);  // Bottom right.
	vertices[5].texture = XMFLOAT2(1.0f, 1.0f);

	// Lock the vertex buffer so it can be written to.
	result = pDeviceContext->Map(m_vertexBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the vertex buffer.
	verticesPtr = (VertexType*)mappedResource.data;

	// Copy the data into the vertex buffer.
	memcpy(verticesPtr, (void*)vertices.get(), (sizeof(VertexType) * m_vertexCount));

	// Unlock the vertex buffer.
	pDeviceContext->Unmap(m_vertexBuffer.Get(), 0);

	return true;
}

bool CDXTextureRenderer::UpdateConstantBuffer(CDXD3DDevice * device)
{
	if (!m_constantBuffer.Get())
	{
		return false;
	}
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> pDeviceContext = device->GetDeviceContext();

	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = pDeviceContext->Map(m_constantBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the vertex buffer.
	ConstantBufferType * dataPtr = (ConstantBufferType*)mappedResource.data;
	dataPtr->world = XMMatrixTranspose(XMMatrixIdentity());
	dataPtr->view = XMMatrixTranspose(XMMatrixIdentity());
	XMMATRIX projectionMatrix = XMMatrixOrthographicLH(static_cast<float>(GetWidth()), static_cast<float>(GetHeight()), 1000.0f, 0.1f);
	projectionMatrix.r[3].m128_f32[3] = 1.0001f; // Fudge the W argument to make it show
	dataPtr->projection = XMMatrixTranspose(projectionMatrix);

	// Unlock the vertex buffer.
	pDeviceContext->Unmap(m_constantBuffer.Get(), 0);

	return true;
}

bool CDXTextureRenderer::UpdateStructuredBuffer(CDXD3DDevice * device, const LayerData & layerData)
{
	if (!m_structuredBuffer.Get())
	{
		return false;
	}
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> pDeviceContext = device->GetDeviceContext();
	REX::W32::D3D11_MAPPED_SUBRESOURCE mappedResource;
	HRESULT result = pDeviceContext->Map(m_structuredBuffer.Get(), 0, REX::W32::D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
	if (FAILED(result))
	{
		return false;
	}

	// Get a pointer to the data in the vertex buffer.
	LayerData * dataPtr = (LayerData*)mappedResource.data;
	memcpy(dataPtr, &layerData, sizeof(LayerData));

	// Unlock the vertex buffer.
	pDeviceContext->Unmap(m_structuredBuffer.Get(), 0);

	return true;
}