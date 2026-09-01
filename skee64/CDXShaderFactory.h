#pragma once

#include "REX/W32/D3D11.h"
#include <DirectXMath.h>
#include "REX/W32/COMPTR.h"



class CDXD3DDevice;

class CDXShaderFile
{
public:
	virtual void* GetBuffer() const = 0;
	virtual size_t GetBufferSize() const = 0;
	virtual const char* GetSourceName() const = 0;
	virtual const char* GetEntryPoint() const = 0;
};

class CDXShaderFactory
{
public:
	virtual bool CreateVertexShader(CDXD3DDevice * device, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile, REX::W32::D3D11_INPUT_ELEMENT_DESC * polygonLayout, int numElements, REX::W32::ComPtr<REX::W32::ID3D11VertexShader> & vertexShader, REX::W32::ComPtr<REX::W32::ID3D11InputLayout> & layout);
	virtual bool CreatePixelShader(CDXD3DDevice * device, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile, REX::W32::ComPtr<REX::W32::ID3D11PixelShader> & pixelShader);
	virtual bool CreateGeometryShader(CDXD3DDevice * device, CDXShaderFile * sourceFile, CDXShaderFile * precompiledFile, REX::W32::ComPtr<REX::W32::ID3D11GeometryShader> & geometryShader);

private:
	void OutputShaderErrorMessage(REX::W32::ComPtr<REX::W32::ID3DBlob> & errorMessage, std::stringstream & output);
};