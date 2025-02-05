#pragma once
#include "../Namespaces.h"

#include <vector>
#include <mutex>

namespace MultiThreading
{
    template <typename T>
    class Vector
    {
    private:
        std::vector<T> m_vector;
        mutable std::mutex m_mutex;

    public:
        Vector() = default;

        // Add element
        void pushBack(const T& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.push_back(value);
        }

        void pushBack(T&& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.push_back(std::move(value));
        }

        // For non-bool types
        template<typename U = T>
        typename std::enable_if<!std::is_same<U, bool>::value, U&>::type
            operator[](size_t index) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // For bool type
        template<typename U = T>
        typename std::enable_if<std::is_same<U, bool>::value, typename std::vector<bool>::reference>::type
            operator[](size_t index) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Const access for non-bool types
        template<typename U = T>
        typename std::enable_if<!std::is_same<U, bool>::value, const U&>::type
            operator[](size_t index) const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Const access for bool type
        template<typename U = T>
        typename std::enable_if<std::is_same<U, bool>::value, bool>::type
            operator[](size_t index) const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector[index];
        }

        // Resize
        void resize(size_t newSize) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.resize(newSize);
        }

        void resize(size_t newSize, const T& value) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.resize(newSize, value);
        }

        // Get size
        size_t size() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector.size();
        }

        // Clear
        void clear() {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_vector.clear();
        }

        // Optional: Get underlying vector (careful with this!)
        std::vector<T> getCopy() const {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_vector;
        }
    };
}
