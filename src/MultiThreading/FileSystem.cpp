#include "../../include/MultiThreading/FileSystem.h"


namespace MultiThreading
{
    std::shared_ptr<std::shared_mutex> FileSystem::getFileMutex(const std::string& path)
    {
        auto access = fileMutexes.getWriteAccess();
        auto it = access->find(path);
        if (it == access->end())
        {
            auto [it, inserted] =
            access->emplace(path, std::make_shared<std::shared_mutex>());
            return it->second;
        }
        return it->second;
    }

    bool FileSystem::exists(const std::string& path)
    {
        std::lock_guard<std::shared_mutex> lock(directoryMutex);
        try {
            return fs::exists(path);
        }
        catch (const fs::filesystem_error&) {
            return false;
        }
    }

    bool FileSystem::isDirectory(const std::string& path)
    {
        std::lock_guard<std::shared_mutex> lock(directoryMutex);
        try {
            return fs::is_directory(path);
        }
        catch (const fs::filesystem_error&) {
            return false;
        }
    }

    std::vector<std::string> FileSystem::listDirectory(const std::string& path)
    {
        std::lock_guard<std::shared_mutex> lock(directoryMutex);
        std::vector<std::string> entries;

        try {
            for (const auto& entry : fs::directory_iterator(path)) {
                entries.push_back(entry.path().string());
            }
        }
        catch (const fs::filesystem_error& e) {
            throw;
        }

        return entries;
    }

    bool FileSystem::createDirectory(const std::string& path)
    {
        std::lock_guard<std::shared_mutex> lock(directoryMutex);
        try {
            return fs::create_directory(path);
        }
        catch (const fs::filesystem_error& e) {
            // Handle or log error
            return false;
        }
    }

    bool FileSystem::removeFile(const std::string& path)
    {
        std::lock_guard<std::shared_mutex> lock(directoryMutex);
        try {
            return fs::remove(path);
        }
        catch (const fs::filesystem_error& e) {
            // Handle or log error
            return false;
        }
    }

    void FileSystem::readFileByLine(const std::string& path, std::function<void(const std::string&)> lineProcessor)
    {
        std::shared_lock<std::shared_mutex> lock(*getFileMutex(path));

        try {
            std::ifstream file(path);
            if (!file.is_open()) {
                throw std::runtime_error("Could not open file: " + path);
            }

            std::string line;
            while (std::getline(file, line)) {
                lineProcessor(line);
            }
        }
        catch (const std::exception& e) {
            throw;
        }
    }

    void FileSystem::readFileBinaryBatched(const std::string& path,
        std::function<void(const char* data, size_t size)> batchProcessor,
        size_t batchSize /*= 8192*/)
    {
        std::shared_lock<std::shared_mutex> lock(*getFileMutex(path));

        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }

            std::vector<char> buffer(batchSize);
            while (file) {
                file.read(buffer.data(), batchSize);
                std::streamsize bytesRead = file.gcount();
                if (bytesRead > 0) {
                    batchProcessor(buffer.data(), bytesRead);
                }
            }
        }
        catch (const std::exception& e) {
            throw;
        }
    }

    std::vector<char> FileSystem::readFileBinaryRange(const std::string& path,
        size_t offset,
        size_t length)
    {
        std::shared_lock<std::shared_mutex> lock(*getFileMutex(path));

        try {
            std::ifstream file(path, std::ios::binary);
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }

            file.seekg(offset);
            std::vector<char> buffer(length);
            file.read(buffer.data(), length);

            // Resize to actual bytes read
            buffer.resize(file.gcount());
            return buffer;
        }
        catch (const std::exception& e) {
            throw;
        }
    }

    std::string FileSystem::readFileText(const std::string& path)
    {
        std::shared_lock<std::shared_mutex> lock(*getFileMutex(path));

        try {
            std::ifstream file(path);
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }

            return std::string(
                std::istreambuf_iterator<char>(file),
                std::istreambuf_iterator<char>()
            );
        }
        catch (const std::exception& e) {
            throw;
        }
    }


    std::vector<char> FileSystem::readFileBinary(const std::string& path)
    {
        std::shared_lock<std::shared_mutex> lock(*getFileMutex(path));

        try {
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }

            // Get file size and prepare buffer
            std::streamsize size = file.tellg();
            file.seekg(0);
            std::vector<char> buffer(size);

            // Read entire file
            if (!file.read(buffer.data(), size)) {
                throw std::runtime_error("Failed to read file: " + path);
            }

            return buffer;
        }
        catch (const std::exception& e) {
            throw;
        }
    }

    bool FileSystem::writeFileText(const std::string& path,
        const std::string& content,
        CreateMode mode /*= FileCreateMode::Truncate*/)
    {
        std::ofstream file;
        std::shared_lock<std::shared_mutex> fileLock(*getFileMutex(path), std::defer_lock);
        {
            std::lock_guard<std::shared_mutex> lock(directoryMutex);

            switch (mode)
            {
            case MultiThreading::FileSystem::CreateMode::Create:
                try {
                    if (fs::exists(path))
                        return false;
                    file.open(path, std::ios::app);
                }
                catch (const fs::filesystem_error&) {
                    return false;
                }
                break;
            case MultiThreading::FileSystem::CreateMode::Truncate:
                if (fs::exists(path))
                    file.open(path, std::ios::trunc);
                break;
            case MultiThreading::FileSystem::CreateMode::Append:
                try {
                    if (!fs::exists(path))
                        return false;
                    file.open(path, std::ios::app);
                }
                catch (const fs::filesystem_error&) {
                    return false;
                }
                break;
            default:
                file.open(path, std::ios::app);
                break;
            }
            fileLock.lock();
        }

        try {
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }
            file << content;  // Write the content
            if (file.fail()) {
                throw std::runtime_error("Failed to write to file: " + path);
            }
            file.close();  // Explicitly close to catch potential flush errors
            if (file.fail()) {
                throw std::runtime_error("Failed to close file: " + path);
            }
        }
        catch (const std::exception& e) {
            throw;
        }
    }

    bool FileSystem::writeFileBinary(const std::string& path,
        const std::vector<char>& content,
        CreateMode mode /*= FileCreateMode::Truncate*/)
    {
        std::ofstream file;
        std::shared_lock<std::shared_mutex> fileLock(*getFileMutex(path), std::defer_lock);
        {
            std::lock_guard<std::shared_mutex> lock(directoryMutex);

            switch (mode)
            {
            case MultiThreading::FileSystem::CreateMode::Create:
                try {
                    if (fs::exists(path))
                        return false;
                    file.open(path, std::ios::binary | std::ios::app);
                }
                catch (const fs::filesystem_error&) {
                    return false;
                }
                break;
            case MultiThreading::FileSystem::CreateMode::Truncate:
                if (fs::exists(path))
                    file.open(path, std::ios::binary | std::ios::trunc);
                break;
            case MultiThreading::FileSystem::CreateMode::Append:
                try {
                    if (!fs::exists(path))
                        return false;
                    file.open(path, std::ios::binary | std::ios::app);
                }
                catch (const fs::filesystem_error&) {
                    return false;
                }
                break;
            default:
                file.open(path, std::ios::binary | std::ios::app);
                break;
            }
            fileLock.lock();
        }

        try {
            if (!file) {
                throw std::runtime_error("Could not open file: " + path);
            }

            file.write(content.data(), content.size());  // Write binary data

            if (file.fail()) {
                throw std::runtime_error("Failed to write to file: " + path);
            }
            file.close();  // Explicitly close to catch potential flush errors
            if (file.fail()) {
                throw std::runtime_error("Failed to close file: " + path);
            }
        }
        catch (const std::exception& e) {
            throw;
        }
    }
}