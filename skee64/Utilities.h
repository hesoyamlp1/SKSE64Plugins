#pragma once

#include <cstdarg>
#include <mutex>

#include <RE/C/ConsoleLog.h>
#include <REX/W32/BASE.h>
#include <REX/W32/KERNEL32.h>


namespace utils
{
	class ScopedCriticalSection
	{
	public:
		ScopedCriticalSection(REX::W32::CRITICAL_SECTION* cs) : m_cs(cs)
		{
			REX::W32::EnterCriticalSection(m_cs);
		};
		~ScopedCriticalSection()
		{
			REX::W32::LeaveCriticalSection(m_cs);
		}

	private:
		REX::W32::CRITICAL_SECTION* m_cs;
	};

	inline void hash_combine(std::size_t& seed) { }

	template <typename T, typename... Rest>
	inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
		std::hash<T> hasher;
		seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		hash_combine(seed, rest...);
	}

	size_t hash_lower(const char* str, size_t count);

	std::string format(const char* format, ...);


#if __cplusplus > 201703L
	template<typename T = std::mutex>
	using scoped_lock = std::scoped_lock<T>;
#else
	template<typename T = std::mutex>
	using scoped_lock = std::lock_guard<T>;
#endif
}

// Project-local replacement for the legacy SKSE GameAPI Console_Print:
// prints a formatted message to the game console (RE::ConsoleLog).
inline void Console_Print(const char* fmt, ...)
{
	auto* log = RE::ConsoleLog::GetSingleton();
	if (!log) {
		return;
	}
	va_list args;
	va_start(args, fmt);
	log->VPrint(fmt, args);
	va_end(args);
}
