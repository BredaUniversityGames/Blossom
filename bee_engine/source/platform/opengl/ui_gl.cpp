#include <precompiled/engine_precompiled.hpp>
#include "ui/ui.hpp"

#include "core/engine.hpp"
#include "core/device.hpp"
#include "core/ecs.hpp"

#include "resources/mesh/mesh_gl.hpp"
#include "resources/image/image_gl.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include "rendering/shader.hpp"
#include "rendering/shader_db.hpp"
#include "resources/resource_manager.hpp"
#include "resources/image/image_common.hpp"

class bee::UIRenderer::Impl
{
public:
};

bee::UIRenderer::UIRenderer()
{
    m_uiPass = Engine.ShaderDB()[ShaderDB::Type::UI];
    m_impl = std::make_unique<Impl>();

    std::vector<float> positions = { -0.5f, -0.5f, 0.0f,
                                  0.5f, -0.5f, 0.0f,
                                 -0.5f,  0.5f, 0.0f,
                                  0.5f,  0.5f, 0.0f };

    std::vector<uint32_t> indices = { 0, 1, 2,
                                      3, 2, 1 };

    std::vector<float> normals = {};
    std::vector<float> uvs = { 0.0f, 1.0f,
                               1.0f, 1.0f,
                               0.0f, 0.0f,
                               1.0f, 0.0f, };

    m_quadMesh = Engine.Resources().Meshes().FromRawData(
        "UI_Quad", std::move(indices), std::move(positions), std::move(normals), std::move(uvs), {}, {}, {}
    );
}

bee::UIRenderer::~UIRenderer()
{
    
}

void bee::UIRenderer::Render()
{
    PushDebugGL("UI pass");
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	float width = (float)Engine.Device().GetWidth();
    float height = (float)Engine.Device().GetHeight();
    float aspect = width / height;
        
    m_uiPass->Activate();
    m_uiPass->GetParameter("camera_projection")->SetValue(glm::ortho(-1.0f * aspect, 1.0f * aspect, -1.0f, 1.0f));

    bool is_menu_drawn = false;

    // UI menus
    {
        auto menus = Engine.ECS().Registry.view<UIMenu>();

        if (!menus.empty())
        {
            is_menu_drawn = true;

            auto menu_ent = menus.front();
            auto menu = menus.get<UIMenu>(menu_ent);
            auto vao = menu.mesh.Retrieve()->vao_handle;
            glBindVertexArray(vao);

            for (int i = 0; i < menu.buttons.size(); i += 1)
            {
                Button& button = menu.buttons[i];

                m_uiPass->GetParameter("button_position")->SetValue(button.position);
                m_uiPass->GetParameter("button_scale")->SetValue(glm::vec2(1.0f, button.inverseAspect) * button.scale);
                m_uiPass->GetParameter("color")->SetValue(button.color);

                auto texture = button.texture.Retrieve()->handle;

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);

                glDrawElements(GL_TRIANGLES, menu.mesh.Retrieve()->index_count, GL_UNSIGNED_INT, 0);
            }
        }
    }

    // UI elements (HUD)
    {
        Engine.ECS().Registry.sort<UIElement>([](const auto& lhs, const auto& rhs)
            {
                return lhs.layer < rhs.layer;
            });

        Engine.ECS().Registry.view<UIElement>().each([&](entt::entity, UIElement& element) {

            if (!element.visible) return;

            if (!element.draw_on_main_menu && is_menu_drawn)
                return;

            if (!element.texture.Valid())
                return;

            auto vao = m_quadMesh.Retrieve()->vao_handle;
            glBindVertexArray(vao);

            float texture_aspect = image_utils::GetAspectRatio(element.texture);

            glm::vec2 quadSize = { 1.0f, 1.0f };
            quadSize.x *= element.scale.x * texture_aspect;
            quadSize.y *= element.scale.y;

            glm::vec2 halfQuadSize = quadSize / 2.0f;

            glm::vec2 pos = element.position * glm::vec2{ aspect, 1.0f };

            pos += CalculateQuadOffset(quadSize, element.origin);

            m_uiPass->GetParameter("button_position")->SetValue(pos);
            m_uiPass->GetParameter("button_scale")->SetValue(glm::vec2(element.scale) * glm::vec2(texture_aspect, 1.0f));

            auto texture = element.texture.Retrieve()->handle;

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

            glDrawElements(GL_TRIANGLES, m_quadMesh.Retrieve()->index_count, GL_UNSIGNED_INT, 0);
        });
    }

    glDisable(GL_BLEND);
    PopDebugGL();
}