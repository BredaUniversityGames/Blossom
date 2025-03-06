#pragma once
#include <core/fileio.hpp>
#include <resources/resource_handle.hpp>
#include <string_view>

#include <resources/model/model.hpp>

namespace bee {

struct MeshData
{
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> tangent;
    std::vector<float> uvs;

    std::vector<uint32_t> indices;
};

class ModelLoader
{
public:
    ModelLoader() = default;
    ~ModelLoader() = default;

    ModelLoader(const ModelLoader&) = delete;
    ModelLoader(ModelLoader&&) = delete;

    ResourceHandle<Model> FromGLTF(bee::FileIO::Directory directory, std::string_view path);

private:
    void CalculateBoundsRecursive(std::shared_ptr<Model> model, const std::vector<BoundingBox>& bounds, int nodeID, glm::mat4 parentTransform = glm::mat4(1.0f));
    uint32_t GetLodFromName(std::string_view name);
    std::vector<float> ComputeTangents(std::vector<uint32_t> indices,
        std::vector<float> positions, std::vector<float> texture_uvs);
};

}