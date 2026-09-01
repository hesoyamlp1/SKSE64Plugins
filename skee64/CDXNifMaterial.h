#pragma once

#include "CDXMaterial.h"

#include <RE/N/NiSmartPointer.h>
#include <RE/N/NiTexture.h>

class CDXNifMaterial : public CDXMaterial
{
public:
	// Holds a reference to the RE::NiTexture to keep it from being destroyed
	void SetNiTexture(int index, RE::NiTexture* texture);
	
private:
	RE::NiPointer<RE::NiTexture> m_pTextures[5];
};