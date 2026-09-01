#pragma once

#include <unordered_map>
#include <string>
#include "REX/W32/COMPTR.h"
#include "REX/W32/D3D11.h"
#include <sstream>

#include "CDXTypes.h"



class CDXD3DDevice;
class CDXShaderFactory;

class CDXPixelShaderCache : protected std::unordered_map<std::string, REX::W32::ComPtr<REX::W32::ID3D11PixelShader>>
{
public:
	explicit CDXPixelShaderCache(CDXShaderFactory * factory) : m_factory(factory) { }

	virtual REX::W32::ComPtr<REX::W32::ID3D11PixelShader> GetShader(CDXD3DDevice* device, const std::string & name);

protected:
	CDXShaderFactory * m_factory;
};