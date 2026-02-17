#pragma once
#include "Namespaces.h"

#include <unordered_map>
#include <string>
#include <array>
#include <cstdint>

namespace Platform
{
	enum class Error {
		Ok = 0,				 // No error
		NotPermitted,        // Operation not permitted
		NoFileOrdir,         // No such file or directory
		InOutError,          // Input/output error
		BadFileDesc,         // Bad file descriptor
		ResourceUnavailible, // Resource temporarily unavailable
		InvalidArg,          // Invalid argument
		CannotAllocMem,      // Cannot allocate memory
		PermissionDenied,    // Permission denied
		Busy,				 // Device or resource busy
		FileDoesntExist,     // File exists
		BadAddr,             // Bad address
		CallInterrupted,     // Interrupted function call
		IsDir,				 // Is a directory
		TooManyOpenFiles,    // Too many open files
		NoSpaceLeft,         // No space left on device
		ReadOnlyFile,        // Read-only file system
		invalidSeek,         // Invalid seek
		InappropriateIo,     // Inappropriate I/O control operation
		ResultTooLarge,      // Result too large
		Unknown,			 // Unknown error
		OpNotSupported,      // Operation not supported
		OpWouldBlock,		 // Operation would block (same as EAGAIN)

		// Windows-specific mappings that don't have perfect POSIX equivalents
		WinInvalidHandle,	 // Special case for invalid handle

		Num
	};

#ifdef _WIN32
	using ErrorCodeType = unsigned long;
#else
	using ErrorCodeType = int;
#endif

	class ErrorMapper
	{
    private:
        static const std::unordered_map<ErrorCodeType, Error> s_errorMap;
        static const std::array<const std::string, static_cast<size_t>(Error::Num) + 1> s_errorStrings;

    public:
		static Error fromSystem();

        static Error convert(ErrorCodeType errorCode) {
            auto it = s_errorMap.find(errorCode);
            return (it != s_errorMap.end()) ? it->second : Error::Unknown;
        }

        static std::string toString(Error err) {
            return s_errorStrings[static_cast<size_t>(err)];
        }
	};
}