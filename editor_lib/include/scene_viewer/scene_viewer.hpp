#pragma once
#include <code_utils/bee_utils.hpp>
#include <entt/entity/entity.hpp>

#include <string>
#include <unordered_map>
#include <unordered_set>

#include <imguizmo/ImGuizmo.h>
#include <menu_interface.hpp>

namespace bee {
class SceneViewer : public IMenu {
public:

	SceneViewer();
	~SceneViewer();

	NON_COPYABLE(SceneViewer);
	NON_MOVABLE(SceneViewer);

	virtual void Show(BlossomGame& game) override {
		ShowEntityEditor();
		ShowSceneHierarchy();
		DrawTransformGizmo();
	}

	void ShowSceneHierarchy();
	void ShowEntityEditor();
	void DrawTransformGizmo();

private:

	//helpers
	void display_entity_selectable(entt::entity entity);
	void display_transform_hierarchy(entt::entity entity);

	//Returns true if the entity contains the component
	bool display_component_widget(entt::id_type component_id, const std::string& component_name);
	void enumerate_components_to_add(const std::unordered_set<entt::id_type>& components);

	std::unordered_set<entt::id_type> component_filter;
	std::unordered_map<entt::id_type, std::string> reflected_components;

	entt::entity m_selectedEntity = entt::null;
	ImGuizmo::OPERATION m_gizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
};
}