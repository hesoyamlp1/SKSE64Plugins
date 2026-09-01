#include "ActorArmorTangentUpdater.h"


#include "RE/N/NiGeometry.h"

#include <functional>

#include "NifUtils.h"
#include <cstdint>

void ActorArmorTangentUpdater::OnAttach(RE::TESObjectREFR* refr, RE::TESObjectARMO* armor, RE::TESObjectARMA* addon, RE::NiAVObject* object, bool isFirstPerson, RE::NiNode* skeleton, RE::NiNode* root)
{
	if (refr && refr->IsActor())
	{
		RE::TESObjectARMO* skinForm = nullptr;
		RE::Actor* actor = refr ? refr->As<RE::Actor>() : nullptr;
		RE::TESNPC* actorBase = actor->GetActorBase();
		std::uint8_t gender = 0;

		RE::TESRace* race = actor->GetRace();
		if (actorBase) {
			if (!race) {
				race = actorBase->GetRace();
			}
			gender = static_cast<std::uint8_t>(actorBase->GetSex());
		}

		if (!skinForm && race) {
			skinForm = race->skin;
		}

		if (skinForm)
		{
			std::function<RE::BGSTextureSet* ()> GetTextureSet = [=]()
			{
				RE::BGSTextureSet* textureSet = nullptr;
				for (std::uint32_t i = 0; i < skinForm->armorAddons.size(); ++i)
				{
					RE::TESObjectARMA* arma = skinForm->armorAddons[i];
					if (addon->GetSlotMask().underlying() & arma->GetSlotMask().underlying() && arma->IsValidRace(race))
					{
						textureSet = arma->skinTextures[gender];
						break;
					}
				}

				return textureSet;
			};

			RE::BGSTextureSet* textureSet = nullptr;
			VisitGeometry(object, [&textureSet, &GetTextureSet, refr, armor, addon](RE::BSGeometry* geometry)
			{
				RE::BSShaderProperty* shaderProperty = geometry->GetGeometryRuntimeData().shaderProperty.get();
				if (shaderProperty && shaderProperty->flags.all(RE::BSShaderProperty::EShaderPropertyFlag::kModelSpaceNormals))
				{
					RE::BSLightingShaderMaterial* material = static_cast<RE::BSLightingShaderMaterial*>(shaderProperty->material);
					if (material && material->GetFeature() == RE::BSShaderMaterial::Feature::kFaceGenRGBTint)
					{
						if (!textureSet)
						{
							textureSet = GetTextureSet();
						}
						if (textureSet && !(textureSet->flags.all(RE::BGSTextureSet::Flag::kHasModelSpaceNormalMap)))
						{
							shaderProperty->SetFlags(RE::BSShaderProperty::EShaderPropertyFlag8::kModelSpaceNormals, false);
						}
					}
				}

				return false;
			});
		}
	}
}
