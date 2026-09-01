#pragma once

#include "REX/W32/COMPTR.h"
#include "REX/W32/D3D11.h"



class CDXD3DDevice
{
public:
	CDXD3DDevice() : m_pDevice(nullptr), m_pDeviceContext(nullptr) { }
	CDXD3DDevice(const REX::W32::ComPtr<REX::W32::ID3D11Device> & pDevice, const REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> & pDeviceContext) : m_pDevice(pDevice), m_pDeviceContext(pDeviceContext){}

	void SetDevice(const REX::W32::ComPtr<REX::W32::ID3D11Device> & d) { m_pDevice = d; }
	void setDeviceContext(const REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> & d) { m_pDeviceContext = d; }

	REX::W32::ComPtr<REX::W32::ID3D11Device> GetDevice() { return m_pDevice; }
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> GetDeviceContext() { return m_pDeviceContext; }

protected:
	REX::W32::ComPtr<REX::W32::ID3D11Device> m_pDevice;
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> m_pDeviceContext;
};