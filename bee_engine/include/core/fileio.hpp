#pragma once
#include <map>
#include <fstream>
#include <string>
#include <vector>

namespace bee
{

/// <summary>
/// The FileIO class provides a cross-platform way to read and write files.
/// </summary>
class FileIO
{
public:

    /// <summary>
    /// Initializes the File IO system, filling in all the paths and
    /// mounting the virtual file system on platforms that need it.
    /// </summary>
    FileIO();

    /// <summary>
    /// Unmount the virtual file system and shut down the File IO system.
    /// </summary>
    ~FileIO();

    /// <summary>
    /// Types of paths that can be used. These can be very different on different platforms.
    /// </summary>
    enum class Directory
    {
        Shared,
        Asset,
        Save,
        None
    };

    /// <summary>
    /// Read a text file into a string. The string is empty if the file was not found.
    /// </summary>
    std::string ReadTextFile(Directory type, const std::string& path);

    /// <summary>
    /// Write a string to a text file. The file is created if it does not exist.
    /// Returns true if the file was written successfully.
    /// </summary>
    bool WriteTextFile(Directory type, const std::string& path, const std::string& content);

    /// <summary>
    /// Read a binary file into a string. The vector is empty if the file was not found.
    /// </summary>
    std::vector<char> ReadBinaryFile(Directory type, const std::string& path);

    /// <summary>
    /// Read a binary file into a string. The vector is empty if the file was not found.
    /// Assumes the path was already correctly indexed using GetPath() function
    /// </summary>
    std::vector<char> ReadBinaryFile(const std::string& path);

    /// <summary>
    /// Write a string to a binary file. The file is created if it does not exist.
    /// Returns true if the file was written successfully.
    /// </summary>
    bool WriteBinaryFile(Directory type, const std::string& path, const std::vector<char>& content);

    /// <summary>
    /// Get the full path of a file.
    /// </summary>
    std::string GetPath(Directory type, const std::string& path);

    /// <summary>
    /// Check if a file exists.
    /// </summary>
    bool Exists(Directory type, const std::string& path);

    /// <summary>
    /// Check the last time a file was modified. Only used on desktop platforms.
    /// </summary>
    uint64_t LastModified(Directory type, const std::string& path);

    /// <summary>
    /// Open a file stream for reading
    /// </summary>
    std::ifstream OpenReadStream(Directory type, const std::string& path);

    /// <summary>
    /// Open a file stream for writing, clearing everything in the file previously
    /// </summary>
    std::ofstream OpenWriteStream(Directory type, const std::string& path);

private:
    friend class EngineClass;

    // Roots of the paths per type, stored in a map.
    std::map<Directory, std::string> Paths;
};

}  // namespace bee
