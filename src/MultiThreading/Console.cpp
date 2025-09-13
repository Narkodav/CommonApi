#include "MultiThreading/Console.h"

namespace MultiThreading
{
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