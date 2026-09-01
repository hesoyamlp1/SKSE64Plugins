#ifndef __CDXNIFSCENE__
#define __CDXNIFSCENE__

#pragma once

#include "CDXEditableScene.h"
#include "CDXRenderState.h"
#include <RE/B/BSScaleformImageLoader.h>
#include <RE/B/BSShaderRenderTargets.h>
#include <RE/N/NiNode.h>
#include <RE/N/NiSmartPointer.h>
#include <RE/N/NiTexture.h>

#include "REX/W32/COMPTR.h"
#include <cstdint>
#include <RE/A/Actor.h>



class CDXD3DDevice;
class CDXNifScene : public CDXEditableScene, public CDXRenderState
{
public:
	CDXNifScene();

	virtual bool Setup(const CDXInitParams & initParams) override;
	virtual void Release() override;
	virtual void CreateBrushes() override;

	virtual void Begin(CDXCamera * camera, CDXD3DDevice * device) override;
	virtual void End(CDXCamera * camera, CDXD3DDevice * device) override;

	bool CreateRenderTarget(CDXD3DDevice * device, std::uint32_t width, std::uint32_t height);

	REX::W32::ComPtr<REX::W32::ID3D11RenderTargetView> GetRenderTargetView() { return m_renderTargetView; }
	RE::NiTexture* GetTexture() { return m_renderTexture.get(); }

	void SetWorkingActor(RE::Actor * actor) { m_actor = actor; }
	RE::Actor* GetWorkingActor() { return m_actor; }

	void SetImportRoot(RE::NiNode * node) { m_importRoot = RE::NiPointer<RE::NiNode>(node); }
	RE::NiNode * GetImportRoot() { return m_importRoot.get(); }

	void ReleaseImport();

protected:
	RE::Actor*						m_actor;
	RE::NiPointer<RE::NiNode>			m_importRoot;
	REX::W32::ComPtr<REX::W32::ID3D11RenderTargetView>		m_renderTargetView;
	RE::NiPointer<RE::NiTexture>		m_renderTexture;

	REX::W32::ComPtr<REX::W32::ID3D11Texture2D>			m_depthStencilBuffer;
	REX::W32::ComPtr<REX::W32::ID3D11DepthStencilState>	m_depthStencilState;
	REX::W32::ComPtr<REX::W32::ID3D11DepthStencilView>	m_depthStencilView;
};

#endif
