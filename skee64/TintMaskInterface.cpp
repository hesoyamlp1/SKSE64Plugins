#include "TintMaskInterface.h"
#include "SKEEHooks.h"
#include "ItemDataInterface.h"
#include "ShaderUtilities.h"
#include "NifUtils.h"
#include "FileUtils.h"



#include "RE/N/NiGeometry.h"
#include "RE/N/NiExtraData.h"
#include "RE/N/NiRTTI.h"
#include "RE/B/BSResourceNiBinaryStream.h"


#include "tinyxml2.h"

#include "CDXD3DDevice.h"
#include "CDXNifTextureRenderer.h"
#include "CDXNifPixelShaderCache.h"
#include "CDXShaderFactory.h"

#include <vector>
#include <algorithm>
#include <cstdint>



CDXShaderFactory			g_shaderFactory;
CDXNifPixelShaderCache		g_pixelShaders(&g_shaderFactory);
extern TintMaskInterface	g_tintMaskInterface;
extern std::uint32_t	g_tintHairSlot;

skee_u32 TintMaskInterface::GetVersion()
{
	return kCurrentPluginVersion;
}

void TintMaskInterface::CreateTintsFromData(RE::TESObjectREFR * refr, std::map<std::int32_t, CDXNifTextureRenderer::MaskData> & masks, const LayerTarget & layerTarget, ItemAttributeDataPtr & overrides, std::uint32_t & flags)
{
	std::uint32_t skinColor = 0;
	std::uint32_t hairColor = 0;

	if (refr->GetBaseObject() && refr->GetBaseObject()->Is(RE::FormType::NPC))
	{
		RE::TESNPC* actorBase = static_cast<RE::TESNPC*>(refr->GetBaseObject());
		skinColor = ((std::uint32_t)actorBase->bodyTintColor.red << 16) | ((std::uint32_t)actorBase->bodyTintColor.green << 8) | (std::uint32_t)actorBase->bodyTintColor.blue;

		auto headData = actorBase->headRelatedData;
		if (headData) {
			auto hairColorForm = headData->hairColor;
			if (hairColorForm) {
				// Dunno why the hell they multiplied hairColor by 2
				hairColor |= std::min((std::int32_t)hairColorForm->color.red * 2, 255) << 16;
				hairColor |= std::min((std::int32_t)hairColorForm->color.green * 2, 255) << 8;
				hairColor |= std::min((std::int32_t)hairColorForm->color.blue * 2, 255);
			}
		}
	}

	for (auto base : layerTarget.textureData) {
		masks[base.first].texture = base.second;
	}
	for (auto base : layerTarget.colorData) {
		if (base.second == kPreset_Skin)
		{
			masks[base.first].color = skinColor;
			flags |= kUpdate_Skin;
		}
		else if (base.second == kPreset_Hair)
		{
			masks[base.first].color = hairColor;
			flags |= kUpdate_Hair;
		}
		else
		{
			masks[base.first].color = base.second;
		}
	}
	for (auto base : layerTarget.blendData) {
		masks[base.first].technique = base.second;
	}
	for (auto base : layerTarget.typeData) {
		masks[base.first].textureType = static_cast<CDXTextureRenderer::TextureType>(base.second);
	}
	for (auto base : layerTarget.alphaData) {
		masks[base.first].color |= (std::uint32_t)(base.second * 255) << 24;
	}

	if (overrides)
	{
		overrides->GetLayer(layerTarget.targetIndex, [&](auto layerData)
		{
			for (auto base : layerData.m_textureMap) {
				auto it = masks.find(base.first);
					if (it != masks.end()) {
						masks[base.first].texture = *base.second;
					}
			}

			if (layerTarget.slots.empty())
			{
				for (auto base : layerData.m_colorMap) {
					auto it = masks.find(base.first);
					if (it != masks.end()) {
						masks[base.first].color = base.second;
					}
				}
			}
			else
			{
				for (auto base : layerData.m_colorMap) {
					auto it = layerTarget.slots.equal_range(base.first);
					for (auto itr = it.first; itr != it.second; ++itr) {
						auto it = masks.find(itr->second);
						if (it != masks.end()) {
							masks[itr->second].color = base.second;
						}
					}
				}
			}

			for (auto base : layerData.m_blendMap) {
				auto it = masks.find(base.first);
				if (it != masks.end()) {
					masks[base.first].technique = *base.second;
				}
			}
			for (auto base : layerData.m_typeMap) {
				auto it = masks.find(base.first);
				if (it != masks.end()) {
					masks[base.first].textureType = static_cast<CDXTextureRenderer::TextureType>(base.second);
				}
			}
		});
	}
}

NIOVTaskDeferredMask::NIOVTaskDeferredMask(RE::TESObjectREFR * refr, bool isFirstPerson, RE::TESObjectARMO * armor, RE::TESObjectARMA * addon, RE::NiAVObject * object, ItemAttributeDataPtr overrides)
{
	m_firstPerson = isFirstPerson;
	m_formId = refr->formID;
	m_armorId = armor ? armor->formID : 0;
	m_addonId = addon ? addon->formID : 0;
	m_object = RE::NiPointer<RE::NiAVObject>(object);
	m_overrides = overrides;
}

void NIOVTaskDeferredMask::Dispose()
{
	delete this;
}

void NIOVTaskDeferredMask::Run()
{
	RE::TESForm * refrForm = RE::TESForm::LookupByID(m_formId);
	RE::TESForm * armorForm = RE::TESForm::LookupByID(m_armorId);
	RE::TESForm * addonForm = m_addonId ? RE::TESForm::LookupByID(m_addonId) : nullptr;
	if (refrForm && armorForm) {
		RE::TESObjectREFR * refr = refrForm ? refrForm->As<RE::TESObjectREFR>() : nullptr;
		RE::TESObjectARMO * armor = armorForm ? armorForm->As<RE::TESObjectARMO>() : nullptr;
		RE::TESObjectARMA * addon = addonForm ? addonForm->As<RE::TESObjectARMA>() : nullptr;

		if (refr && armor) {
			g_tintMaskInterface.ApplyMasks(refr, m_firstPerson, armor, addon, m_object.get(), TintMaskInterface::kUpdate_All, m_overrides);
		}
	}
}

void TintMaskInterface::ApplyMasks(RE::TESObjectREFR * refr, bool isFirstPerson, RE::TESObjectARMO * armor, RE::TESObjectARMA * addon, 
	RE::NiAVObject * rootNode, std::uint32_t flags, ItemAttributeDataPtr overrides, LayerFunctor layerFunctor)
{
	LayerTargetList layerList;
	VisitObjects(rootNode, [&](RE::NiAVObject* object)
	{
		LayerTarget mask;
		mask.targetIndex = 0; // Target the diffuse by default
		if (mask.object = RE::NiPointer<RE::BSGeometry>(object ? object->AsGeometry() : nullptr)) {
			auto textureData = netimmerse_cast<RE::NiStringsExtraData*>(object->GetExtraData("MASKT"));
			if (textureData) {
				for (std::int32_t i = 0; i < textureData->size; ++i)
				{
					mask.textureData[i] = textureData->value[i];
				}
				
			}
			auto colorData = netimmerse_cast<RE::NiIntegersExtraData*>(object->GetExtraData("MASKC"));
			if (colorData) {
				for (std::int32_t i = 0; i < colorData->size && i < colorData->size; ++i)
				{
					mask.colorData[i] = colorData->value[i];
				}
			}

			auto alphaData = netimmerse_cast<RE::NiFloatsExtraData*>(object->GetExtraData("MASKA"));
			if (alphaData) {
				for (std::int32_t i = 0; i < alphaData->size && i < alphaData->size; ++i)
				{
					mask.alphaData[i] = alphaData->value[i];
				}
			}

			if (mask.object && mask.textureData.size() > 0)
				layerList.push_back(mask);
		}

		return false;
	});

	m_modelMap.ApplyLayers(refr, isFirstPerson, armor, addon, rootNode, [&](RE::NiPointer<RE::BSGeometry> geom, std::int32_t targetIndex, TextureLayer* layer)
	{
		LayerTarget obj;
		obj.object = geom;
		obj.targetIndex = targetIndex;
		obj.targetFlags = 0;
		obj.textureData = layer->textures;
		obj.colorData = layer->colors;
		obj.alphaData = layer->alphas;
		obj.blendData = layer->blendModes;
		obj.typeData = layer->types;
		obj.slots = layer->slots;
		layerList.push_back(obj);
	});

	std::shared_ptr<ItemAttributeData> itemOverrideData;
	if (overrides && !layerList.empty()) {
		itemOverrideData = overrides;
	}

	std::unique_ptr<CDXD3DDevice> device;
	// Renderer runtime data exposes the context as a raw pointer; the ComPtr ctor takes an owning ref.
	REX::W32::ComPtr<REX::W32::ID3D11DeviceContext> pDeviceContext(RE::BSGraphics::Renderer::GetSingleton()->GetRuntimeData().context);
	REX::W32::ComPtr<REX::W32::ID3D11Device> pDevice;
	pDeviceContext->GetDevice(pDevice.GetAddressOf());
	if (!pDevice.Get()) {
		return;
	}
	device = std::make_unique<CDXD3DDevice>(pDevice, pDeviceContext);

	int i = 0;
	for (auto & layer : layerList)
	{
		RE::NiPointer<RE::BSShaderProperty> shaderProperty = layer.object->GetGeometryRuntimeData().shaderProperty;
		if(shaderProperty) {
			RE::BSLightingShaderProperty* lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProperty.get());
			if(lightingShader) {
				RE::BSLightingShaderMaterial * material = (RE::BSLightingShaderMaterial *)lightingShader->material;

				std::uint32_t changedFlags = 0;
				std::map<std::int32_t, CDXNifTextureRenderer::MaskData> tintMasks;
				CreateTintsFromData(refr, tintMasks, layer, itemOverrideData, changedFlags);

				// Must have selective update
				if (flags != kUpdate_All && (changedFlags & flags) == 0) {
					continue;
				}

				const char * texturePath = nullptr;
				RE::NiPointer<RE::NiSourceTexture> sourceTexture;
				if (material->textureSet)
				{
					texturePath = material->textureSet->GetTexturePath(static_cast<RE::BSTextureSet::Texture>(layer.targetIndex));
					if (!texturePath) {
						continue;
					}

					RE::NiPointer<RE::NiTexture> texture;
					RE::BSShaderManager::GetTexture(texturePath, 1, texture, false);
                    sourceTexture.reset(static_cast<RE::NiSourceTexture*>(texture.get()));
				}

				// No source texture at this path, bad texture
				if (!sourceTexture) {
					continue;
				}

				std::shared_ptr<CDXNifTextureRenderer> renderer;
				if (m_maskMap.IsCaching())
				{
					renderer = m_maskMap.GetRenderTarget(lightingShader, layer.targetIndex);
				}
				if(!renderer)
				{
					renderer = std::make_shared<CDXNifTextureRenderer>();
					if (!renderer->Init(device.get(), &g_pixelShaders))
					{
						continue;
					}

					if (renderer && m_maskMap.IsCaching()) {
						m_maskMap.AddRenderTargetGroup(lightingShader, layer.targetIndex, renderer);
					}
				}

				char path[512];
				_snprintf_s(path, 512, "RT [%08X][%08X][%08X](%s)", refr->formID, armor->formID, addon ? addon->formID : 0, layer.object->name.c_str());

				RE::NiPointer<RE::NiSourceTexture> output;
				if (renderer->ApplyMasksToTexture(device.get(), sourceTexture, tintMasks, path, output))
				{
					RE::BSShaderMaterial* newMaterial = material->Create();
					newMaterial->CopyMembers(material);

					// If the target material has HairTint we should neutralize the color and block it from receiving tint updates
					// But only if theres a layer in the XML that says to do this, this is still missing TODO
					if (newMaterial->GetFeature() == RE::BSShaderMaterial::Feature::kHairTint)
					{
						RE::BSLightingShaderMaterialHairTint* hairMaterial = static_cast<RE::BSLightingShaderMaterialHairTint*>(newMaterial);
						hairMaterial->tintColor.red = 0.5;
						hairMaterial->tintColor.green = 0.5;
						hairMaterial->tintColor.blue = 0.5;

						RE::NiExtraData* extraData = lightingShader->GetExtraData("NO_TINT");
						if (!extraData) {
							extraData = RE::NiBooleanExtraData::Create("NO_TINT", true);
							lightingShader->AddExtraData(extraData);
						}
					}

					auto targetTexture = GetTextureFromIndex(static_cast<RE::BSLightingShaderMaterial*>(newMaterial), layer.targetIndex);
					if (targetTexture) {
						targetTexture->reset(static_cast<RE::NiSourceTexture*>(output.get()));
					}

					if (layerFunctor) {
						layerFunctor(armor, addon, texturePath, output, layer);
					}

#if DUMP_TEXTURE
					char texturePath[REX::W32::MAX_PATH];
					_snprintf_s(texturePath, REX::W32::MAX_PATH, "Layer_%d_%s.dds", i++, layer.object->name);
					int len = strlen(texturePath);
					for (int i = 0; i < len; ++i)
					{
						if (isalpha(texturePath[i]) || isdigit(texturePath[i]) || texturePath[i] == '.' || texturePath[i] == '_')
							continue;
						texturePath[i] = '_';
					}
					SaveRenderedDDS(output.get(), texturePath);
#endif

					lightingShader->SetMaterial(newMaterial, 1); // This creates a new material from the one we created above, so we destroy it after
					SKEE::InitializeShader(lightingShader, layer.object.get());
					
					newMaterial->~BSShaderMaterial();
					RE::free(newMaterial);
				}

#if 0
					
				NiTexture * newTarget = NULL;
				if (m_maskMap.IsCaching())
					newTarget = m_maskMap.GetRenderTarget(lightingShader);
				if (!newTarget) {
					std::uint32_t width = 0;
					std::uint32_t height = 0;
					if (mask.resolutionWData)
						width = mask.resolutionWData;
					if (mask.resolutionHData)
						height = mask.resolutionHData;
					else
						height = mask.resolutionWData;

					newTarget = SKEE::CreateSourceTexture("TintMask");
					newTarget->rendererData = RE::BSGraphics::Renderer::GetSingleton()->CreateRenderTexture(width, height);

					if (newTarget && m_maskMap.IsCaching()) {
						m_maskMap.AddRenderTargetGroup(lightingShader, newTarget);
					}
				}
				if(newTarget) {
					RE::BSTArray<TintMask*> tintMasks;
					CreateTintsFromData(tintMasks, mask.layerCount, mask.textureData, mask.colorData, mask.alphaData, overrideMap);

					newTarget->IncRef();
					if (ApplyMasksToRenderTarget(&tintMasks, newTarget)) {
						BSLightingShaderMaterialFacegen * tintedMaterial = static_cast<BSLightingShaderMaterialFacegen*>(CreateShaderMaterial(BSLightingShaderMaterialFacegen::kShaderType_FaceGen));
						tintedMaterial->CopyMembers(material);
						tintedMaterial->renderedTexture = newTarget;
						lightingShader->SetFlags(static_cast<RE::BSShaderProperty::EShaderPropertyFlag8>(0x0A), true); // Enable detailmap
						lightingShader->SetFlags(static_cast<RE::BSShaderProperty::EShaderPropertyFlag8>(0x15), false); // Disable FaceGen_RGB
						//material->ReleaseTextures();
						lightingShader->SetMaterial(tintedMaterial, 1); // New material takes texture ownership
						if (newTarget) // Let the material now take ownership since the old target is destroyed now
							newTarget->DecRef();
						SKEE::InitializeShader(lightingShader, mask.object.get());
					}

					newTarget->DecRef();

					ReleaseTintsFromData(tintMasks);
				}

#endif
			}
		}
	}
}

void TintMaskMap::ManageRenderTargetGroups()
{
	std::lock_guard<std::recursive_mutex> locker(m_lock);
	m_caching = true;
}

std::shared_ptr<CDXNifTextureRenderer> TintMaskMap::GetRenderTarget(RE::BSLightingShaderProperty* key, std::int32_t index)
{
	auto it = m_data.find(RE::NiPointer<RE::BSLightingShaderProperty>(key));
	if (it != m_data.end()) {
		auto idx = it->second.find(index);
		if (idx != it->second.end()) {
			return idx->second;
		}
	}

	return nullptr;
}

void TintMaskMap::AddRenderTargetGroup(RE::BSLightingShaderProperty* key, std::int32_t index, std::shared_ptr<CDXNifTextureRenderer> value)
{
	std::lock_guard<std::recursive_mutex> locker(m_lock);
	m_data[RE::NiPointer<RE::BSLightingShaderProperty>(key)][index] = value;
}

void TintMaskMap::ReleaseRenderTargetGroups()
{
	std::lock_guard<std::recursive_mutex> locker(m_lock);
	m_data.clear();
	m_caching = false;
}

TextureLayer * TextureLayerMap::GetTextureLayer(SKEEFixedString texture)
{
	auto it = find(texture);
	if (it != end()) {
		return &it->second;
	}

	return nullptr;
}

TextureLayerMap * MaskTriShapeMap::GetTextureMap(SKEEFixedString triShape)
{
	auto it = find(triShape);
	if (it != end()) {
		return &it->second;
	}

	return nullptr;
}

TextureLayer * MaskModelMap::GetMask(SKEEFixedString nif, SKEEFixedString trishape, SKEEFixedString texture)
{
	return &m_data[nif][trishape][GetSanitizedPath(texture)];
}

MaskTriShapeMap * MaskModelMap::GetTriShapeMap(SKEEFixedString nifPath)
{
	auto it = m_data.find(nifPath);
	if (it != m_data.end()) {
		return &it->second;
	}

	return nullptr;
}

bool ApplyMaskData(MaskTriShapeMap * triShapeMap, RE::NiAVObject * object, const char * nameOverride, std::function<void(RE::NiPointer<RE::BSGeometry>, std::int32_t, TextureLayer*)> functor)
{
	RE::NiPointer<RE::BSGeometry> geometry{object ? object->AsGeometry() : nullptr};
	if (!geometry) {
		return false;
	}

	auto textureMap = triShapeMap->GetTextureMap(nameOverride ? nameOverride : object->name);
	if (!textureMap) {
		return false;
	}

	RE::BSShaderProperty* shaderProperty = geometry->GetGeometryRuntimeData().shaderProperty.get();
	if (!shaderProperty) {
		return false;
	}

	auto lightingShader = netimmerse_cast<RE::BSLightingShaderProperty*>(shaderProperty);
	if (!lightingShader) {
		return false;
	}

	auto material = static_cast<RE::BSLightingShaderMaterial*>(lightingShader->material);
	if (!material) {
		return false;
	}

	auto textureSet = material->textureSet;
	if (!textureSet) {
		return false;
	}

	// Pass over all the textures to match with those that exist in the mapping
	for (std::uint32_t i = 0; i < 8; ++i)
	{
		const char * texture = textureSet->GetTexturePath(static_cast<RE::BSTextureSet::Texture>(i));
		if (!texture)
			continue;

		auto it = textureMap->find(GetSanitizedPath(texture));
		if (it == textureMap->end())
		{
			char buff[std::numeric_limits<std::int32_t>::digits10 + 1];
			sprintf_s(buff, "%d", i);
			it = textureMap->find(buff);
		}
		if (it == textureMap->end())
		{
			continue;
		}

		functor(geometry, i, &it->second);
	}

	return true;
}

SKEEFixedString MaskModelMap::GetModelPath(std::uint8_t gender, bool isFirstPerson, RE::TESObjectARMO * armor, RE::TESObjectARMA * arma)
{
	SKEEFixedString modelPath;
	if (arma) {
		modelPath = (isFirstPerson ? arma->bipedModel1stPersons[gender] : arma->bipedModels[gender]).GetModel();
		if (isFirstPerson && modelPath.length() == 0) { // If first person was not found, try third person
			modelPath = arma->bipedModels[gender].GetModel();
			if (modelPath.length() == 0) { // If gender not found, try male
				modelPath = arma->bipedModels[0].GetModel();
			}
		}
	}
	else if (armor) {
		modelPath = armor->worldModels[gender].GetModel();
		if (modelPath.length() == 0) { // If gender not found, try male
			modelPath = armor->worldModels[0].GetModel();
		}
	}

	return modelPath;
}

void MaskModelMap::ApplyLayers(RE::TESObjectREFR * refr, bool isFirstPerson, RE::TESObjectARMO * armor, RE::TESObjectARMA * arma, RE::NiAVObject * node, std::function<void(RE::NiPointer<RE::BSGeometry>, std::int32_t, TextureLayer*)> functor)
{
	std::uint8_t gender = 0;
	RE::TESNPC * actorBase = refr->GetBaseObject() ? refr->GetBaseObject()->As<RE::TESNPC>() : nullptr;
	if (actorBase)
		gender = actorBase->GetSex();

	std::lock_guard<std::recursive_mutex> locker(m_lock);

	// Special case if the addon has no 1p model, use the 3p model
	SKEEFixedString modelPath = GetModelPath(gender, isFirstPerson, armor, arma);
	auto triShapeMap = GetTriShapeMap(modelPath);
	if (!triShapeMap) {
		return;
	}

	std::uint32_t count = 0;
	VisitObjects(node, [&](RE::NiAVObject* object)
	{
		if (ApplyMaskData(triShapeMap, object, nullptr, functor))
			count++;

		return false;
	});

	if (count == 0)
		ApplyMaskData(triShapeMap, node, "", functor);
}

bool TintMaskInterface::IsDyeable(RE::TESObjectARMO * armor)
{
	std::lock_guard<std::recursive_mutex> locker(m_dyeableLock);

	auto it = m_dyeable.find(armor->formID);
	if (it != m_dyeable.end())
	{
		return it->second;
	}
	else
	{
		// This could be expensive so lets just cache items we've seen
		for (std::uint8_t g = 0; g <= 1; ++g)
		{
			for (std::uint8_t fp = 0; fp <= 1; ++fp)
			{
				for (std::uint32_t i = 0; i < armor->armorAddons.size(); i++)
				{
					RE::TESObjectARMA* arma = armor->armorAddons[i];
					if (arma)
					{
						SKEEFixedString modelPath = m_modelMap.GetModelPath(g, fp == 1, armor, arma);
						if (m_modelMap.GetTriShapeMap(modelPath))
						{
							m_dyeable.emplace(armor->formID, true);
							return true;
						}
					}
				}
			}
		}

		m_dyeable.emplace(armor->formID, false);
	}

	return false;
}

void TintMaskInterface::VisitTemplateData(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, std::function<void(MaskTriShapeMap*)> functor)
{
	std::uint8_t gender = 0;
	RE::TESNPC* actorBase = refr->GetBaseObject() ? refr->GetBaseObject()->As<RE::TESNPC>() : nullptr;
	if (actorBase) {
		RE::Actor* actor = static_cast<RE::Actor*>(refr);
		gender = actorBase->GetSex();

		for (std::uint32_t i = 0; i < armor->armorAddons.size(); ++i)
		{
			RE::TESObjectARMA* addon = armor->armorAddons[i];
			if (!addon)
				continue;

			if (!addon->GetPlayable() || !addon->IsValidRace(actor->race))
				continue;

			auto shapeMap = m_modelMap.GetTriShapeMap(m_modelMap.GetModelPath(gender, false, armor, addon));
			if (shapeMap)
			{
				functor(shapeMap);
			}
		}
	}
}

void TintMaskInterface::GetTemplateColorMap(RE::TESObjectREFR* refr, RE::TESObjectARMO * armor, std::map<std::int32_t, std::uint32_t>& colorMap)
{
	VisitTemplateData(refr, armor, [&](auto shapeMap)
	{
		if (shapeMap->IsRemappable())
		{
			for (auto& shape : *shapeMap)
			{
				// Try all textures, though we probably only care about the diffuse
				for (auto& layer : shape.second)
				{
					for (auto& slot : layer.second.slots)
					{
						colorMap[slot.first] = 0;

						auto cit = layer.second.colors.find(slot.second);
						if (cit != layer.second.colors.end())
						{
							colorMap[slot.first] = cit->second & 0xFFFFFF;
						}

						auto ait = layer.second.alphas.find(slot.second);
						if (ait != layer.second.alphas.end())
						{
							colorMap[slot.first] |= std::uint32_t(ait->second * 255) << 24;
						}
					}
				}
			}
		}
		else
		{
			// Try all shapes in this nif
			for (auto& shape : *shapeMap)
			{
				// Try all textures, though we probably only care about the diffuse
				for (auto& layer : shape.second)
				{
					// For each color mapping extract the template color and alpha
					for (auto& color : layer.second.colors)
					{
						colorMap[color.first] = color.second & 0xFFFFFF;

						auto it = layer.second.alphas.find(color.first);
						if (it != layer.second.alphas.end())
						{
							colorMap[color.first] |= std::uint32_t(it->second * 255) << 24;
						}
					}
				}
			}
		}
	});
}

void TintMaskInterface::GetSlotTextureIndexMap(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, std::map<std::int32_t, std::uint32_t>& slotTextureIndexMap)
{
	VisitTemplateData(refr, armor, [&](auto shapeMap)
	{
		if (shapeMap->IsRemappable())
		{
			for (auto& shape : *shapeMap)
			{
				// Try all textures, though we probably only care about the diffuse
				for (auto& layer : shape.second)
				{
					std::int32_t textureIndex = 0;
					if (sscanf_s(layer.first.c_str(), "%d", &textureIndex))
					{
						for (auto& slot : layer.second.slots)
						{
							slotTextureIndexMap[slot.first] = textureIndex;
						}
					}
				}
			}
		}
	});
}

void TintMaskInterface::LoadMods()
{
	m_dyeableLock.lock();
	m_dyeable.clear();
	m_dyeableLock.unlock();
	m_modelMap.Lock();
	m_modelMap.m_data.clear();
	
	std::vector<SKEEFixedString> tintFiles;
	FileUtils::GetAllFiles("Data\\SKSE\\Plugins\\NiOverride\\TintData\\", "*.xml", tintFiles);
	std::sort(tintFiles.begin(), tintFiles.end());

	for (auto tintFile : tintFiles)
	{
		ParseTintData(tintFile.c_str());
	}

	m_modelMap.Release();
}

void TintMaskInterface::ParseTintData(const char* filePath)
{
	std::string path(filePath);
	path.erase(0, 5);

	// BSResourceNiBinaryStream resolves through BSA archives too (legacy behavior);
	// a plain std::ifstream only sees loose files.
	RE::BSResourceNiBinaryStream bStream(path.c_str());
	if (!bStream.good()) {
		SKSE::log::error("{} - Failed to open tint data file {}", __FUNCTION__, path);
		return;
	}
	std::string data;
	BSFileUtil::ReadAll(&bStream, data);

	tinyxml2::XMLDocument tintDoc;
	tintDoc.Parse(data.c_str(), data.size());

	if (tintDoc.Error()) {
		SKSE::log::error("{}", tintDoc.GetErrorStr1());
		return;
	}

	auto element = tintDoc.FirstChildElement("tintmasks");
	if (element) {
		auto object = element->FirstChildElement("object");
		while (object)
		{
			auto objectPath = SKEEFixedString(object->Attribute("path"));
			if (object->BoolAttribute("override")) {
				auto trishapeMap = m_modelMap.GetTriShapeMap(objectPath);
				if (trishapeMap) {
					trishapeMap->clear();
				}
			}

			bool remappable = object->BoolAttribute("remappable");

			std::uint32_t index = 0;
			auto child = object->FirstChildElement("geometry");
			while (child)
			{
				auto trishapeName = child->Attribute("name");
				auto trishape = SKEEFixedString(trishapeName ? trishapeName : "");

				const char * texture = child->Attribute("texture");
				if (!texture)
				{
					texture = child->Attribute("diffuse");
					if (!texture)
					{
						texture = "0";
					}
				}
				
				auto layer = m_modelMap.GetMask(objectPath, trishape, texture);
				if (child->BoolAttribute("override")) {
					layer->types.clear();
					layer->colors.clear();
					layer->alphas.clear();
					layer->blendModes.clear();
					layer->textures.clear();
				}

				if (remappable) {
					index = 0;
				}

				
				auto mask = child->FirstChildElement("mask");
				while (mask) {
					auto path = mask->Attribute("path");
					auto color = mask->Attribute("color");

					std::uint32_t colorValue;
					if (_strnicmp(color, "skin", 4) == 0) {
						colorValue = kPreset_Skin;
					}
					else if (_strnicmp(color, "hair", 4) == 0) {
						colorValue = kPreset_Hair;
					}
					else if (color) {
						sscanf_s(color, "%x", &colorValue);
					}
					else {
						colorValue = 0xFFFFFF;
					}
					
					auto alpha = mask->DoubleAttribute("alpha");

					const char* blend = mask->Attribute("blend");
					const char* type = mask->Attribute("type");
					if (!type) {
						type = "mask";
					}

					std::uint8_t typeNumber = static_cast<std::uint8_t>(CDXNifTextureRenderer::TextureType::Mask);
					if (_strnicmp(type, "mask", 4) == 0) {
						 typeNumber = static_cast<std::uint8_t>(CDXNifTextureRenderer::TextureType::Mask);
					}
					else if (_strnicmp(type, "normal", 6) == 0) {
						typeNumber = static_cast<std::uint8_t>(CDXNifTextureRenderer::TextureType::Normal);
					}
					else if (_strnicmp(type, "solid", 5) == 0 || _strnicmp(type, "color", 5) == 0) {
						typeNumber = static_cast<std::uint8_t>(CDXNifTextureRenderer::TextureType::Color);
					}
					else {
						std::int32_t typeValue = 0;
						sscanf_s(type, "%d", &typeValue);
						typeNumber = static_cast<std::uint8_t>(typeValue);
					}
					
					int i = index;
					mask->QueryIntAttribute("index", &i);

					int slot = -1;
					mask->QueryIntAttribute("slot", &slot);

					layer->textures[i] = path ? path : "";
					layer->colors[i] = colorValue;
					layer->alphas[i] = alpha;
					layer->blendModes[i] = blend ? blend : "overlay";
					layer->types[i] = typeNumber;

					if (remappable && slot >= 0) {
						layer->slots.emplace(slot, i);
					}

					mask = mask->NextSiblingElement("mask");
					index++;
				}

				child = child->NextSiblingElement("geometry");
			}

			auto trishapeMap = m_modelMap.GetTriShapeMap(objectPath);
			if (trishapeMap) {
				trishapeMap->SetRemappable(remappable);
			}

			object = object->NextSiblingElement("object");
		}
	}
}

void TintMaskInterface::OnAttach(RE::TESObjectREFR * refr, RE::TESObjectARMO * armor, RE::TESObjectARMA * addon, RE::NiAVObject * object, bool isFirstPerson, RE::NiNode * skeleton, RE::NiNode * root)
{
	std::uint32_t armorMask = armor->GetSlotMask().underlying();
	std::uint32_t addonMask = addon->GetSlotMask().underlying();
	if (((armorMask & addonMask) & (g_tintHairSlot)) != 0)
	{
		RE::Actor* actor = refr->IsActor() ? static_cast<RE::Actor*>(refr) : nullptr;
		RE::NiColorA value;
		RE::TESNPC * actorBase = static_cast<RE::TESNPC*>(refr->GetBaseObject());
		if (GetActorHairColor(actor, value)) {
			RE::NiColorA* color = &value;
			SKEE::UpdateModelHair(object, &color);
		}
	}
}

bool TintMaskInterface::GetActorHairColor(RE::Actor* actor, RE::NiColorA& color)
{
	RE::BGSColorForm* hairColorForm = nullptr;
	RE::TESNPC* actorBase = static_cast<RE::TESNPC*>(actor->GetBaseObject());
	if (actorBase) {
		std::uint8_t gender = actorBase->GetSex();
		// Try our parent templates
		RE::TESNPC* templateNPC = actorBase;
		do {
			auto headData = templateNPC->headRelatedData;
			if (headData) {
				hairColorForm = headData->hairColor;
			}
			templateNPC = templateNPC->faceNPC;
		} while (templateNPC && !hairColorForm);

		// No templates had a hair color record, get the default from the race
		if (!hairColorForm)
		{
			RE::TESRace* race = actor->race;
			if (!race) {
				race = actorBase->GetRace();
			}

			// Race had no chargen data, they probably don't have a hair record
			auto chargenData = race->faceRelatedData[gender];
			if (!chargenData)
			{
				return false;
			}

			hairColorForm = race->faceRelatedData[gender]->defaultHairColor;
		}
	}

	if (hairColorForm) {
		color.red = static_cast<float>(std::min((std::int32_t)hairColorForm->color.red * 2, 255)) / 255.0f;
		color.green = static_cast<float>(std::min((std::int32_t)hairColorForm->color.green * 2, 255)) / 255.0f;
		color.blue = static_cast<float>(std::min((std::int32_t)hairColorForm->color.blue * 2, 255)) / 255.0f;
		color.alpha = 1.0f;
		return true;
	}

	return false;
}

RE::BSEventNotifyControl TintMaskInterface::ProcessEvent(const SKSE::NiNodeUpdateEvent* a_event, RE::BSTEventSource<SKSE::NiNodeUpdateEvent>* a_source)
{
	RE::TESObjectREFR* refr = a_event->reference;
	if (refr && refr->IsActor())
	{
		RE::Actor* actor = static_cast<RE::Actor*>(refr);
		RE::NiColorA value;
		if (GetActorHairColor(actor, value)) {
			RE::NiColorA* color = &value;
			VisitEquippedNodes(actor, g_tintHairSlot, [&](RE::TESObjectARMO* armor, RE::TESObjectARMA* arma, RE::NiAVObject* node, bool isFP)
			{
				SKEE::UpdateModelHair(node, &color);
			});
		}
	}
	return RE::BSEventNotifyControl::kContinue;
}
