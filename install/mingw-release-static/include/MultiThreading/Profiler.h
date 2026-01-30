#pragma once
#include "../Namespaces.h"

#include <string>
#include <unordered_map>
#include <chrono>
#include <numeric>
#include <shared_mutex>

template <typename IdType = int>
class Profiler {
public:
    struct OperationStats {
        double totalTimeMs;
        double avgTimeMs;
        size_t calls;
        double minTimeMs;
        double maxTimeMs;
        double stdDev;
    };

    class ScopedTiming {
    private:
        Profiler& m_profiler;
        IdType m_profileId;
        std::string m_name;
        std::chrono::steady_clock::time_point m_startTime;

    public:
        ScopedTiming(Profiler& profiler, IdType profileId, const std::string& name)
            : m_profiler(profiler)
            , m_profileId(profileId)
            , m_name(name)
            , m_startTime(std::chrono::steady_clock::now())
        {
        }

        ~ScopedTiming() {
            auto duration = std::chrono::steady_clock::now() - m_startTime;
            std::lock_guard<std::shared_mutex> lock(m_profiler.m_mutex);
            m_profiler.m_timings[{m_profileId, m_name}].samples.push_back(duration);
        }

        // Delete copy/move operations
        ScopedTiming(const ScopedTiming&) = delete;
        ScopedTiming& operator=(const ScopedTiming&) = delete;
        ScopedTiming(ScopedTiming&&) = delete;
        ScopedTiming& operator=(ScopedTiming&&) = delete;
    };

private:
    struct TimingData {
        std::vector<std::chrono::duration<double>> samples;

        double getAverage() const {
            if (samples.empty()) return 0.0;
            auto total = std::accumulate(samples.begin(), samples.end(),
                std::chrono::duration<double>(0));
            return (total.count() * 1000.0) / samples.size();
        }

        double getTotal() const {
            auto total = std::accumulate(samples.begin(), samples.end(),
                std::chrono::duration<double>(0));
            return total.count() * 1000.0;
        }
    };

    struct PairHash {
        template <class T1, class T2>
        std::size_t operator()(const std::pair<T1, T2>& p) const {
            return std::hash<T1>{}(p.first) ^ (std::hash<T2>{}(p.second) << 1);
        }
    };

    std::unordered_map<std::pair<IdType, std::string>, TimingData, PairHash> m_timings;
    mutable std::shared_mutex m_mutex;

public:

    void reset() {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_timings.clear();
    }

    void reset(IdType profileId) {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        
        std::erase_if(m_timings, [profileId](const auto& pair) {
            return pair.first.first == profileId;
            });
    }

    std::unordered_map<std::string, OperationStats> getStats(IdType profileId) const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        std::unordered_map<std::string, OperationStats> stats;

        for (const auto& [key, timing] : m_timings) {
            if (key.first == profileId) {
                const auto& samples = timing.samples;
                if (samples.empty()) continue;

                // Calculate min/max
                auto [min_it, max_it] = std::minmax_element(samples.begin(), samples.end());
                double minTime = min_it->count() * 1000.0;
                double maxTime = max_it->count() * 1000.0;

                // Calculate standard deviation
                double mean = timing.getAverage();
                double variance = 0.0;
                for (const auto& sample : samples) {
                    double diff = (sample.count() * 1000.0) - mean;
                    variance += diff * diff;
                }
                variance /= samples.size();
                double stdDev = std::sqrt(variance);

                stats[key.second] = {
                    timing.getTotal(),
                    timing.getAverage(),
                    samples.size(),
                    minTime,
                    maxTime,
                    stdDev
                };
            }
        }
        return stats;
    }

    void printStats(IdType profileId) const {
        auto stats = getStats(profileId);

        std::cout << "\nProfiling Results for ID " << profileId << ":\n";
        std::cout << "==================\n";

        for (const auto& [name, stat] : stats) {
            std::cout << name << ":\n";
            std::cout << "  Total time: " << stat.totalTimeMs << "ms\n";
            std::cout << "  Calls: " << stat.calls << "\n";
            std::cout << "  Avg time: " << stat.avgTimeMs << "ms\n";
            std::cout << "  Min time: " << stat.minTimeMs << "ms\n";
            std::cout << "  Max time: " << stat.maxTimeMs << "ms\n";
            std::cout << "  Std Dev: " << stat.stdDev << "ms\n";
            std::cout << "==================\n";
        }
    }

    void printAllStats() const {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        std::unordered_set<int> profileIds;

        for (const auto& [key, _] : m_timings) {
            profileIds.insert(key.first);
        }

        for (const int& id : profileIds) {
            printStats(id);
        }
    }

    void timeOperation(IdType profileId, const std::string& name, std::function<void()> operation) {
        auto startTime = std::chrono::steady_clock::now();
        operation();
        auto endTime = std::chrono::steady_clock::now();
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        auto& timing = m_timings[{profileId, name}];
        timing.samples.push_back(endTime - startTime);
    }

    [[nodiscard]] auto timeOperationScoped(IdType profileId, const std::string& name) {
        return ScopedTiming(*this, profileId, name);
    }
};

