#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional> 
#include <cctype>
#include <locale>
#include <fstream>
#include <filesystem>

#include <RE/B/BSResourceNiBinaryStream.h>
#include <RE/T/TESFile.h>
#include "StringTable.h"
#include <cstdint>

namespace RE
{
	class TESRace;
	class BGSHeadPart;
	class TESLevCharacter;
	class TESNPC;
	class TESForm;
}

namespace std {
	extern inline std::string &ltrim(std::string &s);
	extern inline std::string &rtrim(std::string &s);
	extern inline std::string &trim(std::string &s);
	std::vector<std::string> explode(const std::string& str, const char& ch);
}

namespace BSFileUtil {
	bool ReadLine(RE::BSResourceNiBinaryStream* fin, std::string * str);

	template<typename Container>
	void ReadAll(RE::BSResourceNiBinaryStream* fin, Container & c)
	{
		char ch;
		while (fin->get(ch)) {
			c.push_back(ch);
		}
	}
	bool IsActive(const RE::TESFile* modInfo);
}

namespace FileUtils
{
	void GetAllFiles(const char* lpFolder, const char* lpFilePattern, std::vector<SKEEFixedString> & filePaths);

	// Creates every directory component of `path` (legacy IFileStream::MakeAllDirs).
	void MakeAllDirs(const char* path);
}

// Minimal binary file stream backed by std::fstream. Replaces the legacy
// skse64 common/IFileStream for the subset of its API the plugin uses.
class BinaryStream
{
public:
	BinaryStream() = default;
	explicit BinaryStream(const char* path) { Open(path); }

	bool Open(const char* path)
	{
		Close();
		m_file.open(path, std::ios::in | std::ios::out | std::ios::binary);
		return m_file.is_open();
	}

	bool Create(const char* path)
	{
		Close();
		m_file.open(path, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
		return m_file.is_open();
	}

	void Close()
	{
		if (m_file.is_open())
		{
			m_file.close();
		}
	}

	void ReadBuf(void* buf, std::uint32_t length)
	{
		m_file.read(static_cast<char*>(buf), static_cast<std::streamsize>(length));
	}

	void WriteBuf(const void* buf, std::uint32_t length)
	{
		m_file.write(static_cast<const char*>(buf), static_cast<std::streamsize>(length));
	}

	void Write8(std::uint8_t value)   { m_file.put(static_cast<char>(value)); }
	void Write16(std::uint16_t value) { m_file.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
	void Write32(std::uint32_t value) { m_file.write(reinterpret_cast<const char*>(&value), sizeof(value)); }
	void WriteFloat(float value)      { m_file.write(reinterpret_cast<const char*>(&value), sizeof(value)); }

	std::int64_t GetOffset()
	{
		return static_cast<std::int64_t>(m_file.tellp());
	}

	void SetOffset(std::int64_t offset)
	{
		m_file.seekp(offset, std::ios::beg);
		m_file.seekg(offset, std::ios::beg);
	}

	void Skip(std::int64_t bytes)
	{
		SetOffset(GetOffset() + bytes);
	}

private:
	std::fstream m_file;
};

RE::TESRace * GetRaceByName(std::string & raceName);
RE::BGSHeadPart * GetHeadPartByName(std::string & headPartName);

RE::TESFile* GetModInfoByFormID(std::uint32_t formId, bool allowLight = true);

std::string GetFormIdentifier(RE::TESForm * form);
RE::TESForm * GetFormFromIdentifier(const std::string & formIdentifier);

void ForEachMod(std::function<void(RE::TESFile *)> functor);

template<int MaxBuf>
class BSResourceTextFile
{
public:
	BSResourceTextFile(RE::BSResourceNiBinaryStream* file) : fin(file) { }

	bool ReadLine(std::string* str)
	{
		str->clear();
		char ch;
		while (fin->get(ch)) {
			if (ch == '\n') {
				return true;
			}
			if (ch != '\r') {
				str->push_back(ch);
			}
		}
		return !str->empty();
	}

protected:
	RE::BSResourceNiBinaryStream * fin;
	char buf[MaxBuf];
};

void VisitLeveledCharacter(RE::TESLevCharacter * character, std::function<void(RE::TESNPC*)> functor);
