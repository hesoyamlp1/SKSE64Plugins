#pragma once

#include "RE/B/BSFixedString.h"

namespace std {
	template <> struct hash < RE::BSFixedString >
	{
		size_t operator()(const RE::BSFixedString & x) const
		{
			return hash<const char*>()(x.c_str());
		};
	};
}
