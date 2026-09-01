#pragma once

namespace RE
{
	class GFxValue;
	class GFxMovie;
}

namespace ScaleformUtils
{
	void RegisterNumber(RE::GFxValue* dst, const char* name, double value);
	void RegisterBool(RE::GFxValue* dst, const char* name, bool value);
	void RegisterUnmanagedString(RE::GFxValue* dst, const char* name, const char* str);
	void RegisterString(RE::GFxValue* dst, RE::GFxMovie* movie, const char* name, const char* str);
}
