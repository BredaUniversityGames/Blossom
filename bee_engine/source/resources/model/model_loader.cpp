#include <precompiled/engine_precompiled.hpp>
#include <resources/resource_manager.hpp>
#include <resources/mesh/mesh_loader.hpp>
#include <resources/model/model_loader.hpp>
#include <resources/image/image_loader.hpp>
#include <math/geometry.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

#include <core/engine.hpp>
#include <tools/log.hpp>

#include <jolt/Physics/Collision/Shape/MeshShape.h>

#define TINYGLTF_USE_CPP14
#include <tinygltf/tiny_gltf.h>

//Helpers

bee::Sampler::Filter get_filter(int val) {
    switch (val)
    {
    case TINYGLTF_TEXTURE_FILTER_NEAREST:
        return bee::Sampler::Filter::Nearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR:
        return bee::Sampler::Filter::Linear;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
        return bee::Sampler::Filter::NearestMipmapNearest;
    case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
        return bee::Sampler::Filter::LinearMipmapNearest;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
        return bee::Sampler::Filter::NearestMipmapLinear;
    case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
        return bee::Sampler::Filter::LinearMipmapLinear;
    default:
        return bee::Sampler::Filter::Nearest;
    }
}

bee::Sampler::Wrap get_wrap(int val)
{
    switch (val)
    {
    case TINYGLTF_TEXTURE_WRAP_REPEAT:
        return bee::Sampler::Wrap::Repeat;
    case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
        return bee::Sampler::Wrap::ClampToEdge;
    case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
        return bee::Sampler::Wrap::MirroredRepeat;
    default:
        return bee::Sampler::Wrap::ClampToEdge;
    }
}

bee::Sampler from_tinygltf_sampler(const tinygltf::Sampler& tinygltf_sampler)
{
    bee::Sampler sampler{};
    sampler.MagFilter = get_filter(tinygltf_sampler.magFilter);
    sampler.MinFilter = get_filter(tinygltf_sampler.minFilter);
    sampler.WrapS = get_wrap(tinygltf_sampler.wrapS);
    sampler.WrapT = get_wrap(tinygltf_sampler.wrapT);
    return sampler;
}

uint32_t calculate_stride(const tinygltf::Accessor& accessor) {
    uint32_t elementSize = 0;
    switch (accessor.componentType)
    {
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
    case TINYGLTF_COMPONENT_TYPE_BYTE:
        elementSize = 1;
        break;
    case TINYGLTF_COMPONENT_TYPE_SHORT:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        elementSize = 2;
        break;
    case TINYGLTF_COMPONENT_TYPE_FLOAT:
    case TINYGLTF_COMPONENT_TYPE_INT:
    case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        elementSize = 4;
        break;
    default:
        assert(false);
    }

    switch (accessor.type)
    {
    case TINYGLTF_TYPE_MAT2:
        return 4 * elementSize;
    case TINYGLTF_TYPE_MAT3:
        return 9 * elementSize;
    case TINYGLTF_TYPE_MAT4:
        return 16 * elementSize;
    case TINYGLTF_TYPE_SCALAR:
        return elementSize;
    case TINYGLTF_TYPE_VEC2:
        return 2 * elementSize;
    case TINYGLTF_TYPE_VEC3:
        return 3 * elementSize;
    case TINYGLTF_TYPE_VEC4:
        return 4 * elementSize;
    default:
        assert(false);
    }

    return 0;
}

std::vector<float> extract_attribute(
    const tinygltf::Model& document, const tinygltf::Primitive& primitive, const std::string& attribute_name,
    glm::vec3* min = nullptr, glm::vec3* max = nullptr
) {

    std::vector<float> data;
    if (primitive.attributes.count(attribute_name)) {

        const auto& accessor = document.accessors[primitive.attributes.find(attribute_name)->second];
        const auto& view = document.bufferViews[accessor.bufferView];
        const auto& buffer = document.buffers[view.buffer];

        data.resize(accessor.count * calculate_stride(accessor) / 4);
        memcpy(data.data(), &buffer.data.at(view.byteOffset + accessor.byteOffset), accessor.count * calculate_stride(accessor));

        if (min) {
            min->x = (float)accessor.minValues[0]; min->y = (float)accessor.minValues[1]; min->z = (float)accessor.minValues[2];
        }

        if (max) {
            max->x = (float)accessor.maxValues[0]; max->y = (float)accessor.maxValues[1]; max->z = (float)accessor.maxValues[2];
        }
    }

    return data;
}



bee::ResourceHandle<bee::Model> bee::ModelLoader::FromGLTF(bee::FileIO::Directory directory, std::string_view path)
{
	auto fullpath = bee::Engine.FileIO().GetPath(directory, std::string(path));
	
    tinygltf::TinyGLTF loader;
    tinygltf::Model gltfModel;

    std::string err;
    std::string warn;
    bool result = false;

    // Check which format to load (.glb or gltf)
    if (path.find(".gltf") != std::string::npos)
    {
        result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, fullpath);
    }
    else if (path.find(".glb") != std::string::npos)
    {
        result = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, fullpath);
    }

    if (!warn.empty()) bee::Log::Warn(warn);

    if (!err.empty()) bee::Log::Error(err);

    if (!result) {
        bee::Log::Error("Failed to load glTF: {}", fullpath);
        throw std::runtime_error("TINY_GLTF_ERROR");
    }
    else
        bee::Log::Info("Loaded glTF: {}", fullpath);

    // Create a copy of all the nodes in the model, using pointers.
    std::vector<tinygltf::Node*> nodesCopy{ gltfModel.nodes.size() };
    std::for_each(nodesCopy.begin(), nodesCopy.end(), [idx = 0, &gltfModel](tinygltf::Node*& node) mutable { node = &gltfModel.nodes[idx++]; });

    // Sort the pointer to nodes list.
    std::sort(nodesCopy.begin(), nodesCopy.end(), [](tinygltf::Node*& a, tinygltf::Node*& b)
    {
        return a->name < b->name;
    });

    //Model elements
    std::vector<int> rootNodes = gltfModel.scenes[gltfModel.defaultScene].nodes;

    std::vector<ModelNode> nodes;

    std::vector<ResourceHandle<Material>> materials;
    std::vector<std::vector<PrimitiveSet>> meshes;
    std::vector<ColliderGroup> colliderGroups;

    //Load all textures
    std::vector<ResourceHandle<Image>> allImages;

    for (auto& texture : gltfModel.images) {


        if (texture.width == -1)
        {
            allImages.emplace_back();
            continue;
        }

        auto newImage = bee::Engine.Resources().Images().FromRawData(
            texture.image.data(), ImageFormat::RGBA8, texture.width, texture.height
        );
        allImages.emplace_back(newImage);
    }

    struct LODMesh
    {
        std::array<uint32_t, 3> meshes;
        uint32_t count{ 0 };
    };

    std::vector<LODMesh> gltfLodMeshes{};

    // Go through the nodes and collect LOD meshes, since the nodes contain the correct LOD convention name.
    // We update the mesh reference on the node to point to the correct one.
    for (auto& gltfNode : nodesCopy)
    {
        if(gltfNode->mesh == -1)
            continue;

        uint32_t lodLevel{ GetLodFromName(gltfNode->name) };

        if (lodLevel == 0)
        {
            LODMesh lodMesh{};
            lodMesh.count = 1;
            lodMesh.meshes[0] = gltfNode->mesh;
            gltfLodMeshes.push_back(lodMesh);
        }
        else
        {
            LODMesh& lodMesh = gltfLodMeshes[gltfLodMeshes.size() - 1];
            lodMesh.count++;
            lodMesh.meshes[lodLevel] = gltfNode->mesh;
        }
        gltfNode->mesh = gltfLodMeshes.size() - 1;
    }

    // Since we reduced the amount of meshes to collect them between LOD structures, the original indices won't match anymore.
    // This collection will contain the new indices to point to the correct meshes.
    std::vector<int32_t> newMeshIndices{};

    std::vector<BoundingBox> boundingBoxes{};

    //Load all primitive sets
    int mesh_num = 0;
    for (auto& lodMesh : gltfLodMeshes) 
    {

        std::vector<PrimitiveSet> primitiveSets{};
        ColliderGroup colliderGroup{};

        for(uint32_t i = 0; i < lodMesh.count; ++i)
        {
            PrimitiveSet primitiveSet{};

            std::string name = gltfModel.meshes[lodMesh.meshes[i]].name;
            if (name.empty()) name = "mesh" + std::to_string(mesh_num++);

            tinygltf::Mesh mesh = gltfModel.meshes[lodMesh.meshes[i]];
            int primitiveNum = 0;
            for (auto& primitive : mesh.primitives)
            {
                // Get attribute data from GLTF
                std::vector<uint32_t> indices{};

                // Index buffer
                const auto& accessor = gltfModel.accessors[primitive.indices];
                const auto& view = gltfModel.bufferViews[accessor.bufferView];
                const auto& buffer = gltfModel.buffers[view.buffer];

                if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
                {
                    indices.resize(accessor.count);
                    memcpy(indices.data(), &buffer.data.at(view.byteOffset + accessor.byteOffset), accessor.count * sizeof(uint32_t));
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
                {
                    std::vector<uint16_t> temp;
                    temp.resize(accessor.count);
                    memcpy(temp.data(), &buffer.data.at(view.byteOffset + accessor.byteOffset), accessor.count * sizeof(uint16_t));

                    indices.reserve(temp.size());
                    for (auto& i : temp) indices.push_back(i);
                }
                else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
                {
                    std::vector<uint8_t> temp;
                    temp.resize(accessor.count);
                    memcpy(temp.data(), &buffer.data.at(view.byteOffset + accessor.byteOffset), accessor.count * sizeof(uint8_t));

                    indices.reserve(temp.size());
                    for (auto& i : temp) indices.push_back(i);
                }
                else
                {
                    assert(false);
                }

                glm::vec3 meshMax, meshMin;

                std::vector<float> positions = extract_attribute(gltfModel, primitive, "POSITION", &meshMin, &meshMax);
                std::vector<float> normals = extract_attribute(gltfModel, primitive, "NORMAL");
                std::vector<float> tangents = extract_attribute(gltfModel, primitive, "TANGENT");
                std::vector<float> uvs = extract_attribute(gltfModel, primitive, "TEXCOORD_0");

                if (tangents.empty()) tangents = ComputeTangents(indices, positions, uvs);

                auto extents = (meshMax - meshMin) * 0.5f;

                std::string mesh_name =
                    fullpath
                    + "/"
                    + name
                    + "_prim_" + std::to_string(primitiveNum++);

                if (i == lodMesh.count - 1)
                {
                    JPH::TriangleList triangles;
                    triangles.reserve(indices.size() / 3);

                    for (int indexIndex = 0; indexIndex < indices.size(); indexIndex += 3)
                    {
                        int v1index = indices[indexIndex] * 3;
                        int v2index = indices[indexIndex + 1] * 3;
                        int v3index = indices[indexIndex + 2] * 3;
                    
                        JPH::Float3 v1 = JPH::Float3(positions[v1index], positions[v1index + 1], positions[v1index + 2]);
                        JPH::Float3 v2 = JPH::Float3(positions[v2index], positions[v2index + 1], positions[v2index + 2]);
                        JPH::Float3 v3 = JPH::Float3(positions[v3index], positions[v3index + 1], positions[v3index + 2]);
                    
                        JPH::Triangle triangle(v1, v2, v3);
                    
                        triangles.push_back(triangle);
                    }
                    
                    JPH::MeshShapeSettings shapeSettings(triangles);
                    shapeSettings.SetEmbedded();
                    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();

                    if (shapeResult.HasError())
                    {
                        colliderGroup.colliders.push_back(std::nullopt);
                        Log::Warn("Jolt error creating convex hull: {}", shapeResult.GetError().c_str());
                    }
                    else
                    {
                        colliderGroup.colliders.push_back(shapeResult.Get());
                    }
                }

                auto primHandle = bee::Engine.Resources().Meshes().FromRawData(
                    mesh_name, std::move(indices), std::move(positions), std::move(normals), std::move(uvs), std::move(tangents), {},
                    BoundingBox(meshMin + extents, extents)
                );

                boundingBoxes.emplace_back(BoundingBox(meshMin + extents, extents));

                primitiveSet.primitiveMaterialPairs.emplace_back(
                    primHandle, primitive.material
                );
            }
            primitiveSets.push_back(primitiveSet);
        }

        colliderGroups.push_back(colliderGroup);

        meshes.emplace_back(primitiveSets);
        newMeshIndices.insert(newMeshIndices.end(), lodMesh.count, meshes.size() - 1);
    }

    // Load all materials
    for (auto& material : gltfModel.materials) {
        
        int subsurfaceTextureIndex = -1;
        float subsurfaceFactor = 1;

        // We are reusing KHR_materials_transmission extension for the subsurface map and factor 
        if (material.extensions.find("KHR_materials_transmission") != material.extensions.end())
        {
            auto transmission = material.extensions.find("KHR_materials_transmission");
            if (transmission->second.Has("transmissionTexture"))
            {
                auto transmissionImageIndex = transmission->second.Get("transmissionTexture").Get("Index");
                subsurfaceTextureIndex = transmissionImageIndex.Get<int>();
            }
            if (transmission->second.Has("transmissionFactor"))
                subsurfaceFactor = transmission->second.Get("transmissionFactor").GetNumberAsDouble();
        }

        int textureIndices[TextureSlotIndex::MAX_TEXTURES] = {
            material.pbrMetallicRoughness.baseColorTexture.index,
            material.emissiveTexture.index,
            material.normalTexture.index,
            material.occlusionTexture.index,
            material.pbrMetallicRoughness.metallicRoughnessTexture.index,
            subsurfaceTextureIndex,
        };

        auto& baseFactorVec = material.pbrMetallicRoughness.baseColorFactor;
        auto& emissiveFactorVec = material.emissiveFactor;

        glm::vec4 texture_factors[TextureSlotIndex::MAX_TEXTURES] = {
            glm::vec4(glm::make_vec4(baseFactorVec.data())),
            { emissiveFactorVec[0], emissiveFactorVec[1], emissiveFactorVec[2], 0.0f },
            glm::vec4(static_cast<float>(material.normalTexture.scale)),
            glm::vec4(static_cast<float>(material.occlusionTexture.strength)),
            { material.pbrMetallicRoughness.metallicFactor, material.pbrMetallicRoughness.roughnessFactor, 0.0f, 0.0f},
            glm::vec4(static_cast<float>(subsurfaceFactor)),
        };

        auto builder = MaterialBuilder();

        for (uint32_t i = 0; i < TextureSlotIndex::MAX_TEXTURES; i++)
        {
            if (textureIndices[i] != -1) 
            {
                auto& texture = gltfModel.textures[textureIndices[i]];
                builder.WithTexture(i, allImages[texture.source])
                       .WithSampler(i, from_tinygltf_sampler(gltfModel.samplers[texture.sampler]));
            }
            builder.WithFactor(i, texture_factors[i]);
        }
        builder.DoubleSided(material.doubleSided);

        materials.emplace_back(builder.Build());
    }

    //Load scene nodes 
    for (size_t i = 0, counter = 0; i < gltfModel.nodes.size(); ++i)
    {
        auto& gltfNode = gltfModel.nodes[i];
        uint32_t lodLevel{ GetLodFromName(gltfNode.name) };

        ModelNode node;
        node.lodLevel = lodLevel;

        node.children = gltfNode.children;

        node.meshIndex = gltfNode.mesh;

        if (!gltfNode.matrix.empty())
        {
            glm::vec4 perspective_temp;
            glm::vec3 skew_temp;

            glm::mat4 matrix = glm::make_mat4(gltfNode.matrix.data());
            glm::decompose(matrix, node.scale, node.rotation, node.translation, skew_temp, perspective_temp);
        }
        else
        {
            if (!gltfNode.scale.empty())
                node.scale = glm::vec3(
                    static_cast<float>(gltfNode.scale[0]), 
                    static_cast<float>(gltfNode.scale[1]),
                    static_cast<float>(gltfNode.scale[2])
                );

            if (!gltfNode.rotation.empty())
                node.rotation = glm::quat(
                    static_cast<float>(gltfNode.rotation[3]),
                    static_cast<float>(gltfNode.rotation[0]),
                    static_cast<float>(gltfNode.rotation[1]),
                    static_cast<float>(gltfNode.rotation[2])
                );

            if (!gltfNode.translation.empty())
                node.translation = glm::vec3(
                    static_cast<float>(gltfNode.translation[0]),
                    static_cast<float>(gltfNode.translation[1]),
                    static_cast<float>(gltfNode.translation[2])
                );
        }

        nodes.emplace_back(node);
    }

    auto newEntry = std::make_shared<ResourceEntry<Model>>();
    newEntry->origin_path = std::string(path);
    newEntry->resource = std::make_shared<Model>(
        std::move(rootNodes),
        std::move(nodes),
        std::move(meshes),
        std::move(colliderGroups),
        std::move(materials),
        glm::vec3(0.0f),
        glm::vec3(0.0f)
    );

    for (auto r : newEntry->resource->rootNodes)
    {
        CalculateBoundsRecursive(newEntry->resource, boundingBoxes, r);
    }

    return { newEntry };
}

void bee::ModelLoader::CalculateBoundsRecursive(std::shared_ptr<Model> model, const std::vector<BoundingBox>& bounds, int nodeID, glm::mat4 parentTransform)
{
    auto& node = model->nodes[nodeID];
    const auto translation = glm::translate(glm::mat4(1.0f), node.translation);
    const auto rotation = glm::mat4_cast(node.rotation);
    const auto scale = glm::scale(glm::mat4(1.0f), node.scale);
    glm::mat4 transform = parentTransform * translation * rotation * scale;

    if (node.meshIndex != -1)
    {
        auto& meshBounds = bounds[node.meshIndex];
        auto bb = meshBounds.ApplyTransform(transform);

        model->minBounds = glm::min(model->minBounds, bb.GetStart());
        model->maxBounds = glm::max(model->maxBounds, bb.GetEnd());
    }

    for (auto child : model->nodes[nodeID].children)
    {
        CalculateBoundsRecursive(model, bounds, child, transform);
    }
}

uint32_t bee::ModelLoader::GetLodFromName(std::string_view name)
{
    uint32_t lodLevel{ 0 };
    size_t offset = name.find("LOD", 0);
    if (offset != std::string::npos) // There is LOD in the name.
    {
        std::string_view lodSuffix = name.substr(offset);
        if (lodSuffix.length() == 4)
        {
            lodLevel = lodSuffix[3] - '0'; // No LOD is expected to be higher than 3, so we can get away with this.
        }
    }

    return lodLevel;
}

std::vector<float> bee::ModelLoader::ComputeTangents(std::vector<uint32_t> indices,
    std::vector<float> positions, std::vector<float> texture_uvs)
{
    uint32_t vertexCount = positions.size() / 3;
    std::vector<float> tangentData(vertexCount * 4);
    std::fill(tangentData.begin(), tangentData.end(), 0);
    for (uint32_t i = 0; i < indices.size(); i += 3)
    {
        const int index1 = indices[i];
        const int index2 = indices[i + 1];
        const int index3 = indices[i + 2];

        glm::vec3 pos1(positions[index1 * 3], positions[index1 * 3 + 1], positions[index1 * 3 + 2]);
        glm::vec3 pos2(positions[index2 * 3], positions[index2 * 3 + 1], positions[index2 * 3 + 2]);
        glm::vec3 pos3(positions[index3 * 3], positions[index3 * 3 + 1], positions[index3 * 3 + 2]);

        glm::vec2 uv1(texture_uvs[index1 * 2], texture_uvs[index1 * 2 + 1]);
        glm::vec2 uv2(texture_uvs[index2 * 2], texture_uvs[index2 * 2 + 1]);
        glm::vec2 uv3(texture_uvs[index3 * 2], texture_uvs[index3 * 2 + 1]);

        glm::vec3 edge1 = pos2 - pos1;
        glm::vec3 edge2 = pos3 - pos1;
        glm::vec2 deltaUV1 = uv2 - uv1;
        glm::vec2 deltaUV2 = uv3 - uv1;

        float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

        tangentData[index1 * 4] += f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangentData[index1 * 4 + 1] += f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangentData[index1 * 4 + 2] += f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangentData[index1 * 4 + 3] = f > 0.0f ? 1.0f : -1.0f;

        tangentData[index2 * 4] += f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangentData[index2 * 4 + 1] += f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangentData[index2 * 4 + 2] += f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangentData[index2 * 4 + 3] = f > 0.0f ? 1.0f : -1.0f;

        tangentData[index3 * 4] += f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tangentData[index3 * 4 + 1] += f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tangentData[index3 * 4 + 2] += f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        tangentData[index3 * 4 + 3] = f > 0.0f ? 1.0f : -1.0f;
    }

    for (uint32_t i = 0; i < vertexCount; i++)
    {
        glm::vec4 tangent = { tangentData[i * 4 + 0], tangentData[i * 4 + 1], tangentData[i * 4 + 2], tangentData[i * 4 + 3] };

        glm::vec3 tangent_vec3(tangent);

        tangent_vec3 = glm::normalize(tangent_vec3);

        tangent.w = tangent.w > 0.0f ? -1.0f : 1.0f;

        tangent = glm::vec4(tangent_vec3, tangent.w);

        tangentData[i * 4 + 0] = tangent.x;
        tangentData[i * 4 + 1] = tangent.y;
        tangentData[i * 4 + 2] = tangent.z;
        tangentData[i * 4 + 3] = tangent.w;
    }
    
    return tangentData;
}
