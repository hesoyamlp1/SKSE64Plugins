#include "ScaleformUtils.h"

#include "RE/G/GFxValue.h"
#include "RE/G/GFxMovie.h"

namespace ScaleformUtils
{
	void RegisterNumber(RE::GFxValue* dst, const char* name, double value)
	{
		RE::GFxValue	fxValue{};
		fxValue.SetNumber(value);
		dst->SetMember(name, fxValue);
	}

	void RegisterBool(RE::GFxValue* dst, const char* name, bool value)
	{
		RE::GFxValue	fxValue{};
		fxValue.SetBoolean(value);
		dst->SetMember(name, fxValue);
	}

	void RegisterUnmanagedString(RE::GFxValue* dst, const char* name, const char* str)
	{
		RE::GFxValue	fxValue{};
		fxValue.SetString(str);
		dst->SetMember(name, fxValue);
	}

	void RegisterString(RE::GFxValue* dst, RE::GFxMovie* movie, const char* name, const char* str)
	{
		RE::GFxValue	fxValue{};
		movie->CreateString(&fxValue, str);
		dst->SetMember(name, fxValue);
	}
}
