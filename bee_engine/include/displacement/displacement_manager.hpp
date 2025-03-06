#pragma once
#include <code_utils/bee_utils.hpp>

#include <memory>
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>
#include "resources/resource_handle.hpp"

namespace bee
{
class DisplacementManager
{
public:
    DisplacementManager();
    ~DisplacementManager();

    NON_COPYABLE(DisplacementManager);
    NON_MOVABLE(DisplacementManager);

    void Update(float deltaTime);
    glm::mat4 DisplacementMapTransform() const;

    ResourceHandle<bee::Image>& GetTex() { return m_displacementTexture; };

    struct DisplacementWriteParams
    {
        // Position relative to the focus component.
        glm::vec2 position;
        float radius;
    };

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    ResourceHandle<bee::Image> m_displacementTexture;
    ResourceHandle<bee::Image> m_prevDisplacementTexture;
    std::shared_ptr<Shader> m_clearCompute;
    std::shared_ptr<Shader> m_displacementWriteCompute;

    int32_t m_textureWidth = 512, m_textureHeight = 512;
};
}
