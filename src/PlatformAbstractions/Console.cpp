#include "PlatformAbstractions/Console.h"

#ifdef _WIN32
#include <windows.h>
#else 
#include <unistd.h>
#endif

namespace MultiThreading
{
#ifdef _WIN32
    const std::array<Console::HandleType, static_cast<size_t>(Console::CallType::Num)> Console::s_handles = {
    GetStdHandle(STD_INPUT_HANDLE),
    GetStdHandle(STD_OUTPUT_HANDLE),
    GetStdHandle(STD_ERROR_HANDLE),
    };
#else 
    const std::array<Console::HandleType, static_cast<size_t>(Console::CallType::Num)> Console::s_handles = {
    STDIN_FILENO,
    STDOUT_FILENO,
    STDERR_FILENO,
    };
#endif

    size_t Console::write(CallType callType, const void* buf, size_t count, Platform::Error& error) {
        const auto& h = s_handles[static_cast<size_t>(callType)];

#ifdef _WIN32
        if (h == INVALID_HANDLE_VALUE || h == NULL) {
            error = Platform::Error::WinInvalidHandle;
            return 0;
        }

        DWORD bytesWritten = 0;
        if (!WriteFile(h, buf, static_cast<DWORD>(count), &bytesWritten, nullptr)) {
            error = Platform::ErrorMapper::fromSystem();
        }
        error = Platform::Error::Ok;
        return bytesWritten;
#else
        ssize_t result = ::write(h, buf, count);

        if (result == -1) {
            error = Platform::ErrorMapper::fromSystem();
        }
        error = Platform::Error::Ok;
        return result;
#endif
    }

    size_t Console::read(CallType callType, void* buf, size_t count, Platform::Error& error)
    {
        const auto& h = s_handles[static_cast<size_t>(callType)];

#ifdef _WIN32
        if (h == INVALID_HANDLE_VALUE || h == NULL) {
            error = Platform::Error::WinInvalidHandle;
            return 0;
        }

        DWORD bytesRead = 0;
        if (!ReadFile(h, buf, static_cast<DWORD>(count), &bytesRead, nullptr)) {
            error = Platform::ErrorMapper::fromSystem();
        }
        error = Platform::Error::Ok;
        return bytesRead;
#else
        ssize_t result = ::read(h, buf, count);

        if (result == -1) {
            error = Platform::ErrorMapper::fromSystem();
        }
        error = Platform::Error::Ok;
        return result;
#endif
    }
}