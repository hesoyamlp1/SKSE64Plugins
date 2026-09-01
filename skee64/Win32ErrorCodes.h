#pragma once

#include <cstdint>

// Stable Win32 error codes (winerror.h values — ABI-stable across all Windows
// versions). REX::W32 does not define these. windows.h is still pulled in
// transitively by the PCH (DirectXTK SimpleMath), which #defines the bare
// ERROR_* macros; drop them so we can provide namespaced constants that keep
// the original names, in the REX::W32 style. Do not rely on the bare macros.
#ifdef ERROR_FILE_NOT_FOUND
#undef ERROR_FILE_NOT_FOUND
#endif
#ifdef ERROR_PATH_NOT_FOUND
#undef ERROR_PATH_NOT_FOUND
#endif
#ifdef ERROR_ACCESS_DENIED
#undef ERROR_ACCESS_DENIED
#endif
#ifdef ERROR_INSUFFICIENT_BUFFER
#undef ERROR_INSUFFICIENT_BUFFER
#endif

namespace skee::W32
{
	inline constexpr std::uint32_t ERROR_FILE_NOT_FOUND = 2;
	inline constexpr std::uint32_t ERROR_PATH_NOT_FOUND = 3;
	inline constexpr std::uint32_t ERROR_ACCESS_DENIED = 5;
	inline constexpr std::uint32_t ERROR_INSUFFICIENT_BUFFER = 122;
}
