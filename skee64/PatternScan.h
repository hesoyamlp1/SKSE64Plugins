#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// Compile-time "XX ?? YY" byte patterns plus a runtime scanner.
//
// Wildcards can't be encoded in the byte array itself, so BytePattern pairs a
// byte buffer with a per-byte mask: bytes[i] only matters when mask[i] is set.
// MakePattern parses a space-separated hex string at compile time ("?"/"??"
// tokens are wildcards) and collapses it into this struct, keeping call sites
// readable:
//
//	uintptr_t hit = PatternScan::FindUnique(base, range, PatternScan::MakePattern("48 8B 4C 18 18"));

namespace PatternScan
{
	struct BytePattern
	{
		static constexpr size_t kMax = 128;

		std::array<uint8_t, kMax> bytes{};
		std::array<bool, kMax> mask{};	// true = literal byte, false = wildcard
		size_t len = 0;					// == kMax means the pattern string was invalid
	};

	consteval bool IsHexDigit(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

	consteval int HexValue(char c)
	{
		if (c >= '0' && c <= '9') return c - '0';
		if (c >= 'A' && c <= 'F') return c - 'A' + 10;
		return c - 'a' + 10;
	}

	consteval BytePattern MakePattern(const char* str)
	{
		BytePattern out;
		bool err = false;
		while (*str)
		{
			if (*str == ' ') { str++; continue; }
			const char* tok = str;
			while (*str && *str != ' ') str++;
			size_t n = (size_t)(str - tok);
			bool wild = true;
			int val = 0;
			for (size_t i = 0; i < n; i++)
			{
				char c = tok[i];
				if (c == '?') continue;
				wild = false;
				if (!IsHexDigit(c)) { err = true; break; }
				val = val * 16 + HexValue(c);
			}
			if (!wild && n != 2) err = true;
			if (out.len >= BytePattern::kMax) err = true;
			out.bytes[out.len] = (uint8_t)val;
			out.mask[out.len] = !wild;
			out.len++;
		}
		if (err || out.len == 0) out.len = BytePattern::kMax;
		return out;
	}

	inline bool MatchAt(const uint8_t* data, const BytePattern& p)
	{
		for (size_t i = 0; i < p.len; i++)
			if (p.mask[i] && data[i] != p.bytes[i])
				return false;
		return true;
	}

	// Scans [base, base + range). Returns the address of the match, or 0 unless
	// the pattern matches exactly once in that range.
	inline uintptr_t FindUnique(const uint8_t* base, size_t range, const BytePattern& p)
	{
		if (p.len == 0 || p.len >= BytePattern::kMax || p.len > range) return 0;
		uintptr_t first = 0;
		for (size_t i = 0; i + p.len <= range; i++)
		{
			if (!MatchAt(base + i, p)) continue;
			if (!first) { first = (uintptr_t)(base + i); continue; }
			return 0; // second match: not unique
		}
		return first;
	}
}
