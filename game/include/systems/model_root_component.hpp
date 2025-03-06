#pragma once
#include <core/ecs.hpp>
#include <visit_struct/visit_struct.hpp>

#include <resources/resource_handle.hpp>
#include <resources/resource_manager.hpp>
#include <resources/model/model_loader.hpp>
#include <core/engine.hpp>

namespace bee {

class ModelRootComponent {
public:

    static void SubscribeToEvents();
    static void UnsubscribeToEvents();

    ResourceHandle<Model> model;

private:
    static void OnPatch(entt::registry& registry, entt::entity entity);
    static void OnDestroy(entt::registry& registry, entt::entity entity);
};

template<typename A>
void save(A& archive, const ResourceHandle<Model>& model) {
    archive(model.GetPath());
}

template<typename A>
void load(A& archive, ResourceHandle<Model>& model) {

    std::string path;
    archive(path);
    if (!path.empty()) 
        model = Engine.Resources().Models().FromGLTF(FileIO::Directory::Asset, path);
}

}


VISITABLE_STRUCT(bee::ModelRootComponent, model);