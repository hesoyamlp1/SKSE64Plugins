#pragma once

#include "RE/N/NiObject.h"
#include "RE/N/NiSmartPointer.h"
#include "RE/N/NiRTTI.h"

// netimmerse_isKind: check if an object is of a specific type (or derived from it)
// Equivalent to the original NetImmerse ni_is_type(obj, Type). Relocates To::Ni_RTTI
// (a VariantID) to the game's NiRTTI record, matching how netimmerse_cast resolves it.
template <class To>
inline bool netimmerse_isKind(const RE::NiObject* a_obj) noexcept
{
	if (!a_obj) {
		return false;
	}
	static REL::Relocation<const RE::NiRTTI*> to{ To::Ni_RTTI };
	const RE::NiRTTI* toRTTI = to.get();
	if (!toRTTI) {
		return false;
	}
	return a_obj->GetRTTI()->IsKindOf(toRTTI);
}

// niptr_cast: RTTI-checked downcast of a smart pointer, returning a smart pointer.
// Restores the legacy niptr_cast call shape (P:\git\xSE\skse64\skse64\NiTypes.h) while
// preserving NiPointer ownership and using NetImmerse RTTI instead of an unchecked
// static_cast. Returns a null NiPointer when the object is not of type To.
template <class T_to, class T_from>
RE::NiPointer<T_to> niptr_cast(const RE::NiPointer<T_from>& a_src) noexcept
{
	return RE::NiPointer<T_to>(netimmerse_cast<T_to*>(a_src.get()));
}
