#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "CommonApi/PlatformAbstractions/Thread.h"

namespace Platform
{
#ifdef _WIN32
    static DWORD WINAPI threadEntry(LPVOID arg)
#else
    static void* threadEntry(void* arg)
#endif
    {
        std::unique_ptr<std::function<void()>> func(reinterpret_cast<std::function<void()>*>(arg));
        (*func)();
        func.reset();
        return 0;
    }

    void Thread::startInternal(std::unique_ptr<std::function<void()>> callable) {
#ifdef _WIN32
        m_thread = CreateThread(
            nullptr,
            0,
            &threadEntry,
            callable.get(),
            0,
            nullptr
        );

        if(!m_thread) throw Exception(ErrorMapper::fromSystem());
#else
        auto error = pthread_create(&m_thread, nullptr, &threadEntry, callable.get());
        if (!error)
        {
            m_thread = 0;
            throw Exception(ErrorMapper::convert(error));
        }
#endif

        callable.release();
    }

    void Thread::join() {
        if (m_thread) {
#ifdef _WIN32
            if(WaitForSingleObject(m_thread, INFINITE) == WAIT_FAILED) {
                throw Exception(ErrorMapper::fromSystem());
            }
            if(!CloseHandle(m_thread)) {
                throw Exception(ErrorMapper::fromSystem());
            }
#else
            pthread_join(m_thread, nullptr);
#endif
            m_thread = 0;
        }
    }

    void Thread::detach() {
        if (m_thread) {
#ifdef _WIN32
            if(!CloseHandle(m_thread)) {
                throw Exception(ErrorMapper::fromSystem());
            }
#else
            pthread_detach(m_thread); // avoid leaks
#endif
            m_thread = 0;
        }
    }
}