#pragma once
#include "../Namespaces.h"

#include <unordered_map>
#include <string>
#include <array>

#ifdef _WIN32
#include <windows.h>
#else 
#include <unistd.h>
#endif

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

	class ErrorMapper
	{
    private:
#ifdef _WIN32
        static inline const std::unordered_map<DWORD, Error> s_errorMap = {
            {ERROR_SUCCESS,               Error::Ok},
            {ERROR_ACCESS_DENIED,         Error::PermissionDenied},
            {ERROR_FILE_NOT_FOUND,        Error::NoFileOrdir},
            {ERROR_PATH_NOT_FOUND,        Error::NoFileOrdir},
            {ERROR_TOO_MANY_OPEN_FILES,   Error::TooManyOpenFiles},
            {ERROR_INVALID_HANDLE,        Error::WinInvalidHandle},
            {ERROR_NOT_ENOUGH_MEMORY,     Error::CannotAllocMem},
            {ERROR_NO_MORE_FILES,         Error::NoFileOrdir},
            {ERROR_WRITE_PROTECT,         Error::ReadOnlyFile},
            {ERROR_SEEK,                  Error::invalidSeek},
            {ERROR_NOT_READY,             Error::Busy},
            {ERROR_INVALID_PARAMETER,     Error::InvalidArg},
            {ERROR_INVALID_FUNCTION,      Error::OpNotSupported},
            {ERROR_FILE_EXISTS,           Error::FileDoesntExist},
            {ERROR_DISK_FULL,             Error::NoSpaceLeft},
            {ERROR_BROKEN_PIPE,           Error::InOutError},
            {ERROR_OPERATION_ABORTED,     Error::CallInterrupted},
            {ERROR_IO_INCOMPLETE,         Error::InOutError},
            {ERROR_IO_PENDING,            Error::ResourceUnavailible},
            {ERROR_NOACCESS,              Error::BadAddr},
            {ERROR_LOCK_VIOLATION,        Error::PermissionDenied},
            {ERROR_SHARING_VIOLATION,     Error::PermissionDenied},
            {ERROR_HANDLE_EOF,            Error::InOutError},
            {ERROR_NOT_SUPPORTED,         Error::OpNotSupported},
            {ERROR_BAD_NETPATH,           Error::NoFileOrdir},
            {ERROR_BAD_NET_NAME,          Error::NoFileOrdir},
            {ERROR_ALREADY_EXISTS,        Error::FileDoesntExist},
            {ERROR_INSUFFICIENT_BUFFER,   Error::CannotAllocMem},
            {ERROR_BAD_COMMAND,           Error::InvalidArg},
            {ERROR_WRITE_FAULT,           Error::InOutError},
            {ERROR_READ_FAULT,            Error::InOutError},
            {ERROR_GEN_FAILURE,           Error::InOutError},
            {ERROR_DEV_NOT_EXIST,         Error::NoFileOrdir},
            {ERROR_INVALID_NAME,          Error::InvalidArg},
            {ERROR_DIR_NOT_EMPTY,         Error::Busy},
            {ERROR_DIRECTORY,             Error::IsDir},
            {ERROR_NOT_SAME_DEVICE,       Error::InvalidArg},
            {ERROR_PRIVILEGE_NOT_HELD,    Error::NotPermitted},
        };
#else
        static inline const std::unordered_map<int, Error> s_errorMap = {
            {0,            Error::OK},
            {EPERM,        Error::NotPermitted},
            {ENOENT,       Error::NoFileOrdir},
            {EIO,          Error::InOutError},
            {EBADF,        Error::BadFileDesc},
            {EAGAIN,       Error::ResourceUnavailible},
            {EINVAL,       Error::InvalidArg},
            {ENOMEM,       Error::CannotAllocMem},
            {EACCES,       Error::PermissionDenied},
            {EBUSY,        Error::Busy},
            {EEXIST,       Error::FileDoesntExist},
            {EFAULT,       Error::BadAddr},
            {EINTR,        Error::CallInterrupted},
            {EISDIR,       Error::IsDir},
            {EMFILE,       Error::TooManyOpenFiles},
            {ENOSPC,       Error::NoSpaceLeft},
            {EROFS,        Error::ReadOnlyFile},
            {ESPIPE,       Error::invalidSeek},
            {ENOTTY,       Error::InappropriateIo},
            {ERANGE,       Error::ResultTooLarge},
            {EOPNOTSUPP,   Error::OpNotSupported},
            {EWOULDBLOCK,  Error::OpWouldBlock},
        };
#endif
        static inline const std::array<const std::string, static_cast<size_t>(Error::Num) + 1> s_errorStrings = {
            "Success",
            "Operation not permitted",
            "No such file or directory",
            "Input/output error",
            "Bad file descriptor",
            "Resource temporarily unavailable",
            "Invalid argument",
            "Cannot allocate memory",
            "Permission denied",
            "Device or resource busy",
            "File exists",
            "Bad address",
            "Interrupted function call",
            "Is a directory",
            "Too many open files",
            "No space left on device",
            "Read-only file system",
            "Invalid seek",
            "Inappropriate I/O control operation",
            "Result too large",
            "Unknown error",
            "Operation not supported",
            "Operation would block",
            "Invalid handle",
            "Invalid error",
        };


    public:
        static Error fromSystem() {
#ifdef _WIN32
            return convert(GetLastError());
#else
            return convert(Error);
#endif
        }

        static Error convert(
#ifdef _WIN32
            DWORD errorCode
#else
            int errorCode
#endif
        ) {
            auto it = s_errorMap.find(errorCode);
            return (it != s_errorMap.end()) ? it->second : Error::Unknown;
        }

        static std::string toString(Error err) {
            return s_errorStrings[static_cast<size_t>(err)];
        }
	};
}