#include "CDXPixelShaderCache.h"
#include "CDXD3DDevice.h"
#include "CDXShaderCompile.h"



REX::W32::ComPtr<REX::W32::ID3D11PixelShader> CDXPixelShaderCache::GetShader(CDXD3DDevice * device, const std::string & name)
{
	auto it = find(name);
	if (it != end())
	{
		return it->second;
	}

	return nullptr;
}

