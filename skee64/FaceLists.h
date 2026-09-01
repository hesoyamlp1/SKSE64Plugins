#pragma once
// Project-local face preset / morph index enumerations.
//
// The legacy skse64/GameData.h defined FacePresetList / FaceMorphList as game-data
// singletons (with GetSingleton() and presets[]/morphs[] arrays). SKEE only uses their
// ENUM CONSTANTS as array indices (it populates its own name arrays from game settings),
// so we recreate just the index enumerations here. The actual per-preset/morph game data
// is not needed by the plugin.

namespace FacePresetList
{
	constexpr std::uint32_t kNumPresets = 4;
	enum
	{
		kPreset_NoseType,
		kPreset_BrowType,
		kPreset_EyesType,
		kPreset_LipType
	};
}

namespace FaceMorphList
{
	constexpr std::uint32_t kNumMorphs = 19;
	enum
	{
		kMorph_NoseShortLong = 0,
		kMorph_NoseDownUp,
		kMorph_JawUpDown,
		kMorph_JawNarrowWide,
		kMorph_JawBackForward,
		kMorph_CheeksDownUp,
		kMorph_CheeksInOut,
		kMorph_EyesMoveDownUp,
		kMorph_EyesMoveInOut,
		kMorph_BrowDownUp,
		kMorph_BrowInOut,
		kMorph_BrowBackForward,
		kMorph_LipMoveDownUp,
		kMorph_LipMoveInOut,
		kMorph_ChinThinWide,
		kMorph_ChinMoveUpDown,
		kMorph_OverbiteUnderbite,
		kMorph_EyesBackForward,
		kMorph_Vampire
	};
}
