#include "FileUtils.h"
#include <REX/W32/KERNEL32.h>

#include <unordered_set>
#include <queue>
#include <RE/T/TESDataHandler.h>
#include <RE/T/TESFile.h>
#include <RE/T/TESLevCharacter.h>
#include <RE/N/NiBinaryStream.h>
#include <filesystem>

#include <cstdint>


// trim from start
namespace std
{
	std::string &ltrim(std::string &s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int s) { return !std::isspace(s); }));
		return s;
	}

	// trim from end
	std::string &rtrim(std::string &s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), [](int s) { return !std::isspace(s); }).base(), s.end());
		return s;
	}

	// trim from both ends
	std::string &trim(std::string &s) {
		return ltrim(rtrim(s));
	}

	// return vector of delimited string
	std::vector<std::string> explode(const std::string& str, const char& ch) {
		std::string next;
		std::vector<std::string> result;

		for (std::string::const_iterator it = str.begin(); it != str.end(); it++) {
			if (*it == ch) {
				if (!next.empty()) {
					result.push_back(next);
					next.clear();
				}
			}
			else {
				next += *it;
			}
		}
		if (!next.empty())
			result.push_back(next);
		return result;
	}
}

namespace BSFileUtil
{
bool ReadLine(RE::BSResourceNiBinaryStream* fin, std::string* str)
{
	char buf[1024];
	std::uint32_t ret = 0;

	for (std::uint32_t i = 0; i < 1023; ++i) {
		if (!fin->get(buf[i])) break;
		buf[i + 1] = '\0';
		ret = i + 1;
		if (buf[i] == '\n') break;
	}
	if (ret > 0) {
		if (buf[ret - 1] == '\n') buf[ret - 1] = '\0';
		*str = buf;
		return true;
	}
	return false;
}
bool IsActive(const RE::TESFile* modInfo)
{
    if (modInfo) {
        return modInfo->GetCompileIndex() != 0xFF;
    }
    return false;
}
}

namespace FileUtils
{
static void SKEEPathCombine(char (&dst)[REX::W32::MAX_PATH], const char* dir, const char* file)
{
    auto p = std::filesystem::path(dir) / file;
    std::snprintf(dst, sizeof(dst), "%s", p.string().c_str());
}

void GetAllFiles(const char* lpFolder, const char* lpFilePattern, std::vector<SKEEFixedString> & filePaths)
{
	char szFullPattern[REX::W32::MAX_PATH];
	REX::W32::WIN32_FIND_DATAA FindFileData;
	REX::W32::HANDLE hFindFile;
	// first we are going to process any subdirectories
	SKEEPathCombine(szFullPattern, lpFolder, "*");
	hFindFile = REX::W32::FindFirstFileA(szFullPattern, &FindFileData);
	if (hFindFile != REX::W32::INVALID_HANDLE_VALUE)
	{
		do
		{
			if (FindFileData.fileAttributes & REX::W32::FILE_ATTRIBUTE_DIRECTORY)
			{
				// found a subdirectory; recurse into it
				SKEEPathCombine(szFullPattern, lpFolder, FindFileData.fileName);
				if (FindFileData.fileName[0] == '.')
					continue;
				GetAllFiles(szFullPattern, lpFilePattern, filePaths);
			}
		} while (REX::W32::FindNextFileA(hFindFile, &FindFileData));
		REX::W32::FindClose(hFindFile);
	}
	// now we are going to look for the matching files
	SKEEPathCombine(szFullPattern, lpFolder, lpFilePattern);
	hFindFile = REX::W32::FindFirstFileA(szFullPattern, &FindFileData);
	if (hFindFile != REX::W32::INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(FindFileData.fileAttributes & REX::W32::FILE_ATTRIBUTE_DIRECTORY))
			{
				// found a file; do something with it
				SKEEPathCombine(szFullPattern, lpFolder, FindFileData.fileName);
				filePaths.push_back(szFullPattern);
			}
		} while (REX::W32::FindNextFileA(hFindFile, &FindFileData));
		REX::W32::FindClose(hFindFile);
	}
}

void MakeAllDirs(const char* path)
{
	// Create every directory component of `path` (legacy IFileStream::MakeAllDirs).
	std::string_view p(path);
	while (true)
	{
		const auto pos = p.find_last_of("\\/");
		if (pos == std::string_view::npos || pos == 0)
			break;
		std::error_code ec;
		std::filesystem::create_directories(std::string(p.substr(0, pos)), ec);
		p = p.substr(0, pos);
	}
}
}

RE::TESRace * GetRaceByName(std::string & raceName)
{
	RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
	if (dataHandler)
	{
		auto& races = dataHandler->GetFormArray(RE::FormType::Race);
		for (std::uint32_t i = 0; i < races.size(); i++)
		{
			RE::TESRace * race = races[i] ? races[i]->As<RE::TESRace>() : nullptr;
			if (race) {
				RE::BSFixedString raceStrName(raceName.c_str());
				if (race->formEditorID == raceStrName) {
					return race;
				}
			}
		}
	}

	return NULL;
}

RE::BGSHeadPart * GetHeadPartByName(std::string & headPartName)
{
	RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
	if (dataHandler)
	{
		auto& headParts = dataHandler->GetFormArray(RE::FormType::HeadPart);
		for (std::uint32_t i = 0; i < headParts.size(); i++)
		{
			RE::BGSHeadPart * headPart = headParts[i] ? headParts[i]->As<RE::BGSHeadPart>() : nullptr;
			if (headPart) {
				RE::BSFixedString partName(headPartName.c_str());
				if (headPart->formEditorID == partName) {
					return headPart;
				}
			}
		}
	}

	return NULL;
}

RE::TESFile* GetModInfoByFormID(std::uint32_t formId, bool allowLight)
{
	auto * dataHandler = RE::TESDataHandler::GetSingleton();

	std::uint8_t modIndex = formId >> 24;
	std::uint16_t lightIndex = ((formId >> 12) & 0xFFF);

	RE::TESFile* modInfo = nullptr;
	if (modIndex == 0xFE && allowLight) {
		modInfo = const_cast<RE::TESFile*>(dataHandler->LookupLoadedLightModByIndex(lightIndex));
	} else if (modIndex < 0xFE) {
		modInfo = const_cast<RE::TESFile*>(dataHandler->LookupLoadedModByIndex(modIndex));
	}

	return modInfo;
}

std::string GetFormIdentifier(RE::TESForm * form)
{
	char formName[REX::W32::MAX_PATH];
	std::uint8_t modIndex = form->formID >> 24;
	std::uint32_t modForm = form->formID & 0xFFFFFF;

	RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
	RE::TESFile* modInfo = nullptr;
	if (modIndex == 0xFE)
	{
		std::uint16_t lightIndex = (form->formID >> 12) & 0xFFF;
		modInfo = const_cast<RE::TESFile*>(dataHandler->LookupLoadedLightModByIndex(lightIndex));
	}
	else if (modIndex < 0xFE)
	{
		modInfo = const_cast<RE::TESFile*>(dataHandler->LookupLoadedModByIndex(modIndex));
	}

	if (modInfo) {
		sprintf_s(formName, "%s|%06X", modInfo->GetFilename().data(), modForm);
	}

	return std::string(formName);
}

RE::TESForm * GetFormFromIdentifier(const std::string & formIdentifier)
{
	std::size_t pos = formIdentifier.find_first_of('|');
	if (pos == std::string::npos) {
		return RE::TESForm::LookupByEditorID(formIdentifier);
	}
	std::string modName = formIdentifier.substr(0, pos);
	std::string modForm = formIdentifier.substr(pos + 1);

	std::uint32_t formId = 0;
	sscanf_s(modForm.c_str(), "%X", &formId);

	const RE::TESFile* modInfo = RE::TESDataHandler::GetSingleton()->LookupModByName(modName);
	if (!modInfo || !BSFileUtil::IsActive(modInfo)) {
		return nullptr;
	}

	return RE::TESForm::LookupByID(modInfo->GetFormID(formId));
}

void VisitLeveledCharacter(RE::TESLevCharacter * character, std::function<void(RE::TESNPC*)> functor)
{
	std::unordered_set<RE::TESLevCharacter*> visited;
	std::queue<RE::TESLevCharacter*> visit;

	visit.push(character);

	while (!visit.empty())
	{
		character = visit.front();
		visit.pop();

		if (character)
		{
			for (std::uint32_t i = 0; i < character->numEntries; i++)
			{
				RE::TESForm * form = character->entries[i].form;
				if (form) {
					RE::TESLevCharacter * levCharacter = form ? form->As<RE::TESLevCharacter>() : nullptr;
					if (levCharacter && visited.find(levCharacter) == visited.end())
						visit.push(levCharacter);

					RE::TESNPC * npc = form ? form->As<RE::TESNPC>() : nullptr;
					if (npc)
						functor(npc);
				}
			}

			visited.insert(character);
		}
	}
}

void ForEachMod(std::function<void(RE::TESFile*)> functor)
{
	RE::TESDataHandler* dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler)
		return;

	// Walk the data handler's aggregate list of all loaded mods (load order,
	// including light plugins); skip inactive entries.
	for (RE::TESFile* modInfo : dataHandler->files)
	{
		if (modInfo && BSFileUtil::IsActive(modInfo))
		{
			functor(modInfo);
		}
	}
}