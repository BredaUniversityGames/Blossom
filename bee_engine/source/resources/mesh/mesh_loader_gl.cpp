#include <precompiled/engine_precompiled.hpp>
#include <resources/mesh/mesh_gl.hpp>
#include <resources/mesh/mesh_loader.hpp>
#include <resources/mesh/mesh_common.hpp>
#include <glm/glm.hpp>
#include <math/geometry.hpp>
#include <cassert>

bee::ResourceHandle<bee::Mesh> bee::MeshLoader::FromRawData(
    std::string_view identifier, std::vector<uint32_t>&& indices,
    std::vector<float>&& positions, std::vector<float>&& normals,
    std::vector<float>&& texture_uvs, std::vector<float>&& tangents, 
    std::vector<float>&& displacement_uvs,
    const BoundingBox& bounds
    )
{
    MeshData meshData { 
        std::move(indices), 
        std::move(positions), 
        std::move(normals), 
        std::move(texture_uvs), 
        std::move(tangents), 
        std::move(displacement_uvs)
    };
    return FromRawData(identifier, std::move(meshData), bounds);
}

bee::ResourceHandle<bee::Mesh> bee::MeshLoader::FromRawData(
    std::string_view identifier,
    MeshData&& meshData,
    const BoundingBox& bounds
)
{
    GLuint indexHandle{};
    GLuint vaoHandle{};
    BufferArray vertexAttributes{};

    // Vertex Array Object
    glCreateVertexArrays(1, &vaoHandle);
    glBindVertexArray(vaoHandle);
    bee::LabelGL(GL_VERTEX_ARRAY, vaoHandle, std::string(identifier));

    uint32_t vertexCount = static_cast<uint32_t>(meshData.positions.size()) / 3;

    if (!meshData.positions.empty())
    {
        glCreateBuffers(1, &vertexAttributes[VertexAttributeIndex::POSITION]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[VertexAttributeIndex::POSITION]);
        bee::LabelGL(GL_BUFFER, vertexAttributes[VertexAttributeIndex::POSITION], std::string(identifier));

        glBufferData(GL_ARRAY_BUFFER, meshData.positions.size() * sizeof(float), meshData.positions.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VertexAttributeIndex::POSITION);

        glVertexAttribPointer(VertexAttributeIndex::POSITION, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        meshData.positions.clear();
    }

    if (!meshData.normals.empty())
    {
        glCreateBuffers(1, &vertexAttributes[VertexAttributeIndex::NORMAL]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[VertexAttributeIndex::NORMAL]);
        bee::LabelGL(GL_BUFFER, vertexAttributes[VertexAttributeIndex::NORMAL], std::string(identifier));

        glBufferData(GL_ARRAY_BUFFER, meshData.normals.size() * sizeof(float), meshData.normals.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VertexAttributeIndex::NORMAL);

        glVertexAttribPointer(VertexAttributeIndex::NORMAL, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
        meshData.normals.clear();
    }

    if (!meshData.texture_uvs.empty())
    {
        glCreateBuffers(1, &vertexAttributes[VertexAttributeIndex::TEXTURE0_UV]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[VertexAttributeIndex::TEXTURE0_UV]);
        bee::LabelGL(GL_BUFFER, vertexAttributes[VertexAttributeIndex::TEXTURE0_UV], std::string(identifier));

        glBufferData(GL_ARRAY_BUFFER, meshData.texture_uvs.size() * sizeof(float), meshData.texture_uvs.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VertexAttributeIndex::TEXTURE0_UV);

        glVertexAttribPointer(VertexAttributeIndex::TEXTURE0_UV, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
        meshData.texture_uvs.clear();
    }

    if (!meshData.tangents.empty())
    {
        glCreateBuffers(1, &vertexAttributes[VertexAttributeIndex::TANGENT]);
        glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[VertexAttributeIndex::TANGENT]);
        bee::LabelGL(GL_BUFFER, vertexAttributes[VertexAttributeIndex::TANGENT], std::string(identifier));

        glBufferData(GL_ARRAY_BUFFER, meshData.tangents.size() * sizeof(float), meshData.tangents.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(VertexAttributeIndex::TANGENT);

        glVertexAttribPointer(VertexAttributeIndex::TANGENT, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
        meshData.tangents.clear();
    }

    glCreateBuffers(1, &vertexAttributes[VertexAttributeIndex::DISPLACEMENT_UV]);
    glBindBuffer(GL_ARRAY_BUFFER, vertexAttributes[VertexAttributeIndex::DISPLACEMENT_UV]);
    bee::LabelGL(GL_BUFFER, vertexAttributes[VertexAttributeIndex::DISPLACEMENT_UV], std::string(identifier));

    meshData.displacement_uvs.clear();
    meshData.displacement_uvs.resize(vertexCount * 4, 0.0f);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 4 * sizeof(float), meshData.displacement_uvs.data(), GL_STATIC_DRAW);
    glEnableVertexAttribArray(VertexAttributeIndex::DISPLACEMENT_UV);

    glVertexAttribPointer(VertexAttributeIndex::DISPLACEMENT_UV, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    meshData.displacement_uvs.clear();

    glCreateBuffers(1, &indexHandle);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexHandle);
    bee::LabelGL(GL_BUFFER, indexHandle, std::string(identifier));

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshData.indices.size() * sizeof(uint32_t), meshData.indices.data(), GL_DYNAMIC_DRAW);

    glBindVertexArray(0);

    std::array<BufferArray, 3> attributes;
    attributes[0] = std::move(vertexAttributes);

    auto new_entry = std::make_shared<ResourceEntry<Mesh>>();
    new_entry->origin_path = std::string(identifier);
    new_entry->resource = std::make_shared<Mesh>(
        indexHandle, static_cast<uint32_t>(meshData.indices.size()), vertexCount, GL_UNSIGNED_INT, attributes, 1, vaoHandle, bounds
    );

    return { new_entry };
}

void bee::mesh_utils::GenerateDisplacementData(ResourceHandle<Mesh> handle, glm::mat4 modelMatrix, glm::vec3 a_minBounds, glm::vec3 a_maxBounds, float trunkPercentage, float distanceMaxBend)
{
    auto mesh = handle.Retrieve();

    for(uint32_t i = 0; i < mesh->lodCount; ++i)
    {
        GLuint positionVBO = mesh->attribute_buffers[i][VertexAttributeIndex::POSITION];
        int positionBufferSize = mesh->vertex_count * 3 * sizeof(float);
        std::vector<glm::vec3> positions{};
        positions.resize(mesh->vertex_count, glm::vec3(0.0f));

        glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
        glGetBufferSubData(GL_ARRAY_BUFFER, 0, positionBufferSize, positions.data());

        GLuint displacementVBO = mesh->attribute_buffers[i][VertexAttributeIndex::DISPLACEMENT_UV];
        int displacementBufferSize = mesh->vertex_count * 4 * sizeof(float);
        std::vector<glm::vec4> displacement{};
        displacement.resize(mesh->vertex_count, glm::vec4(0.0f));

        glm::vec3 tempMaxBounds = modelMatrix * glm::vec4(a_maxBounds, 1.0f);
        glm::vec3 tempMinBounds = modelMatrix * glm::vec4(a_minBounds, 1.0f);

        glm::vec3 maxBounds = glm::max(tempMaxBounds, tempMinBounds);
        glm::vec3 minBounds = glm::min(tempMaxBounds, tempMinBounds);

        glm::vec3 range = maxBounds - minBounds;
        glm::vec3 base = minBounds + range * 0.5f;
        base.z = minBounds.z;
        float trunkHeight = range.z * trunkPercentage;

        for (uint32_t i = 0; i < mesh->vertex_count; i += 1)
        {
            glm::vec3 position = glm::vec3(modelMatrix * glm::vec4(positions[i], 1.0f));

            glm::vec3 pivotPoint = base;
            pivotPoint.z = glm::min(position.z, trunkHeight + minBounds.z);
            glm::vec3 pivot = glm::vec3(glm::vec4(position, 1.0f)) - pivotPoint;
            float distance = glm::length(pivot);
            float windModifier = glm::clamp(distance / distanceMaxBend, 0.0f, 1.0f);

            displacement[i] = glm::vec4(pivot, windModifier);
        }

        glBindVertexArray(mesh->vao_handle);

        glBindBuffer(GL_ARRAY_BUFFER, displacementVBO);
        glBufferData(GL_ARRAY_BUFFER, displacementBufferSize, displacement.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
}