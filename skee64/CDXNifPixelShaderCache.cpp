#include "CDXNifPixelShaderCache.h"
#include "CDXD3DDevice.h"
#include "CDXBSShaderResource.h"

#include "FileUtils.h"

#include <stdio.h>



REX::W32::ComPtr<REX::W32::ID3D11PixelShader> CDXNifPixelShaderCache::GetShader(CDXD3DDevice* device, const std::string & name)
{
	std::string xform = name;
	std::transform(xform.begin(), xform.end(), xform.begin(), ::tolower);

	REX::W32::ComPtr<REX::W32::ID3D11PixelShader> shader = __super::GetShader(device, name);
	if (shader.Get())
	{
		return shader;
	}

	char shaderPath[REX::W32::MAX_PATH];
	sprintf_s(shaderPath, REX::W32::MAX_PATH, "SKSE/Plugins/NiOverride/Shaders/Effects/%s.fx", name.c_str());

	CDXBSShaderResource shaderFile(shaderPath, name.c_str());
	sprintf_s(shaderPath, REX::W32::MAX_PATH, "SKSE/Plugins/NiOverride/Shaders/Compiled/Effects/%s.cso", name.c_str());
	CDXBSShaderResource compiledFile(shaderPath, name.c_str());

	if (m_factory->CreatePixelShader(device, &shaderFile, &compiledFile, shader))
	{
#ifndef _DEBUG
		insert_or_assign(name, shader);
#endif
		return shader;
	}

	return nullptr;
}
