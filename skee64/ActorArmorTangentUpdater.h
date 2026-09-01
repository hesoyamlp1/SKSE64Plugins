#pragma once

#include "IPluginInterface.h"

class ActorArmorTangentUpdater : public IAddonAttachmentInterface
{
public:
	// Inherited via IAddonAttachmentInterface
	virtual void OnAttach(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::NiAVObject* object, bool isFirstPerson, RE::NiNode* skeleton, RE::NiNode* root) override;
};