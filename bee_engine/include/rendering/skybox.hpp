#pragma once
#include <code_utils/bee_utils.hpp>
#include "resources/resource_handle.hpp"

#include "resources/image/image.hpp"

namespace bee
{
 
class Skybox
{
public:
    Skybox(ResourceHandle<Image> skyboxImage, std::shared_ptr<Shader> skyboxPass);
    ~Skybox();
    NON_COPYABLE(Skybox);
    NON_MOVABLE(Skybox);

    void Render() const;

#if defined(BEE_PLATFORM_PC)
    void SetSkybox(ResourceHandle<Image> skyboxImage);
#endif
    std::shared_ptr<bee::ImageCubemap> GetSkyboxCubemap();

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

    void ConvertHDRI(ResourceHandle<Image> skyboxImage);

    std::shared_ptr<Shader> m_skyboxPass;
    const uint32_t m_size = 2048;
};

}
