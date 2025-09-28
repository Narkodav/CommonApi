#pragma once
#include "../../include/PlatformAbstractions/ErrorMapper.h"

#ifdef _WIN32
#include <windows.h>
#else 
#include <unistd.h>
#endif

namespace Platform
{
#ifdef _WIN32
    const std::unordered_map<ErrorCodeType, Error> ErrorMapper::s_errorMap = {
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
    const std::unordered_map<ErrorCodeType, Error> ErrorMapper::s_errorMap = {
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
    const std::array<const std::string, static_cast<size_t>(Error::Num) + 1> ErrorMapper::s_errorStrings = {
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

    Error ErrorMapper::fromSystem() {
#ifdef _WIN32
        return convert(GetLastError());
#else
        return convert(Error);
#endif
    }
}