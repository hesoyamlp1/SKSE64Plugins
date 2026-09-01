#include "OverrideVariant.h"
#include "StringTable.h"

#include <RE/B/BSFixedString.h>
#include <RE/N/NiColor.h>
#include <RE/T/TESForm.h>

#include <SKSE/API.h>

template <> void PackValue<float>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, float* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderEmissiveMultiple:
		case OverrideVariant::kParam_ShaderGlossiness:
		case OverrideVariant::kParam_ShaderSpecularStrength:
		case OverrideVariant::kParam_ShaderLightingEffect1:
		case OverrideVariant::kParam_ShaderLightingEffect2:
		case OverrideVariant::kParam_ShaderAlpha:
		case OverrideVariant::kParam_ControllerStartStop:
		case OverrideVariant::kParam_ControllerStartTime:
		case OverrideVariant::kParam_ControllerStopTime:
		case OverrideVariant::kParam_ControllerFrequency:
		case OverrideVariant::kParam_ControllerPhase:
		case OverrideVariant::kParam_NodeTransformPosition:
		case OverrideVariant::kParam_NodeTransformRotation:
		case OverrideVariant::kParam_NodeTransformScale:
			dst->SetFloat(key, index, *src);
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<std::uint32_t>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, std::uint32_t* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderEmissiveColor:
		case OverrideVariant::kParam_ShaderTintColor:
		case OverrideVariant::kParam_NodeTransformScaleMode:
			dst->SetInt(key, index, static_cast<std::int32_t>(*src));
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<std::int32_t>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, std::int32_t* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderEmissiveColor:
		case OverrideVariant::kParam_ShaderTintColor:
		case OverrideVariant::kParam_NodeTransformScaleMode:
			dst->SetInt(key, index, *src);
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<bool>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, bool* src)
{
	dst->SetNone();
	// dst->SetBool(key, index, *src);
}

template <> void PackValue<SKEEFixedString>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, SKEEFixedString* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderTexture:
		case OverrideVariant::kParam_NodeDestination:
			dst->SetString(key, index, *src);
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<RE::BSFixedString>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, RE::BSFixedString* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderTexture:
		case OverrideVariant::kParam_NodeDestination:
			dst->SetString(key, index, SKEEFixedString(src->c_str()));
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<RE::NiColor>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, RE::NiColor* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderEmissiveColor:
		case OverrideVariant::kParam_ShaderTintColor:
			dst->SetColor(key, index, *src);
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<RE::NiColorA>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, RE::NiColorA* src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderEmissiveColor:
			dst->SetColorA(key, index, *src);
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void PackValue<RE::BGSTextureSet*>(OverrideVariant* dst, std::uint16_t key, std::uint8_t index, RE::BGSTextureSet** src)
{
	switch (key)
	{
		case OverrideVariant::kParam_ShaderTextureSet:
			dst->SetIdentifier(key, index, static_cast<void*>(*src));
			break;
		default:
			dst->SetNone();
			break;
	}
}

template <> void UnpackValue<float>(float* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			*dst = static_cast<float>(src->data.i);
			break;
		case OverrideVariant::kType_Float:
			*dst = src->data.f;
			break;
		case OverrideVariant::kType_Bool:
			*dst = src->data.b ? 1.0f : 0.0f;
			break;
		default:
			*dst = 0.0f;
			break;
	}
}

template <> void UnpackValue<std::uint32_t>(std::uint32_t* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			*dst = src->data.u;
			break;
		case OverrideVariant::kType_Float:
			*dst = static_cast<std::uint32_t>(src->data.f);
			break;
		case OverrideVariant::kType_Bool:
			*dst = src->data.b ? 1 : 0;
			break;
		default:
			*dst = 0;
			break;
	}
}

template <> void UnpackValue<std::int32_t>(std::int32_t* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			*dst = src->data.i;
			break;
		case OverrideVariant::kType_Float:
			*dst = static_cast<std::int32_t>(src->data.f);
			break;
		case OverrideVariant::kType_Bool:
			*dst = src->data.b ? 1 : 0;
			break;
		default:
			*dst = 0;
			break;
	}
}

template <> void UnpackValue<bool>(bool* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			*dst = src->data.u != 0;
			break;
		case OverrideVariant::kType_Float:
			*dst = src->data.f != 0.0f;
			break;
		case OverrideVariant::kType_Bool:
			*dst = src->data.b;
			break;
		default:
			*dst = false;
			break;
	}
}

template <> void UnpackValue<SKEEFixedString>(SKEEFixedString* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_String:
		{
			auto str = src->str;
			*dst = str ? str->c_str() : "";
		}
		break;
		default:
			*dst = "";
			break;
	}
}

template <> void UnpackValue<RE::BSFixedString>(RE::BSFixedString* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_String:
		{
			auto str = src->str;
			*dst = str ? str->c_str() : "";
		}
		break;
		default:
			*dst = "";
			break;
	}
}

template <> void UnpackValue<RE::NiColor>(RE::NiColor* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			dst->red = ((src->data.u >> 16) & 0xFF) / 255.0f;
			dst->green = ((src->data.u >> 8) & 0xFF) / 255.0f;
			dst->blue = (src->data.u & 0xFF) / 255.0f;
			break;
		default:
			dst->red = 0.0f;
			dst->green = 0.0f;
			dst->blue = 0.0f;
			break;
	}
}

template <> void UnpackValue<RE::NiColorA>(RE::NiColorA* dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Int:
			dst->alpha = ((src->data.u >> 24) & 0xFF) / 255.0f;
			dst->red = ((src->data.u >> 16) & 0xFF) / 255.0f;
			dst->green = ((src->data.u >> 8) & 0xFF) / 255.0f;
			dst->blue = (src->data.u & 0xFF) / 255.0f;
			break;
		default:
			dst->red = 0.0f;
			dst->green = 0.0f;
			dst->blue = 0.0f;
			dst->alpha = 0.0f;
			break;
	}
}

template <> void UnpackValue<RE::BGSTextureSet*>(RE::BGSTextureSet** dst, OverrideVariant* src)
{
	switch (src->type)
	{
		case OverrideVariant::kType_Identifier:
			*dst = static_cast<RE::BGSTextureSet*>(src->data.p);
			break;
		default:
			*dst = nullptr;
			break;
	}
}

bool OverrideVariant::IsIndexValid(std::uint16_t key)
{
	return (key >= OverrideVariant::kParam_ControllersStart && key <= OverrideVariant::kParam_ControllersEnd) ||
	       key == OverrideVariant::kParam_ShaderTexture ||
	       (key >= OverrideVariant::kParam_NodeTransformStart && key <= OverrideVariant::kParam_NodeTransformEnd);
}