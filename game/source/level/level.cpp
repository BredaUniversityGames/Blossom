#include <precompiled/game_precompiled.hpp>
#include <level/level.hpp>

#include <core/ecs.hpp>
#include <core/engine.hpp>
#include <resources/resource_manager.hpp>
#include <rendering/render_components.hpp>
#include <core/transform.hpp>
#include <terrain/terrain_chunk.hpp>
#include <grass/grass_manager.hpp>
#include <rendering/render.hpp>
#include <rendering/model_renderer.hpp>
#include <grass/grass_chunk.hpp>

#include <core/fileio.hpp>
#include <math/geometry.hpp>

#include "terrain/terrain_collider.hpp"

#include "systems/collectable.hpp"

#include <cereal/cereal.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>
#include <grass/grass_renderer.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/string.hpp>
#include <tools/serialization_helpers.hpp>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <resources/image/image_loader.hpp>
#include <tools/log.hpp>
#include <resources/material/material_builder.hpp>
#include <tools/serialization.hpp>


#include "jolt/Jolt.h"
#include "physics/physics_system.hpp"
#include "physics/helpers.hpp"
#include <jolt/Physics/Collision/Shape/BoxShape.h>
#include <jolt/Physics/Body/BodyCreationSettings.h>

#include <entt/entity/snapshot.hpp>
#include <systems/point_of_interest.hpp>
#include <systems/model_root_component.hpp>
#include <systems/player_start.hpp>
#include <systems/orbiting_bee_system.hpp>

/// <summary>
/// Level Serialization
/// </summary>
namespace bee {

//LIGHTING
template<typename A>
void save(A& archive, const Level::LightingDescription& desc) {
    archive(cereal::make_nvp("SkyboxTexture", desc.skyboxPath));
    archive(cereal::make_nvp("SunColour", desc.sunColour));
    archive(cereal::make_nvp("SunDirection", desc.sunDirection));
    archive(cereal::make_nvp("SunIntensity", desc.sunIntensity));
    archive(cereal::make_nvp("AmbientFactor", desc.ambientFactor));
    archive(cereal::make_nvp("SSSStrength", desc.SSSstrength));
    archive(cereal::make_nvp("SSSDistorion", desc.SSSdistortion));
    archive(cereal::make_nvp("SSSPower", desc.SSSpower));
}

template<typename A>
void load(A& archive, Level::LightingDescription& desc) {

    archive(cereal::make_nvp("SkyboxTexture", desc.skyboxPath));
    archive(cereal::make_nvp("SunColour", desc.sunColour));
    archive(cereal::make_nvp("SunDirection", desc.sunDirection));
    archive(cereal::make_nvp("SunIntensity", desc.sunIntensity));
    archive(cereal::make_nvp("AmbientFactor", desc.ambientFactor));
    archive(cereal::make_nvp("SSSStrength", desc.SSSstrength));
    archive(cereal::make_nvp("SSSDistorion", desc.SSSdistortion));
    archive(cereal::make_nvp("SSSPower", desc.SSSpower));
}

//TERRAIN
template<typename A>
void load(A& archive, Level::TerrainDescription& desc) {

    std::string heightMapPath;
    archive(cereal::make_nvp("HeightMap", heightMapPath));

    if (!heightMapPath.empty()) try {
        desc.heightMap = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, heightMapPath, ImageFormat::RGBA8);
    }
    catch (std::exception& e) {
        Log::Error("Failed loading heightmap path from level file {}", e.what());
    }
    else {
        Log::Warn("Did not find a heightmap texture in level file");
    }

    archive(cereal::make_nvp("HeightScale", desc.heightScale));
    archive(cereal::make_nvp("MapSizeX", desc.mapSizeX));
    archive(cereal::make_nvp("MapSizeY", desc.mapSizeY));

    std::string albedoPath, roughnessPath, normalPath;

    archive(cereal::make_nvp("TerrainAlbedo", albedoPath));
    archive(cereal::make_nvp("TerrainNormals", normalPath));
    archive(cereal::make_nvp("TerrainRoughness", roughnessPath));

    auto& imageLoader = Engine.Resources().Images();

    auto sampler = bee::Sampler();
    sampler.MagFilter = Sampler::Filter::Linear;
    sampler.MinFilter = Sampler::Filter::LinearMipmapLinear;
    sampler.WrapS = Sampler::Wrap::Repeat;
    sampler.WrapT = Sampler::Wrap::Repeat;

    auto matBuilder = MaterialBuilder();

    if (!albedoPath.empty())
        matBuilder.WithTexture(TextureSlotIndex::BASE_COLOR, imageLoader.FromFile(FileIO::Directory::Asset, albedoPath, ImageFormat::RGBA8));

    if (!roughnessPath.empty())
        matBuilder.WithTexture(TextureSlotIndex::METALLIC_ROUGHNESS, imageLoader.FromFile(FileIO::Directory::Asset, roughnessPath, ImageFormat::RGBA8));

    if (!normalPath.empty())
        matBuilder.WithTexture(TextureSlotIndex::NORMAL_MAP, imageLoader.FromFile(FileIO::Directory::Asset, normalPath, ImageFormat::RGBA8));

    // Fallback factor: metallic - 0, roughness - 1
    glm::vec4 roughnessFactor = { 0, 1, 0, 0 };

    desc.terrainMaterial = matBuilder
        .WithSampler(TextureSlotIndex::BASE_COLOR, sampler)
        .WithSampler(TextureSlotIndex::NORMAL_MAP, sampler)
        .WithSampler(TextureSlotIndex::METALLIC_ROUGHNESS, sampler)
        .WithFactor(TextureSlotIndex::METALLIC_ROUGHNESS, roughnessFactor)
        .Build();

    archive(cereal::make_nvp("TesselationDistance", desc.tesselationDistance));
    archive(cereal::make_nvp("TesselatinModifier", desc.tesselationModifier));
    archive(cereal::make_nvp("UnitsPerTile", desc.unitsPerTile));
}

template<typename A>
void save(A& archive, const Level::TerrainDescription& desc) {

    archive(cereal::make_nvp("HeightMap", desc.heightMap.GetPath()));
    archive(cereal::make_nvp("HeightScale", desc.heightScale));
    archive(cereal::make_nvp("MapSizeX", desc.mapSizeX));
    archive(cereal::make_nvp("MapSizeY", desc.mapSizeY));

    std::string albedoPath, roughnessPath, normalPath;

    if (auto terrainMaterial = desc.terrainMaterial.Retrieve())
    {
        albedoPath = terrainMaterial->BaseColorTexture.GetPath();
        roughnessPath = terrainMaterial->MetallicRoughnessTexture.GetPath();
        normalPath = terrainMaterial->NormalTexture.GetPath();
    }

    archive(cereal::make_nvp("TerrainAlbedo", albedoPath));
    archive(cereal::make_nvp("TerrainNormals", normalPath));
    archive(cereal::make_nvp("TerrainRoughness", roughnessPath));

    archive(cereal::make_nvp("TesselationDistance", desc.tesselationDistance));
    archive(cereal::make_nvp("TesselatinModifier", desc.tesselationModifier));
    archive(cereal::make_nvp("UnitsPerTile", desc.unitsPerTile));
}

//GRASS
template<typename A>
void save(A& archive, const Level::GrassDescription& desc) {
    archive(cereal::make_nvp("GrassColourMap", desc.grassColour.GetPath()));
    archive(cereal::make_nvp("GrassDensityMap", desc.grassDensityMap.GetPath()));
    archive(cereal::make_nvp("Material", desc.material));
}

template<typename A>
void load(A& archive, Level::GrassDescription& desc) {
    auto& imageLoader = Engine.Resources().Images();

    std::string grassColourPath, grassDensityPath;
    archive(cereal::make_nvp("GrassColourMap", grassColourPath));
    archive(cereal::make_nvp("GrassDensityMap", grassDensityPath));
    archive(cereal::make_nvp("Material", desc.material));

    desc.grassColour = imageLoader.FromFile(FileIO::Directory::Asset, grassColourPath, ImageFormat::RGBA8);
    desc.grassDensityMap = imageLoader.FromFile(FileIO::Directory::Asset, grassDensityPath, ImageFormat::RGBA8);
}

template<typename A>
void save(A& archive, const Level::LODDescription& desc)
{
    archive(cereal::make_nvp("Distances", desc.distances));
}
template<typename A>
void load(A& archive, Level::LODDescription& desc)
{
    archive(cereal::make_nvp("Distances", desc.distances));
}


//PROP
template<typename A>
void save(A& archive, const Level::PropDescription& desc) {

    std::vector<std::string> modelPaths;
    modelPaths.reserve(desc.propModels.size());

    for (auto& model : desc.propModels)
    {
        modelPaths.push_back(model.GetPath());
    }

    archive(cereal::make_nvp("PropModels", modelPaths));

    archive(cereal::make_nvp("UseWind", desc.generateWindMask));
    archive(cereal::make_nvp("GenerateCollidableMesh", desc.generateCollidableMesh));
    archive(cereal::make_nvp("MaxBendFactor", desc.distanceMaxBend));
    archive(cereal::make_nvp("BendHeightPercentage", desc.displacementHeightPercent));

    archive(cereal::make_nvp("Collectable", desc.collectable));
    archive(cereal::make_nvp("HiddenUntilSequence", desc.partOfSequence));

    archive(cereal::make_nvp("PropTransforms", desc.propTransforms));
}

template<typename A>
void load(A& archive, Level::PropDescription& desc) {

    std::vector<std::string> modelPaths;
    archive(cereal::make_nvp("PropModels", modelPaths));

    try {

        desc.propModels.clear();
        for (int i = 0; i < modelPaths.size(); i += 1)
        {
            ResourceHandle<Model> model = Engine.Resources().Models().FromGLTF(FileIO::Directory::Asset, modelPaths[i]);
            desc.propModels.push_back(model);
        }
    }
    catch (std::exception& e) {
        Log::Error("Failed loading heightmap path from level file {}", e.what());
    }

    archive(cereal::make_nvp("UseWind", desc.generateWindMask));
    archive(cereal::make_nvp("GenerateCollidableMesh", desc.generateCollidableMesh));
    archive(cereal::make_nvp("MaxBendFactor", desc.distanceMaxBend));
    archive(cereal::make_nvp("BendHeightPercentage", desc.displacementHeightPercent));

    archive(cereal::make_nvp("Collectable", desc.collectable));
    archive(cereal::make_nvp("HiddenUntilSequence", desc.partOfSequence));

    archive(cereal::make_nvp("PropTransforms", desc.propTransforms));
}

// Cereal Semantics
// 
// template<class Archive>
// void save(Archive& archive,
//    MyClass const& m)
// {
//    archive(m.x, m.y, m.z);
// }
//
// template<class Archive>
// void load(Archive& archive,
//    MyClass& m)
// {
//    archive(m.x, m.y, m.z);
// }

}

bee::Level::Level()
{
    FallbackDefaultLevel();
}

bee::Level::Level(JSONLoader& archive)
{
    try {

        archive("Environment", Section{
            [&]() {
                archive("GrassInfo", m_grass);
                archive("TerrainInfo", m_terrain);
                archive("LightingInfo", m_lighting);
                archive("LODsInfo", m_lods);
                archive("AllProps", m_props);
            }
        });

        Engine.ECS().Registry.clear();
        entt::snapshot_loader POIArchive{ Engine.ECS().Registry };

        archive("Entities", Section{
            [&]() {

            POIArchive
                .get<entt::entity>(archive)
                .get<Transform>(archive)
                .get<Transform>(archive)
                .get<ModelRootComponent>(archive)
                .get<POIComponent>(archive)
                .get<PlayerStart>(archive)
                .get<OrbitalSpawnerComponent>(archive)
                .orphans();
            }
        });
    }
    catch (std::exception& e) {
        Log::Warn("Failure to read level file: {}", e.what());
        FallbackDefaultLevel();
    }

}


void bee::Level::SaveToArchive(JSONSaver& archive)
{
    entt::snapshot POIArchive{ Engine.ECS().Registry };

    auto POIView = Engine.ECS().Registry.view<ModelRootComponent, Transform, POIComponent, OrbitalSpawnerComponent>();
    auto PlayerStartView = Engine.ECS().Registry.view<PlayerStart, Transform>();
    const auto& subsurface = Engine.Renderer().GetModelRenderer().GetSubsurfaceData();

    m_lighting.SSSstrength = subsurface.strength;
    m_lighting.SSSdistortion = subsurface.distortion;
    m_lighting.SSSpower = subsurface.power;

    archive("Environment", Section{
        [&]() 
        {
            archive("GrassInfo", m_grass);
            archive("TerrainInfo", m_terrain);
            archive("LightingInfo", m_lighting);
            archive("LODsInfo", m_lods);
            archive("AllProps", m_props);
        }
    });

    archive("Entities", Section{
        [&]()
        {
            POIArchive
                .get<entt::entity>(archive)
                .get<Transform>(archive, POIView.begin(), POIView.end())
                .get<Transform>(archive, PlayerStartView.begin(), PlayerStartView.end())
                .get<ModelRootComponent>(archive, POIView.begin(), POIView.end())
                .get<POIComponent>(archive, POIView.begin(), POIView.end())
                .get<PlayerStart>(archive, PlayerStartView.begin(), PlayerStartView.end())
                .get<OrbitalSpawnerComponent>(archive, POIView.begin(), POIView.end())
                ;
        }
    });
}

bee::Level::~Level() {

    auto& registry = bee::Engine.ECS().Registry;

    for (auto&& [entity, terrain] : registry.view<TerrainChunk>().each()) {
        Engine.ECS().DeleteEntity(entity);
    }

    for (auto&& [entity, tag] : registry.view<GrassChunk>().each()) {
        bee::Engine.ECS().DeleteEntity(entity);
    }

    for (auto&& [entity, tag] : registry.view<ModelTag>().each()) {
        bee::Engine.ECS().DeleteEntity(entity);
    }
}

void bee::Level::GenerateLighting() {

    try {
        auto skyboxImage = ImageLoader().FromFile(FileIO::Directory::Asset, m_lighting.skyboxPath.c_str(), ImageFormat::RGBA32F);
        Engine.Renderer().SetSkybox(skyboxImage);
        Engine.Renderer().SetAmbientFactor(m_lighting.ambientFactor);
        auto& subsurface = Engine.Renderer().GetModelRenderer().GetSubsurfaceData();
        subsurface.strength = m_lighting.SSSstrength;
        subsurface.distortion = m_lighting.SSSdistortion;
        subsurface.power = m_lighting.SSSpower;
    }
    catch (const std::exception& e) {
        Log::Warn("Could not load skybox for level, {}", e.what());
    }
}

void bee::Level::GenerateTerrain()
{
    //Clear all terrain from ecs
    for (auto&& [entity, terrain] : bee::Engine.ECS().Registry.view<TerrainChunk>().each()) {
        Engine.ECS().DeleteEntity(entity);
    }

    std::vector<float> vertices; vertices.reserve((m_terrain.mapSizeX + 1) * (m_terrain.mapSizeY + 1) * 3);
    std::vector<float> uv; uv.reserve((m_terrain.mapSizeX + 1) * (m_terrain.mapSizeY + 1) * 2);

    float half_width = static_cast<float>(m_terrain.mapSizeX) / 2.0f;
    float half_height = static_cast<float>(m_terrain.mapSizeY) / 2.0f;

    for (int y = 0; y <= m_terrain.mapSizeY; ++y) {
        for (int x = 0; x <= m_terrain.mapSizeX; ++x) {

            vertices.push_back((static_cast<float>(x) - half_width) * m_terrain.unitsPerTile);
            vertices.push_back((static_cast<float>(y) - half_height) * m_terrain.unitsPerTile);
            vertices.push_back(0.f);

            uv.push_back(static_cast<float>(x) / static_cast<float>(m_terrain.mapSizeX));
            uv.push_back(static_cast<float>(y) / static_cast<float>(m_terrain.mapSizeY));
        }
    }

    //Indices
    std::vector<unsigned int> indices;
    indices.reserve(m_terrain.mapSizeX * m_terrain.mapSizeY * 6);

    for (int i = 0, y = 0; y < m_terrain.mapSizeY; ++i, ++y) {
        for (int x = 0; x < m_terrain.mapSizeX; ++i, ++x) {

            indices.push_back(i);
            indices.push_back(i + 1);
            indices.push_back(i + m_terrain.mapSizeX + 1);
            indices.push_back(i + 1);
            indices.push_back(i + m_terrain.mapSizeX + 2);
            indices.push_back(i + m_terrain.mapSizeX + 1);
        }
    }

   m_terrain.generatedMesh = 
       bee::Engine.Resources().Meshes().FromRawData("terrain", std::move(indices), std::move(vertices), {}, std::move(uv), {}, {}, BoundingBox());

    // Creating entity
    const auto terrainEntity = bee::Engine.ECS().CreateEntity();
    {
        // Transform
        auto& transform = bee::Engine.ECS().Registry.emplace<bee::Transform>(terrainEntity);
        transform.Name = "Terrain plane";

        // Mesh
        auto& renderer = bee::Engine.ECS().Registry.emplace<bee::MeshRenderer>(terrainEntity);
        renderer.LODs[0] = m_terrain.generatedMesh;

        // Hard override to disable dithering on terrain.
        m_terrain.terrainMaterial.Retrieve()->IsDitherable = false;
        renderer.Material = m_terrain.terrainMaterial;

        // Terrain settings
        auto& terrainChunk = bee::Engine.ECS().Registry.emplace<TerrainChunk>(terrainEntity);
        terrainChunk.height = m_terrain.mapSizeX;
        terrainChunk.width = m_terrain.mapSizeY;
        terrainChunk.heightmap = m_terrain.heightMap;
        terrainChunk.tesselationFactor = m_terrain.tesselationModifier;
        terrainChunk.tesselationDistance = m_terrain.tesselationDistance;
        terrainChunk.heightModifier = m_terrain.heightScale;

        m_terrainCollider.reset();
        m_terrainCollider = std::make_unique<TerrainCollider>(FileIO::Directory::Asset, m_terrain.heightMap.GetPath());
    }
}

void bee::Level::GenerateGrass()
{
    auto& grassRenderer = Engine.Renderer().GetGrassRenderer();
    auto& grassMaps = grassRenderer.GetInputMaps();
    grassMaps.m_colorMap = m_grass.grassColour;
    grassMaps.m_lengthMap = m_grass.grassDensityMap;

    {
        GrassChunkMaterial material{ grassRenderer.GetMaterial() };
        material = m_grass.material;

        grassRenderer.UpdateMaterial(std::move(material));
    }

    //Clear all terrain from ecs
    for (auto&& [entity, terrain] : bee::Engine.ECS().Registry.view<GrassChunk>().each()) {
        Engine.ECS().DeleteEntity(entity);
    }

    constexpr int GRASS_CHUNK_SIZE = 16; //Grass generation is hardcoded for 8x8 units per chunk

    const float grass_chunks_x =
        std::ceilf(static_cast<float>(m_terrain.mapSizeX) / static_cast<float>(GRASS_CHUNK_SIZE));

    const float grass_chunks_y =
        std::ceilf(static_cast<float>(m_terrain.mapSizeY) / static_cast<float>(GRASS_CHUNK_SIZE));

    for (int y = 0; y < static_cast<int>(grass_chunks_y); ++y)
    {
        for (int x = 0; x < static_cast<float>(grass_chunks_x); ++x)
        {
            glm::vec3 grassChunkPos = {
                (static_cast<float>(x) - grass_chunks_x / 2) * GRASS_CHUNK_SIZE + GRASS_CHUNK_SIZE / 2,
                (static_cast<float>(y) - grass_chunks_y / 2) * GRASS_CHUNK_SIZE + GRASS_CHUNK_SIZE / 2,
                0.0f
            };

            auto entity = Engine.GetGrassManager().CreateChunk(grassChunkPos, m_terrain.heightMap, m_terrain.heightScale);

        }
    }
}

void bee::Level::GenerateProp(size_t prop_index)
{
    auto& registry = bee::Engine.ECS().Registry;

    ClearProp(prop_index);

    entt::entity terrainEntity = entt::null;
    auto terrainView = registry.view<Transform, TerrainChunk>();
    if (terrainView.begin() != terrainView.end()) terrainEntity = *terrainView.begin();

    auto& propEntry = m_props.at(prop_index);

	if (propEntry.generateWindMask)
	{
        for (int i = 0; i < propEntry.propModels.size(); i += 1)
        {
		    propEntry.propModels[i].Retrieve()->GenerateDisplacementData(propEntry.displacementHeightPercent, propEntry.distanceMaxBend);
        }
	}

    JPH::PhysicsSystem& physicsSystem = Engine.PhysicsSystem();
    JPH::BodyInterface& bodyInterface = physicsSystem.GetBodyInterface();

    int32_t prevModelIndex = -1;
    glm::vec3 boxExtent;
    glm::vec3 boxCenter;
    JPH::BoxShapeSettings shapeSettings(JPH::Vec3(0.5f, 0.5f, 0.5f));
    shapeSettings.SetEmbedded();
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    JPH::Shape* shape = shapeResult.Get().GetPtr();

    for (int i = 0; i < propEntry.propTransforms.size(); i += 1) {

        auto& matrix = propEntry.propTransforms[i];

        glm::vec3 scale, translation;
        glm::quat rotation;

        glm::vec3 _unusedSkew; glm::vec4 _unusedPerspective;
        glm::decompose(matrix, scale, rotation, translation, _unusedSkew, _unusedPerspective);

        auto newEntity = registry.create();
        registry.emplace<ModelTag>(newEntity, prop_index);

        auto& transform = registry.emplace<Transform>(newEntity);

        transform.SetTranslation(translation);
        transform.SetRotation(rotation);
        transform.SetScale(scale);

        int32_t numModels = static_cast<int32_t>(propEntry.propModels.size());
        int32_t matriciesPerModel = static_cast<int32_t>((1.0 / numModels) * propEntry.propTransforms.size());
        int32_t modelIndex = glm::clamp(i / matriciesPerModel, 0, glm::max(numModels - 1, 0));

        if (modelIndex != prevModelIndex)
        {
            prevModelIndex = modelIndex;

            auto model = propEntry.propModels[modelIndex].Retrieve();
            boxExtent = model->maxBounds - model->minBounds;

            boxCenter = glm::vec3(model->minBounds.x, model->minBounds.y, model->minBounds.z) + boxExtent * 0.5f;
        }

        bool generateCollision = propEntry.collectable || propEntry.partOfSequence;

        if (generateCollision)
        {
            Transform* transform = registry.try_get<Transform>(newEntity);
            bee::Decompose(transform->World(), translation, scale, rotation);

            //Correction since jolt uses different coordinate system
            rotation = rotation * World::YUP_TO_ZUP;

            glm::vec3 offset = glm::rotate(rotation, boxCenter * scale);

            JPH::BodyCreationSettings boxSettings(shape->ScaleShape(GlmToJolt(boxExtent * scale)).Get(), GlmToJolt(translation + offset), GlmToJolt(rotation), JPH::EMotionType::Static, Layers::COLLECTABLE);
            JPH::Body* body = bodyInterface.CreateBody(boxSettings);

            if (body != nullptr)
            {
                bodyInterface.AddBody(body->GetID(), JPH::EActivation::DontActivate);
                body->SetUserData(static_cast<JPH::uint64>(newEntity));
                auto& collider = registry.emplace<bee::ColliderComponent>(newEntity);
                collider.body = body->GetID();
            }
            else
            {
                Log::Warn("physics body creation failed");
                Log::Info("num bodies: {:d}, max bodies: {:d}", physicsSystem.GetNumBodies(), physicsSystem.GetMaxBodies());
            }
        }

        if (propEntry.partOfSequence)
        {
            propEntry.propModels[modelIndex].Retrieve()->InstantiateScene(registry, newEntity, true);
            registry.emplace<BeautifyTag>(newEntity);
        }
        else
        {
            propEntry.propModels[modelIndex].Retrieve()->InstantiateScene(registry, newEntity);
        }
        
        if (propEntry.generateCollidableMesh)
        {
            InitColliderTransforms(Engine.ECS().Registry, newEntity);
        }

        if (propEntry.collectable)
        {
            registry.emplace<Collectable>(newEntity, false);
        }
    }

    Engine.PhysicsSystem().OptimizeBroadPhase();
}

void bee::Level::ClearProp(size_t prop_index)
{
    auto& registry = bee::Engine.ECS().Registry;

    //Clear all previous models
    for (auto&& [entity, tag] : registry.view<ModelTag>().each()) {
        if (tag.index == prop_index) bee::Engine.ECS().DeleteEntity(entity);
    }
}

void bee::Level::FallbackDefaultLevel()
{
    auto& terrain = GetTerrain();

    terrain.heightMap = bee::Engine.Resources().Images().FromFile(
        bee::FileIO::Directory::Asset, "textures/noise/gradient.png",
        ImageFormat::RGBA8);

    auto imageAlbedo = bee::Engine.Resources().Images().FromFile(
        bee::FileIO::Directory::Asset,
        "textures/terrain/Grass_Dense_Tint_01_Base_Basecolor_A.png",
        ImageFormat::RGBA8);

    auto imageNormal = bee::Engine.Resources().Images().FromFile(
        bee::FileIO::Directory::Asset,
        "textures/terrain/Grass_Dense_Tint_01_Base_Normal.png",
        ImageFormat::RGBA8);

    auto imageOcclusion = bee::Engine.Resources().Images().FromFile(
        bee::FileIO::Directory::Asset,
        "textures/terrain/Grass_Dense_Tint_01_Base_AO.png", ImageFormat::RGBA8);

    auto sampler = bee::Sampler();
    sampler.MinFilter = Sampler::Filter::LinearMipmapLinear;
    sampler.MagFilter = Sampler::Filter::Linear;
    sampler.WrapS = Sampler::Wrap::Repeat;
    sampler.WrapT = Sampler::Wrap::Repeat;

    terrain.terrainMaterial =
        MaterialBuilder()
        .WithTexture(TextureSlotIndex::BASE_COLOR, imageAlbedo)
        .WithTexture(TextureSlotIndex::NORMAL_MAP, imageNormal)
        .WithTexture(TextureSlotIndex::OCCLUSION, imageOcclusion)
        .WithSampler(TextureSlotIndex::BASE_COLOR, sampler)
        .WithSampler(TextureSlotIndex::NORMAL_MAP, sampler)
        .WithSampler(TextureSlotIndex::OCCLUSION, sampler)
        .WithDithering(false)
        .Build();

    terrain.mapSizeX = 128;
    terrain.mapSizeY = 128;
    terrain.unitsPerTile = 1;
    terrain.heightScale = 3.0f;

    GetGrass().grassColour = imageAlbedo;
    GetGrass().grassDensityMap = Engine.Resources().Images().FromFile(FileIO::Directory::Asset, "textures/noise/gradient.png", ImageFormat::RGBA8);

}

