#include "../../include/MultiThreading/Logger.h"

namespace MultiThreading
{
    void Logger::log(Level level, const std::string& message)
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        if (!shouldLog(level))
            return;
        auto access = buffer.getWriteAccess();
        if (access->add({ level, message, std::chrono::system_clock::now() }))
            flush(access);
    }

    void Logger::flush() //this one is force flush, called by the user
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto access = buffer.getWriteAccess();
        flush(access);
    }

    void Logger::flush(MultiThreading::Synchronized<MessageBuffer>::WriteAccess& access) //for automatic flushing
    {
        if (!(m_fileOutputEnabled || m_consoleOutputEnabled))
        {
            access->clear();
            return;
        }

        std::stringstream formattedBuffer;
        std::vector<LogMessage> log = access->getLog();
        access->clear();

        for (const auto& message : log)
            if (m_minimumConsoleOutputLevel <= message.level)
                formattedBuffer << formatMessage(message);

        if (m_fileOutputEnabled && m_fullFilePath != "")
            writeToFile(formattedBuffer.str());

        if (m_consoleOutputEnabled)
            writeToConsole(formattedBuffer.str());
    }

    // Helper functions
    std::string Logger::formatMessage(const LogMessage& message)
    {
        std::stringstream formatted;
        formatted << "[" << LevelNames[static_cast<int>(message.level)] << "] "
            << "[" << formatTimestamp(message) << "] "
            << message.message << "\n";
        return formatted.str();
    }

    void Logger::setLogFile(const std::string& path,
        const std::string& filename, const std::string& format)
    {
        std::lock_guard<std::shared_mutex> lock(m_mutex);
        m_logFilePath = path;
        m_logFileName = filename;
        m_logFileFormat = format;
        m_fullFilePath = m_logFilePath + "/" + m_logFileName + "." + m_logFileFormat;
    }

    std::string Logger::formatTimestamp(const LogMessage& message)
    {
        time_t currentTime;
        struct tm timeInfo;
        time(&currentTime);
        localtime_s(&timeInfo, &currentTime);

        std::stringstream ss;
        ss << std::put_time(&timeInfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    void Logger::writeToFile(const std::string& message)
    {
        FileSystem::writeFileText(m_fullFilePath, message,
            FileSystem::CreateMode::AppendNew);
    }

    void Logger::writeToConsole(const std::string& message)
    {
        std::lock_guard<std::mutex> lock(m_consoleMutex);
        std::cout << message << std::flush;
    }
}