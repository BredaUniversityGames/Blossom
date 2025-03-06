#pragma once
#include <core/fileio.hpp>
#include <resources/resource_handle.hpp>
#include <string_view>

namespace bee {

class Mesh;
class BoundingBox;

class MeshLoader
{
public:
    struct MeshData
    {
        std::vector<uint32_t> indices;
        std::vector<float> positions;
        std::vector<float> normals;
        std::vector<float> texture_uvs;
        std::vector<float> tangents;
        std::vector<float> displacement_uvs;
    };

    MeshLoader() = default;
    ~MeshLoader() = default;

    MeshLoader(const MeshLoader&) = delete;
    MeshLoader(MeshLoader&&) = delete;

    ResourceHandle<Mesh> FromRawData(
        std::string_view identifier,
        std::vector<uint32_t>&& indices,
        std::vector<float>&& positions,
        std::vector<float>&& normals,
        std::vector<float>&& texture_uvs,
        std::vector<float>&& tangents,
        std::vector<float>&& displacement_uvs,
        const BoundingBox& bounds
    );

    ResourceHandle<Mesh> FromRawData(
        std::string_view identifier,
        MeshData&& meshData,
        const BoundingBox& bounds
    );
};

}