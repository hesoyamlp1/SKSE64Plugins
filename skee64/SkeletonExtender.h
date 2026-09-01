#pragma once
#include <RE/N/NiAVObject.h>
#include <RE/N/NiNode.h>
#include <RE/T/TESObjectARMA.h>
#include <RE/T/TESObjectARMO.h>
#include <RE/T/TESObjectREFR.h>

#include <set>
#include "StringTable.h"
#include "IPluginInterface.h"

class SkeletonExtenderInterface : public IAddonAttachmentInterface
{
public:
	void Attach(RE::TESObjectREFR * refr, RE::NiNode * skeleton, RE::NiAVObject * objectRoot);
	RE::NiNode * LoadTemplate(RE::NiNode * parent, const char * path);
	void AddTransforms(RE::TESObjectREFR * refr, bool isFirstPerson, RE::NiNode * skeleton, RE::NiAVObject * objectRoot);
	void ReadTransforms(RE::TESObjectREFR * refr, const char * jsonData, bool isFirstPerson, bool isFemale, std::set<SKEEFixedString> & nodes, std::set<SKEEFixedString> & changes);

	// Inherited via IAddonAttachmentInterface
	virtual void OnAttach(RE::TESObjectREFR * refr, RE::TESObjectARMO * armor, RE::TESObjectARMA * addon, RE::NiAVObject * object, bool isFirstPerson, RE::NiNode * skeleton, RE::NiNode * root) override;
};