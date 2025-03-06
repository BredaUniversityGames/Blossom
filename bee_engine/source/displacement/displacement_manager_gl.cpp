#include <precompiled/engine_precompiled.hpp>
#include "displacement/displacement_manager.hpp"

#include <platform/opengl/open_gl.hpp>

#include <core/time.hpp>
#include "core/ecs.hpp"
#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "core/transform.hpp"
#include "displacement/displacer.hpp"
#include "platform/opengl/gl_uniform.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "rendering/shader_db.hpp"
#include "resources/resource_manager.hpp"
#include "resources/image/image_gl.hpp"
#include "resources/image/image_loader.hpp"
#include "tools/log.hpp"

class bee::DisplacementManager::Impl
{
public:
    Uniform<std::array<DisplacementWriteParams, 16>> m_displacementParams;

    void SetupTexture(ResourceHandle<bee::Image>, int32_t width, int32_t height);
    void ClearTexture(ResourceHandle<bee::Image>, int32_t width, int32_t height, std::shared_ptr<bee::Shader> clearCompute);
};

bee::DisplacementManager::DisplacementManager() : m_impl(std::make_unique<Impl>())
{
    m_clearCompute = Engine.ShaderDB()[ShaderDB::Type::CLEAR_TEX];
    m_displacementWriteCompute = Engine.ShaderDB()[ShaderDB::Type::WRITE_DISPLACMENTS];

    m_displacementTexture = Engine.Resources().Images().FromRawData(nullptr, ImageFormat::RGBA8, m_textureWidth, m_textureHeight);
    m_prevDisplacementTexture = Engine.Resources().Images().FromRawData(nullptr, ImageFormat::RGBA8, m_textureWidth, m_textureHeight);

    m_impl->SetupTexture(m_displacementTexture, m_textureWidth, m_textureHeight);
    m_impl->SetupTexture(m_prevDisplacementTexture, m_textureWidth, m_textureHeight);

    m_impl->m_displacementParams.SetName("Displacement params buffer");
}

bee::DisplacementManager::~DisplacementManager()
{
}

void bee::DisplacementManager::Update(float deltaTime)
{
    PushDebugGL("Displacement pass");
    auto displacerView{ Engine.ECS().Registry.view<const Displacer, const Transform>()};
    auto displacerFocusView{ Engine.ECS().Registry.view<DisplacerFocus, const Transform>()};

    glm::vec3 displacementFocusPos{};
    float displacementInfluenceSize{};
    float angle{};

    if (displacerFocusView.size_hint() > 0)
    {
        entt::entity displacerFocusEntity = *displacerFocusView.begin();
        auto [displacerFocus, focusTransform] { displacerFocusView.get(displacerFocusEntity) };

        angle = glm::eulerAngles(focusTransform.GetRotation()).z;

        glm::mat4 focusWorld = focusTransform.World();
        glm::mat4 inverseFocusWorld = glm::inverse(focusWorld);
        displacementFocusPos = *reinterpret_cast<const glm::vec3*>(&focusWorld[3][0]);
        displacementInfluenceSize = displacerFocus.influenceSize;

        std::vector<DisplacementWriteParams> displacementParams{};

        for (auto displacerEntity : displacerView)
        {
            if (displacementParams.size() == m_impl->m_displacementParams->size())
            {
                Log::Warn("Maximum amount of displacers reached (" + std::to_string(m_impl->m_displacementParams->size()) + ")");
                break;
            }

            auto [terrainDisplacer, displacerTransform] = displacerView.get(displacerEntity);

            // Get world position of current displacer.
            glm::mat4 displacerWorld{ displacerTransform.World() };
            glm::vec4 position{ displacerWorld[3][0], displacerWorld[3][1],
                               displacerWorld[3][2], 1.0 };

            // Convert world position of displacer to local position of the focus displacer.
            position = inverseFocusWorld * position;

            DisplacementWriteParams param;
            // Set UV position coordinates.
            param.position = glm::vec2{ position.x, position.y };
            param.radius = terrainDisplacer.radius;
            displacementParams.emplace_back(param);
        }

        glm::vec3 displacerFocusVelocity{ displacementFocusPos - displacerFocus.previousPosition };
        glm::vec2 vel2d{ glm::vec4(displacerFocusVelocity.x, displacerFocusVelocity.y, 0.0f, 0.0f) };

        // Steps:
        // 1. Copy displacement to GPU memory (patch UBO).
        std::copy(displacementParams.begin(), displacementParams.end(), m_impl->m_displacementParams.get()->begin());
        m_impl->m_displacementParams.Patch();

        // 2. Copy current displacement into the previousDisplacement.
        std::swap(m_prevDisplacementTexture, m_displacementTexture);

        // 3. Clear the current displacement surface.
        m_impl->ClearTexture(m_displacementTexture, m_textureWidth, m_textureWidth, m_clearCompute);

        // 4. Write displacements into the current surface.
        auto seconds = Engine.GetTime().GetTotalTime().count() / 1000.0f;

        m_displacementWriteCompute->Activate();
        m_displacementWriteCompute->GetParameter("u_time")->SetValue(seconds);
        m_displacementWriteCompute->GetParameter("u_deltaTime")->SetValue(deltaTime);
        m_displacementWriteCompute->GetParameter("u_velocity")->SetValue(vel2d);
        m_displacementWriteCompute->GetParameter("u_angle")->SetValue(angle - glm::pi<float>() / 2.0f);
        m_displacementWriteCompute->GetParameter("u_influenceSize")->SetValue(displacementInfluenceSize);
        m_displacementWriteCompute->GetParameter("u_displacementCount")->SetValue(static_cast<int32_t>(displacementParams.size()));


        glBindBufferBase(GL_UNIFORM_BUFFER, 10, m_impl->m_displacementParams.buffer);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_prevDisplacementTexture.Retrieve()->handle);

        glBindImageTexture(1, m_displacementTexture.Retrieve()->handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
        glDispatchCompute(m_textureWidth / 16, m_textureHeight / 16, 1);
        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        displacerFocus.previousPosition = displacementFocusPos;
    }
    PopDebugGL();
}

glm::mat4 bee::DisplacementManager::DisplacementMapTransform() const
{
    glm::mat4 matrix{ glm::identity<glm::mat4>() };
    auto displacerFocusView{ Engine.ECS().Registry.view<DisplacerFocus, const Transform>() };
    if (displacerFocusView.size_hint() > 0)
    {
        entt::entity displacerFocusEntity = *displacerFocusView.begin();
        auto [displacerFocus, focusTransform] { displacerFocusView.get(displacerFocusEntity) };
        matrix = glm::translate(matrix, glm::vec3{ 0.5f });
        matrix = glm::scale(matrix, 1.0f / glm::vec3{ displacerFocus.influenceSize });

        matrix = glm::translate(matrix, -focusTransform.GetTranslation());
    }

    return matrix;
}

void bee::DisplacementManager::Impl::SetupTexture(ResourceHandle<bee::Image> handle , int32_t width, int32_t height)
{
    glBindTexture(GL_TEXTURE_2D, handle.Retrieve()->handle);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
}

void bee::DisplacementManager::Impl::ClearTexture(ResourceHandle<bee::Image> handle, int32_t width, int32_t height, std::shared_ptr<bee::Shader> clearCompute)
{
    clearCompute->Activate();
    clearCompute->GetParameter("clearColor")->SetValue(glm::vec4{ 0.5f, 0.5f, 0.5f, 1.0f });
    glBindImageTexture(0, handle.Retrieve()->handle, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glDispatchCompute(width / 16, height / 16, 1);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
