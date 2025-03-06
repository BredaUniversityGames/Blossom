#include <precompiled/engine_precompiled.hpp>
#include <code_utils/bee_utils.hpp>


#if defined(BEE_PLATFORM_PC)
#include <filesystem>
#endif

#include "core/fileio.hpp"
#include "tools/log.hpp"

using namespace bee;
using namespace std;

std::string FileIO::ReadTextFile(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return string();
    }
    file.seekg(0, std::ios::end);
    const size_t size = file.tellg();
    string buffer(size, '\0');
    file.seekg(0);
    file.read(&buffer[0], size);
    return buffer;
}

bool FileIO::WriteTextFile(Directory type, const std::string& path, const std::string& content)
{
    const auto fullPath = GetPath(type, path);
    ofstream file(fullPath);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return false;
    }
    file << content;
    file.close();
    return true;
}

std::vector<char> FileIO::ReadBinaryFile(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    return ReadBinaryFile(fullPath);
}

std::vector<char> bee::FileIO::ReadBinaryFile(const std::string& path) { 
    ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, path);
        return vector<char>();
    }
    const streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (file.read(buffer.data(), size)) return buffer;
    BEE_ASSERT(false);
    return vector<char>();
}

bool FileIO::WriteBinaryFile(Directory type, const std::string& path, const std::vector<char>& content)
{
    const auto fullPath = GetPath(type, path);
    ofstream file(fullPath, std::ios::binary);
    if (!file.is_open())
    {
        Log::Error("File {} with full path {} was not found!", path, fullPath);
        return false;
    }
    file.write(content.data(), content.size());
    file.close();
    return true;
}

std::string FileIO::GetPath(Directory type, const std::string& path)
{
    // Get path taking the type into account.
    // TODO: Handle slashes and backslashes correctly for all platforms.

    auto append = Paths[type] + path;
    std::replace(append.begin(), append.end(), '\\', '/');
    return append;
}

bool FileIO::Exists(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    ifstream f(fullPath.c_str());
    auto good = f.good();
    f.close();
    return good;
}

#ifdef PLATFORM_DEKSTOP
uint64_t FileIO::LastModified(Type type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    std::filesystem::file_time_type ftime = std::filesystem::last_write_time(fullPath);
    return static_cast<uint64_t>(ftime.time_since_epoch().count());
}
#else
uint64_t FileIO::LastModified(Directory type, const std::string& path) { return 0; }
std::ifstream bee::FileIO::OpenReadStream(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    return ifstream(fullPath);
}
std::ofstream bee::FileIO::OpenWriteStream(Directory type, const std::string& path)
{
    const auto fullPath = GetPath(type, path);
    return ofstream(fullPath, std::ios::trunc);
}
#endif  // PLATFORM_DEKSTOP
