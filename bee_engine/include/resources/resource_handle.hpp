#pragma once
#include <memory>
#include <string>

namespace bee {

template <typename T>
class ResourceEntry
{
public:
    std::string origin_path{};
    std::shared_ptr<T> resource;
};

//TODO: create monadic function Use() that takes a lambda and automatically determine resources status
template <typename T>
class ResourceHandle
{
    using Entry = ResourceEntry<T>;

public:
    ResourceHandle() = default;
    ResourceHandle(std::shared_ptr<Entry> bound_entry) : bound_entry(bound_entry) {}

    // Retrieves the parameters used to create the resource (useful to reload a resource that has been unloaded)
    std::string GetPath() const
    {
        if (bound_entry) return bound_entry->origin_path;
        return {};
    }

    void SetPath(const std::string& path) {
        if (bound_entry) bound_entry->origin_path = path;
    }

    // Retrieves a shared_ptr pointing to the resource bound.
    std::shared_ptr<T> Retrieve() const
    {
        if (bound_entry) return bound_entry->resource;
        return nullptr;
    }

    //Checks if the handle is bound to any resource slot
    bool Valid() const { return bound_entry.operator bool(); }

private:
    std::shared_ptr<Entry> bound_entry;
};

//Forward declare for all resource types

class Image;
class Image3D;
class Mesh;
class Model;
class Material;
class Shader;

}
