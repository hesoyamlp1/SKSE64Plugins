#pragma once

#include <vector>
#include <unordered_map>

#include "FileUtils.h"
#include "RaceMenuTypes.h"
#include <cstdint>
namespace RE
{
	class BGSHeadPart;
}

typedef std::vector<RE::BGSHeadPart*> HeadPartList;

class PartEntry
{
public:
	RE::BGSHeadPart * defaultPart = nullptr;
	HeadPartList partList;
};

class PartSet : public std::unordered_map<std::uint32_t, PartEntry>
{
public:
	class Visitor
	{
	public:
		virtual bool Accept(std::uint32_t key, RE::BGSHeadPart * part) { return false; };
	};

	void LoadSliders(RE::RaceMenuSliderArray * sliderArray, RE::RaceMenuSlider * slider);

	void AddPart(std::uint32_t key, RE::BGSHeadPart* part);
	void Visit(Visitor & visitor);

	HeadPartList * GetPartList(std::uint32_t key);
	RE::BGSHeadPart * GetDefaultPart(std::uint32_t key);
	void SetDefaultPart(std::uint32_t key, RE::BGSHeadPart* part);
	std::int32_t GetPartIndex(HeadPartList * headPartList, RE::BGSHeadPart * headPart);
	RE::BGSHeadPart * GetPartByIndex(HeadPartList * headPartList, std::uint32_t index);

	void Revert();
};

void ReadPartReplacements(std::string fixedPath, std::string modPath, std::string fileName);