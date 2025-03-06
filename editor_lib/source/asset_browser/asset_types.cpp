#include <precompiled/editor_precompiled.hpp>
#include <asset_browser/asset_types.hpp>
#include <resources/resource_handle.hpp>

#include <core/engine.hpp>
#include <resources/resource_manager.hpp>

const std::unordered_map<std::string, std::type_index> bee::EXTENSIONS_TO_TYPE
{
	{ ".png", std::type_index(typeid(bee::Image*))},
	{ ".jpg", std::type_index(typeid(bee::Image*)) },
	{ ".hdr", std::type_index(typeid(bee::Image*))},
	{ ".gltf", std::type_index(typeid(bee::Model*)) },
	{ ".glb", std::type_index(typeid(bee::Model*)) },
};


const std::unordered_map<std::string, const char*> bee::EXTENSIONS_TO_ICON {
	{ ".png", ICON_FA_FILE_IMAGE_O },
	{ ".hdr", ICON_FA_FILE_IMAGE_O },
	{ ".jpg", ICON_FA_FILE_IMAGE_O },
	{ ".gltf", ICON_FA_BRIEFCASE },
	{ ".glb", ICON_FA_BRIEFCASE },
};

bool bee::AssetDropTarget(std::filesystem::path* target, std::type_index type)
{
	bool changed = false;
	if (ImGui::BeginDragDropTarget()) {

		std::string payload_key = "FILE" + std::to_string(type.hash_code());
		if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payload_key.c_str())) {

			auto* path_ptr = reinterpret_cast<std::filesystem::path*>(payload->Data);
			*target = *path_ptr;
			changed = true;
		}

		ImGui::EndDragDropTarget();
	}
	return changed;
}

template<>
void bee::DefaultLoad<bee::Model>(ResourceHandle<Model>& handle, const std::filesystem::path& path) {
	handle = Engine.Resources().Models().FromGLTF(FileIO::Directory::Asset, path.string());
}