#include <precompiled/editor_precompiled.hpp>
#include <graphics_menu/graphics_menu.hpp>
#include <core/engine.hpp>
#include <level/level.hpp>
#include <game/blossom.hpp>

#include <resources/resource_manager.hpp>
#include <asset_browser/asset_types.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tools/log.hpp>
#include <rendering/render.hpp>
#include <rendering/post_process/post_process_manager.hpp>
#include "rendering/model_renderer.hpp"

void bee::GraphicsMenu::Show(BlossomGame& game)
{
	ImGui::Begin("Graphics Menu");

	if (auto level = game.GetLevel()) {

		DrawLightingMenu(level);
		DrawPostProcessingMenu(level);
		DrawGrassMenu(level);
		DrawLODsMenu(level);
		DrawSubsurfaceMenu(level);
	    DrawDebugFlags();

		ImGui::End();

		ImGui::Text("No Level loaded");
		ImGui::End();

		return;
	}
	else {

		ImGui::Text("No Level loaded");
		ImGui::End();

		return;
	}
}

void bee::GraphicsMenu::LoadFromFile(BlossomGame& game, cereal::JSONInputArchive* in_archive)
{
	auto level = game.GetLevel();

	skyboxDirty = false;
	skyboxEdit = level->GetLighting().skyboxPath;
}

void bee::GraphicsMenu::ApplyChanges(BlossomGame& game)
{
	//Only for skybox, all others are applied in realtime
	if (auto level = game.GetLevel()) {
		if (skyboxDirty) {
			level->GetLighting().skyboxPath = skyboxEdit.string();
			level->GenerateLighting();

			skyboxDirty = false;
		}
	}
}

void bee::GraphicsMenu::DrawLightingMenu(std::shared_ptr<Level> level)
{
	if (ImGui::TreeNode("Lighting")) {

		auto& lightingData = level->GetLighting();

		//In degrees
		glm::vec3 rotation = glm::eulerAngles(lightingData.sunDirection) * (180.0f / glm::pi<float>());
		if (ImGui::DragFloat3("Sun rotation", glm::value_ptr(rotation))) {

			rotation = glm::clamp(rotation, { -180.0f, -90.0f, -180.0f }, { 180.0f, 90.0f, 180.0f });
			lightingData.sunDirection = glm::quat(rotation * (glm::pi<float>() / 180.0f));
		}

		ImGui::ColorEdit3("Sun colour", glm::value_ptr(lightingData.sunColour));
		ImGui::DragFloat("Sun Intensity", &lightingData.sunIntensity, 0.0f);
		ImGui::SliderFloat("Ambient percentage", &lightingData.ambientFactor, 0.0f, 1.0f);

		std::string displayName = "No Skybox Assigned";
		if (!skyboxEdit.empty()) {
			displayName = "Skybox: " + skyboxEdit.string();
		}

		ImGui::Text(displayName.c_str());
		if (AssetDropTarget<Image>(&skyboxEdit)) {
			skyboxDirty = true;
		}

		ImGui::TreePop();
	}
}

void bee::GraphicsMenu::DrawPostProcessingMenu(std::shared_ptr<Level> level)
{
	if (ImGui::TreeNode("Post Processing")) {

		Engine.Renderer().GetPostProcessManager();

		ImGui::Text("Depth of Field");

		//TODO
		std::shared_ptr<DepthOfField> dof = Engine.Renderer().GetPostProcessManager().GetProcess<DepthOfField>(PostProcess::Type::DepthOfField);
		DepthOfField::DepthOfFieldData& data = dof->GetDOFData();

		ImGui::SliderFloat("Focus distance", &data.FocusDistance, 0.0f, 50.0f);
		ImGui::SliderFloat("Focus fallof distance", &data.FocusFalloffDistance, 0.0f, 30.0f);
		ImGui::SliderFloat("Blur strength", &data.BlurStrength, 0.0f, 1.0f);
		ImGui::SliderFloat("Dither distance", &Engine.Renderer().GetDitherDistance(), 0.0f, 10.0f);

		ImGui::TreePop();
	}
}

void bee::GraphicsMenu::DrawGrassMenu(std::shared_ptr<Level> level)
{
	if(ImGui::TreeNode("Grass"))
	{
		GrassChunkMaterial& material = level->GetGrass().material;

		bool changed = false;
		changed |= ImGui::DragFloatRange2("Blade length", &material.minHeight, &material.maxHeight, 0.01f, 0.1f, 10.0f);
		changed |= ImGui::DragFloat("Cutoff length", &material.cutoffLength, 0.01f, 0.01f, material.maxHeight);
		changed |= ImGui::DragFloat("Blade bending", &material.bladeBending, 0.01f, 0.0f, 1.0f);

		ImGui::Separator();

		changed |= ImGui::DragFloat("Dark edges strength", &material.darkEdgeStrength, 0.01f, 0.0f, 1.0f);
		changed |= ImGui::DragFloat("Dark edges range", &material.darkEdgeRange, 0.01f, 0.0f, 5.0f);
		changed |= ImGui::DragFloat("AO strength", &material.aoStrength, 0.01f, 0.0f, 1.0f);
		changed |= ImGui::DragFloat("AO range", &material.aoRange, 0.01f, 0.01f, 1.0f);

		ImGui::Indent(12);
		for(size_t i = 0; i < material.lodAdjustments.size(); i++)
        {
            std::stringstream labelSS{};
            labelSS << "LOD ";
            labelSS << std::to_string(i);
            labelSS << " adjustment";

            changed |= ImGui::DragFloat(labelSS.str().c_str(), &material.lodAdjustments[i], 0.01f, 0.0f, 5.0f);
        }
		ImGui::Indent(-12);

		if (changed)
		{
			level->GenerateGrass();
		}

		ImGui::TreePop();
	}
}

void bee::GraphicsMenu::DrawLODsMenu(std::shared_ptr<Level> level)
{
	if (ImGui::TreeNode("LODs"))
    {
        auto& lods = level->GetLODs();

        for (size_t i = 1; i <= lods.distances.size(); i++)
        {
            auto& distance = lods.distances[i - 1];

			std::stringstream labelSS{};
			labelSS << "LOD: ";
			labelSS << std::to_string(i);

            ImGui::SliderFloat(labelSS.str().c_str(), &distance, 0.0f, 100.0f);
			if(i < lods.distances.size() && distance > lods.distances[i])
				distance = lods.distances[i];

        }

        ImGui::TreePop();
    }
}

void bee::GraphicsMenu::DrawDebugFlags()
{
	if (ImGui::TreeNode("Debug Rendering Toggle")) {
		auto& rendererDebugFlags = Engine.Renderer().GetDebugFlags();

		ImGui::Checkbox("Show Base Color", &rendererDebugFlags.BaseColor);
		ImGui::Checkbox("Show Vertex Normals", &rendererDebugFlags.Normals);
		ImGui::Checkbox("Show Normal Map", &rendererDebugFlags.NormalMap);
		ImGui::Checkbox("Show Metallic", &rendererDebugFlags.Metallic);
		ImGui::Checkbox("Show Roughness", &rendererDebugFlags.Roughness);
		ImGui::Checkbox("Show Emissive", &rendererDebugFlags.Emissive);
		ImGui::Checkbox("Show Occlusion", &rendererDebugFlags.Occlusion);
		ImGui::Checkbox("Show DisplacementPivot", &rendererDebugFlags.DisplacementPivot);
		ImGui::Checkbox("Show WindMask", &rendererDebugFlags.WindMask);

		ImGui::TreePop();
	}
}

void bee::GraphicsMenu::DrawSubsurfaceMenu(std::shared_ptr<Level> level)
{
	if (ImGui::TreeNode("Subsurface scattering"))
	{
		auto& data = Engine.Renderer().GetModelRenderer().GetSubsurfaceData();
		ImGui::Text("Models");
		ImGui::SliderFloat("Strength", &data.strength, 0.0f, 25.0f);
		ImGui::SliderFloat("Distortion", &data.distortion, 0.0f, 1.0f);
		ImGui::SliderFloat("Power", &data.power, 0.25f, 10.0f);

		ImGui::Text("Grass");
		bool changed = false;
		GrassChunkMaterial& grassMaterial = level->GetGrass().material;
		changed |= ImGui::SliderFloat("Grass Strength", &grassMaterial.SSSstrength, 0.0f, 25.0f);
		changed |= ImGui::SliderFloat("Grass Distortion", &grassMaterial.SSSdistortion, 0.0f, 1.0f);
		changed |= ImGui::SliderFloat("Grass Power", &grassMaterial.SSSpower, 0.25f, 10.0f);

		if (changed)
		{
			level->GenerateGrass();
		}

		ImGui::TreePop();
	}
}
