#include <precompiled/editor_precompiled.hpp>
#include <scene_viewer/scene_viewer.hpp>
#include <imgui/imgui.h>

#include <core/engine.hpp>
#include <core/ecs.hpp>
#include <entt/entity/runtime_view.hpp>
#include <entt/meta/factory.hpp>

#include <tools/log.hpp>

#include <core/transform.hpp>
#include <grass/grass_chunk.hpp>
#include <resources/resource_handle.hpp>
#include <rendering/render_components.hpp>

#include <imgui/imgui_stdlib.h>
#include <ui/ui.hpp>
#include "displacement/displacement_manager.hpp"
#include "displacement/displacer.hpp"

#include <core/input.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "physics/rigidbody.hpp"
#include <systems/point_of_interest.hpp>
#include <asset_browser/asset_types.hpp>
#include <systems/model_root_component.hpp>
#include <systems/player.hpp>
#include <systems/player_start.hpp>
#include <systems/orbiting_bee_system.hpp>

namespace bee {

//A collection of identifiers for use in invoking reflected functions
//Their function signature is documented above the identifiers
namespace MetaFunctions {

//void remove(entity, registry*)
constexpr auto META_REMOVE = entt::hashed_string("__remove");

//void patch(entity, registry*)
constexpr auto META_PATCH = entt::hashed_string("__patch");

//void widget(entity, registry*)
constexpr auto META_WIDGET = entt::hashed_string("__widget");

//void save(cereal::outputJSONarchive&)
//constexpr auto META_SAVE = entt::hashed_string("__save");

//void load(cereal::inputJSONarchive&)
//constexpr auto META_LOAD = entt::hashed_string("__load");
}

struct Inspector {

	template<typename T>
	void operator()(const char* name, T& o) {
		if (ImGui::TreeNode(name))
		{
			visit_struct::for_each(o, *this);
			ImGui::TreePop();
		}
	}

	template<typename T>
	void operator()(const char* name, ResourceHandle<T>& h) {
		ImGui::LabelText(name, "Resource: %s ", h.GetPath().c_str());

		std::filesystem::path target;
		if (AssetDropTarget<T>(&target)) {
			DefaultLoad<T>(h, target);
			patch = true;
		}
	}

	template<typename T>
	void operator()(const char* name, std::vector<T>& v)
	{
		for(size_t i = 0; i < v.size(); ++i)
		{
			std::string indexedName = std::to_string(i);
			indexedName += ": ";
			indexedName += name;
		    Inspector::operator()(indexedName.c_str(), v[i]);
		}
	}

	void operator()(const char* name, float& f) {
		if (ImGui::DragFloat(name, &f, 0.01f)) patch = true;
	}

	void operator()(const char* name, int& i) {
		if (ImGui::DragInt(name, &i)) patch = true;
	}

	void operator()(const char* name, uint32_t& i) {
		int _i = static_cast<int>(i);;

		if (ImGui::DragInt(name, &_i, 0, INT32_MAX))
		{
			patch = true;
			i = static_cast<uint32_t>(_i);
		}
	}

	void operator()(const char* name, size_t& i) {
		int _i = static_cast<int>(i);

		if (ImGui::DragInt(name, &_i, 0, INT32_MAX))
		{
			patch = true;
			i = static_cast<size_t>(_i);
		}
	}

	void operator()(const char* name, glm::vec2& v) {
		if (ImGui::DragFloat2(name, glm::value_ptr(v)))
			patch = true;
	}

	void operator()(const char* name, glm::vec3& v) {
		std::string n{ name }; // Small string optimization, makes this fine to do.
		if (n.find("color") != std::string::npos || n.find("Color") != std::string::npos)
		{
			if (ImGui::ColorEdit3(name, glm::value_ptr(v))) patch = true;
		}
		else
		{
			if (ImGui::DragFloat3(name, glm::value_ptr(v))) patch = true;
		}
	}

	void operator()(const char* name, glm::vec4& v) {
		std::string n{ name }; // Small string optimization, makes this fine to do.
		if (n.find("color") != std::string::npos || n.find("Color") != std::string::npos)
		{
			if (ImGui::ColorEdit4(name, glm::value_ptr(v))) patch = true;
		}
		else
		{
			if (ImGui::DragFloat4(name, glm::value_ptr(v))) patch = true;
		}
	}

	void operator()(const char* name, glm::quat& q) {
		auto v = glm::eulerAngles(q) * (180.0f / glm::pi<float>());

		if (ImGui::DragFloat3(name, glm::value_ptr(v)))
		{
			v = glm::clamp(v, { -180.0f, -90.0f, -180.0f }, { 180.0f, 90.0f, 180.0f });
			q = glm::quat(v * (glm::pi<float>() / 180.0f));
			patch = true;
		}
	}

	void operator()(const char* name, bool& b) {
		if (ImGui::Checkbox(name, &b)) patch = true;
	}

	void operator()(const char* name, std::string& s) {
		if (ImGui::InputText(name, &s)) patch = true;
	}

	bool patch = false;
};

template<typename T>
T& constructor(const entt::entity entity, entt::registry* registry) {
	return registry->emplace<T>(entity);
}

template<typename T>
void destructor(entt::entity e, entt::registry* registry) {
	registry->remove<T>(e);
}

template<typename T>
void patch(entt::entity e, entt::registry* registry) {
	registry->patch<T>(e);
}

template <typename T>
void widget(entt::entity e, entt::registry* registry) {
	auto& component = registry->get<T>(e);
	Inspector i{};
	visit_struct::for_each(component, i);
	if (i.patch) patch<T>(e, registry);
}

template<typename T>
entt::id_type reflect_component() {
	constexpr auto id = entt::type_hash<T>().value();

	entt::meta<T>().type(id)
		.ctor<&constructor<T>, entt::as_ref_t>()
		.func<&widget<T>>(MetaFunctions::META_WIDGET)
		.func<&destructor<T>>(MetaFunctions::META_REMOVE)
		.func<&patch<T>>(MetaFunctions::META_PATCH);

	return id;
}

}

#define REFLECT(T) reflected_components.emplace(reflect_component<T>(), #T)

bee::SceneViewer::SceneViewer()
{
	REFLECT(Transform);
	REFLECT(GrassChunk);
	REFLECT(MeshRenderer);
	REFLECT(Light);
	REFLECT(RigidBody);
	REFLECT(Displacer);
	REFLECT(DisplacerFocus);
	REFLECT(POIComponent);
	REFLECT(UIElement);
	REFLECT(ModelRootComponent);
	REFLECT(Player);
	REFLECT(PlayerStart);
	REFLECT(OrbitalSpawnerComponent);
}

bee::SceneViewer::~SceneViewer()
{
}

void bee::SceneViewer::ShowSceneHierarchy()
{
	auto& registry = Engine.ECS().Registry;

	ImGui::Begin("Scene View");
	ImGui::Text("Component Filter:");

	ImGui::SameLine();

	if (ImGui::SmallButton("Clear")) {
		component_filter.clear();
	}

	ImGui::Indent();

	for (const auto& [component_id, component_info] : reflected_components) {

		bool in_set = component_filter.count(component_id);
		bool active = in_set;

		ImGui::Checkbox(component_info.c_str(), &active);

		//If in list but checkbox returned false, Remove
		if (in_set && !active) {
			component_filter.erase(component_id);
		}

		//or Add otherwise
		else if (!in_set && active) {
			component_filter.emplace(component_id);
		}
	}

	ImGui::Unindent();
	ImGui::Separator();

	//Orphaned View
	if (component_filter.empty()) {
		ImGui::Text("Please choose a filter to display available entities");
	}
	else {

		entt::runtime_view view{};

		for (const auto type : component_filter) {

			auto* storage_ptr = registry.storage(type);
			if (storage_ptr != nullptr) {
				view.iterate(*storage_ptr);
			}
		}

		ImGui::Text("%lu Entities Matching:", view.size_hint());

		if (ImGui::BeginChild("Matching entity list")) {

			//We should limit the number of entries to avoid slowdown using the editor
			constexpr size_t MAX_IN_VIEW = 100;

			size_t count = 0;
			for (auto entity : view) { 
				
				if (count > MAX_IN_VIEW)
				{
					ImGui::Text("And %lu more entities...", view.size_hint() - MAX_IN_VIEW);
					break;
				}

				//If we are looking for transforms
				if (component_filter.count(entt::type_hash<Transform>().value()))
				{
					auto& transform = registry.get<Transform>(entity);
					if (!transform.HasParent()) display_transform_hierarchy(entity);
				}
				else 
				{
					//Entities with out transform
					display_entity_selectable(entity);
				}

				count++;
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

void bee::SceneViewer::ShowEntityEditor()
{
	auto& registry = Engine.ECS().Registry;

	ImGui::Begin("Entity Editor");
	ImGui::SameLine();
	int entity_id = static_cast<int>(entt::to_integral(m_selectedEntity));

	if (registry.valid(m_selectedEntity)) {

		std::string text("Editing: ID ");
		auto* transform = registry.try_get<Transform>(m_selectedEntity);

		if (transform && transform->Name.size()) {
			text = transform->Name;
		}
		else {
			text += std::to_string(entity_id);
		}
		ImGui::Text(text.c_str());
	}
	else {
		ImGui::Text("No entity selected...");
	}

	if (ImGui::Button("New")) {
		m_selectedEntity = registry.create();
	}

	if (registry.valid(m_selectedEntity)) {

		ImGui::SameLine();

		ImGui::Dummy({ 10, 0 }); // space destroy a bit, to not accidentally click it
		ImGui::SameLine();

		// red button
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.15f, 0.15f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.8f, 0.3f, 0.3f, 1.f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1.f, 0.2f, 0.2f, 1.f));

		if (ImGui::Button("Destroy")) {

			Engine.ECS().DeleteEntity(m_selectedEntity);
			m_selectedEntity = entt::null;
		}

		ImGui::PopStyleColor(3);
	}

	ImGui::Separator();

	if (registry.valid(m_selectedEntity)) {
		ImGui::PushID(entity_id);

		std::unordered_set<entt::id_type> has_not;

		for (auto& [component_id, component_name] : reflected_components) {
			bool contains = display_component_widget(component_id, component_name);
			if (!contains) has_not.insert(component_id);
		}

		if (!has_not.empty()) { 
			
			if(ImGui::Button("+ Add Component")) {
				ImGui::OpenPopup("Add Component");
			}

			if (ImGui::BeginPopup("Add Component")) {
				ImGui::Text("Available:");
				ImGui::Separator();

				enumerate_components_to_add(has_not);
				ImGui::EndPopup();
			}
		}

		ImGui::PopID();
	}
	ImGui::End();
}

void bee::SceneViewer::DrawTransformGizmo()
{
	auto& input = Engine.Input();

	if (input.GetKeyboardKeyOnce(Input::KeyboardKey::Y)) {
		m_gizmoOperation = ImGuizmo::TRANSLATE;
	}
	else if (input.GetKeyboardKeyOnce(Input::KeyboardKey::U)) {
		m_gizmoOperation = ImGuizmo::ROTATE;
	}
	else if (input.GetKeyboardKeyOnce(Input::KeyboardKey::I)) {
		m_gizmoOperation = ImGuizmo::SCALE;
	}

	auto* transform = Engine.ECS().Registry.try_get<Transform>(m_selectedEntity);
	if (transform == nullptr) return;

	auto vpPos = input.GetGameAreaPosition();
	auto vpSize = input.GetGameAreaSize();

	ImGuizmo::Enable(true);
	ImGuizmo::SetRect(vpPos.x, vpPos.y, vpSize.x, vpSize.y);

	//Pick the first camera (TODO: add option to set a camera or a camera entity)
	auto cameraView = Engine.ECS().Registry.view<Transform, CameraComponent>();

	//TODO set default value if no camera exists
	Camera frameCamera{};

	if (cameraView.begin() != cameraView.end())
	{
		auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front()).World();
		auto& cameraComponent = Engine.ECS().Registry.get<CameraComponent>(cameraView.front());

		if (!cameraComponent.isOrthographic) {
			frameCamera = Camera::Perspective(
				cameraTransform[3],
				cameraTransform[3] + cameraTransform * glm::vec4(World::FORWARD, 0.0f),
				cameraComponent.aspectRatio,
				cameraComponent.fieldOfView,
				cameraComponent.nearClip,
				cameraComponent.farClip
			);
		}
	}
	else
	{
		Log::Warn("RENDERER: No camera exists in the scene to render from.");
		return;
	}

	auto view = frameCamera.GetView();
	auto proj = frameCamera.GetProjection();

	auto modelMatrix = transform->World();

	bool manipulated = ImGuizmo::Manipulate(
		glm::value_ptr(view), glm::value_ptr(proj),
		m_gizmoOperation, ImGuizmo::MODE::LOCAL,
		glm::value_ptr(modelMatrix)
	);

	if (manipulated) {
		glm::vec3 translation{}, euler_rotation{}, scale{};

		//Correction for parented transforms
		auto* parentTransform = Engine.ECS().Registry.try_get<Transform>(transform->Parent());
		glm::mat4 local_space;
		
		if (parentTransform)
		{
			glm::mat4 parent_model_matrix = parentTransform->World();
			local_space = glm::inverse(parent_model_matrix) * modelMatrix;
		}
		else {
			local_space = modelMatrix;
		}

		ImGuizmo::DecomposeMatrixToComponents(
			glm::value_ptr(local_space),
			glm::value_ptr(translation),
			glm::value_ptr(euler_rotation),
			glm::value_ptr(scale)
		);

		glm::quat rotation = glm::quat(euler_rotation * glm::pi<float>() / 180.0f);

		transform->SetTranslation(translation);
		transform->SetRotation(rotation);
		transform->SetScale(scale);
	}
}

void bee::SceneViewer::display_entity_selectable(entt::entity entity)
{
	ImGui::PushID(entt::to_integral(entity));
	auto selected = m_selectedEntity;
	auto& registry = Engine.ECS().Registry;

	bool is_selected_entity = (m_selectedEntity == entity) ? true : false;
	bool is_selected = is_selected_entity;

	std::string text("Entity Id: ");
	auto ptr = registry.try_get<Transform>(entity);
	if (ptr && ptr->Name.size()) {
		text = ptr->Name;
	}
	else {
		int entity_id = static_cast<int>(entt::to_integral(entity));
		text += std::to_string(entity_id);
	}

	ImGui::Selectable(text.c_str(), &is_selected);

	if (is_selected && !is_selected_entity) { m_selectedEntity = entity; }
	else if (!is_selected && is_selected_entity) { m_selectedEntity = entt::null; }

	ImGui::PopID();
}

void bee::SceneViewer::display_transform_hierarchy(entt::entity entity)
{
	auto& registry = Engine.ECS().Registry;
	ImGui::PushID(entt::to_integral(entity));

	auto& transform = registry.get<Transform>(entity);
	auto first_child = *transform.begin();
	if (registry.valid(first_child)) {

		bool inside_tree = ImGui::TreeNode("##Transform Hierarchy");
		ImGui::SameLine(); display_entity_selectable(entity);

		if (inside_tree) {
			for (auto children : transform) {
				display_transform_hierarchy(children);
			}

			ImGui::TreePop();
		}
	}
	else {
		display_entity_selectable(entity);
	}

	ImGui::PopID();
}

bool bee::SceneViewer::display_component_widget(entt::id_type component_id, const std::string& component_name)
{
	auto& registry = Engine.ECS().Registry;
	const auto* storage_ptr = registry.storage(component_id);

	if (storage_ptr && storage_ptr->contains(m_selectedEntity)) {
		ImGui::PushID(component_id);

		auto meta_type = entt::resolve(component_id);

		if (ImGui::Button(" - ")) {

			meta_type.invoke(MetaFunctions::META_REMOVE, entt::meta_any(), m_selectedEntity, &registry);
			ImGui::PopID();
			return true;
		}

		else {
			ImGui::SameLine();
		}

		if (ImGui::CollapsingHeader(component_name.c_str())) {

			ImGui::Indent();
			ImGui::PushID("Widget");
			entt::resolve(component_id).invoke(MetaFunctions::META_WIDGET, entt::meta_any(), m_selectedEntity, &registry);
			ImGui::PopID();
			ImGui::Unindent();
		}

		ImGui::PopID();
		return true;
	}

	return false;
}

void bee::SceneViewer::enumerate_components_to_add(const std::unordered_set<entt::id_type>& components)
{
	auto& registry = Engine.ECS().Registry;
	for (auto& id : components) {

		auto it = reflected_components.find(id);
		if (it == reflected_components.end()) continue;

		ImGui::PushID(id);

		if (ImGui::Selectable(it->second.c_str())) {
			auto _unused = entt::resolve(it->first).construct(m_selectedEntity, &registry);
		}

		ImGui::PopID();
	}
}
