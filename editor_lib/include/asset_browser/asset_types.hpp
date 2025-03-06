#pragma once
#include <string>
#include <typeindex>
#include <unordered_map>
#include <filesystem>
#include <tools/log.hpp>
#include <resources/resource_handle.hpp>

namespace bee {

extern const std::unordered_map<std::string, std::type_index> EXTENSIONS_TO_TYPE;
extern const std::unordered_map<std::string, const char*> EXTENSIONS_TO_ICON;
extern const std::unordered_map<std::type_index, const char*> TYPE_TO_ICON;

//Returns true if the asset was changed (WARNING: type index must relate to Asset* and not Asset type)
//Prefer templated version for less error prone syntax
bool AssetDropTarget(std::filesystem::path* target, std::type_index type);

//Returns true if the path was changed
template <typename T> bool AssetDropTarget(std::filesystem::path* target) {
	return AssetDropTarget(target, std::type_index(typeid(T*)));
}

template<typename T>
void DefaultLoad(ResourceHandle<T>& handle, const std::filesystem::path& path) {
	Log::Warn("No default loading function exists for this type");
}

template<>
void DefaultLoad<Model>(ResourceHandle<Model>& handle, const std::filesystem::path& path);

}