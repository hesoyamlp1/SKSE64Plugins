#include "PartHandler.h"
#include "FileUtils.h"


#include <cstdint>

void PartSet::AddPart(std::uint32_t key, RE::BGSHeadPart* part)
{
	iterator it = find(key);
	if(it != end()) {
		it->second.partList.push_back(part);
	} else {
		PartEntry partEntry;
		partEntry.partList.push_back(part);
		insert(std::make_pair(key, partEntry));
	}
}

void PartSet::SetDefaultPart(std::uint32_t key, RE::BGSHeadPart* part)
{
	iterator it = find(key);
	if(it != end()) {
		it->second.defaultPart = part;
	} else {
		PartEntry partEntry;
		partEntry.defaultPart = part;
		insert(std::make_pair(key, partEntry));
	}
}

void PartSet::Visit(Visitor & visitor)
{
	for (iterator it = begin(); it != end(); ++it)
	{
		for(HeadPartList::iterator pit = it->second.partList.begin(); pit != it->second.partList.end(); ++pit)
		{
			if(visitor.Accept(it->first, *pit))
				return;
		}
	}
}

HeadPartList * PartSet::GetPartList(std::uint32_t key)
{
	iterator it = find(key);
	if(it != end()) {
		return &it->second.partList;
	}

	return NULL;
}

RE::BGSHeadPart * PartSet::GetDefaultPart(std::uint32_t key)
{
	iterator it = find(key);
	if(it != end()) {
		return it->second.defaultPart;
	}

	return NULL;
}

RE::BGSHeadPart * PartSet::GetPartByIndex(HeadPartList * headPartList, std::uint32_t index)
{
	if(index < headPartList->size())
		return headPartList->at(index);

	return NULL;
}

std::int32_t PartSet::GetPartIndex(HeadPartList * headPartList, RE::BGSHeadPart * headPart)
{
	std::int32_t partIndex = -1;
	for(std::uint32_t p = 0; p < headPartList->size(); p++)
	{
		RE::BGSHeadPart * partMatch = headPartList->at(p);
		if(partMatch->formEditorID == headPart->formEditorID) {
			partIndex = p;
			break;
		}
	}

	return partIndex;
}

void PartSet::Revert()
{
	for (iterator it = begin(); it != end(); ++it)
	{
		it->second.partList.clear();
	}

	clear();
}

void ReadPartReplacements(std::string fixedPath, std::string modPath, std::string fileName)
{
	std::string fullPath = fixedPath + modPath + fileName;
	RE::BSResourceNiBinaryStream file(fullPath.c_str());
	if (!file.good()) {
		return;
	}

	std::uint32_t lineCount = 0;
	std::uint8_t gender = 0;
	std::string str = "";
	while (BSFileUtil::ReadLine(&file, &str))
	{
		lineCount++;
		str = std::trim(str);
		if (str.length() == 0)
			continue;
		if (str.at(0) == '#')
			continue;

		if (str.at(0) == '[')
		{
			str.erase(0, 1);
			if (_strnicmp(str.c_str(), "Male", 4) == 0)
				gender = 0;
			if (_strnicmp(str.c_str(), "Female", 6) == 0)
				gender = 1;
			continue;
		}

		std::vector<std::string> side = std::explode(str, '=');
		if (side.size() < 2) {
			SKSE::log::error("{} Error - Line ({}) race from {} has no left-hand side.", __FUNCTION__, lineCount, fullPath.c_str());
			continue;
		}

		std::string lSide = std::trim(side[0]);
		std::string rSide = std::trim(side[1]);

		RE::BGSHeadPart * facePart = GetHeadPartByName(rSide);
		RE::TESRace * race = GetRaceByName(lSide);
		if (!race) {
			SKSE::log::warn("{} Warning - Line ({}) race {} from {} is not a valid race.", __FUNCTION__, lineCount, lSide.c_str(), fullPath.c_str());
			continue;
		}

		if (!facePart) {
			SKSE::log::warn("{} Warning - Line ({}) head part {} from {} is not a valid head part.", __FUNCTION__, lineCount, rSide.c_str(), fullPath.c_str());
			continue;
		}

		auto charGenData = race->faceRelatedData[gender];
		if (!charGenData) {
			SKSE::log::error("{} Error - Line ({}) race {} from {} has no CharGen data.", __FUNCTION__, lineCount, lSide.c_str(), fullPath.c_str());
			continue;
		}

		if (charGenData->headParts) {
			for (std::uint32_t i = 0; i < charGenData->headParts->size(); i++) {
				RE::BGSHeadPart * headPart = (*charGenData->headParts)[i];
				if (headPart) {
					if (headPart->type == facePart->type) {
						(*charGenData->headParts)[i] = facePart;
					}
				}
			}
		}
	}
}