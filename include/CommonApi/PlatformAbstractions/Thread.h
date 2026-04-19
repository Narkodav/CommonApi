#pragma once
#include "CommonApi/Namespaces.h"
#include "CommonApi/PlatformAbstractions/ErrorMapper.h"

#include <functional>
#include <stdexcept>
#include <memory>

namespace Platform
{
    class Thread {
    private:
#ifdef _WIN32
        void* m_thread;
#else
        unsigned long m_thread;
#endif
    public:
        Thread() : m_thread(0) {}
        ~Thread() {
            join();
        }

        // Disallow copy
        Thread(const Thread&) = delete;
        Thread& operator=(const Thread&) = delete;

        // Allow move
        Thread(Thread&& other) noexcept {
            m_thread = other.m_thread;
            other.m_thread = 0;
        }

        Thread& operator=(Thread&& other) noexcept {
            if(this == &other) return *this;
            m_thread = other.m_thread;
            other.m_thread = 0;
            return *this;
        }

        // Start m_thread with any callable
        template<typename Callable>
        void start(Callable&& f) {
            if (m_thread) {
                throw std::runtime_error("Thread already started");
            }
            startInternal(std::make_unique<std::function<void()>>(std::forward<Callable>(f)));
        }

        template<typename Callable, typename... Args>
        void start(Callable&& f, Args&&... args) {
            if (m_thread) {
                throw std::runtime_error("Thread already started");
            }

            using FuncType = std::decay_t<Callable>;
            auto t = std::make_tuple(std::forward<Args>(args)...);
            auto func = std::make_unique<std::function<void()>>(
                [call = std::forward<FuncType>(f), t = std::move(t)]() mutable {
                    std::apply(call, t);
            });

            startInternal(func);
        }

        void join();
        void detach();

    private:
        void startInternal(std::unique_ptr<std::function<void()>> callable);
    };

}