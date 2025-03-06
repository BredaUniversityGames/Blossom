#pragma once

#include <code_utils/bee_utils.hpp>

#include <resources/image/image_loader.hpp>
#include <resources/mesh/mesh_loader.hpp>
#include <resources/material/material_builder.hpp>
#include <resources/model/model_loader.hpp>

#include <core/engine.hpp>

namespace bee
{

/// <summary>
/// Main point of access for all resource loaders
/// </summary>
class ResourceManager
{
public:
    ResourceManager() = default;
    ~ResourceManager() = default;

    NON_COPYABLE(ResourceManager);
    NON_MOVABLE(ResourceManager);

    ImageLoader& Images() { return image_loader; }
    MeshLoader& Meshes() { return mesh_loader; }
    //MaterialLoader& Materials() { return material_loader; }
    ModelLoader& Models() { return model_loader; }


private:

    MeshLoader mesh_loader;
    ImageLoader image_loader;
    //MaterialLoader material_loader;
    ModelLoader model_loader;

    friend class bee::EngineClass;
};

}  // namespace bee
