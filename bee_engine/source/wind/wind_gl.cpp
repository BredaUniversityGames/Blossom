#include <precompiled/engine_precompiled.hpp>
#include "wind/wind.hpp"

#include "core/engine.hpp"
#include "core/fileio.hpp"
#include "resources/image/image_loader.hpp"
#include "resources/image/image_gl.hpp"
#include "resources/resource_manager.hpp"
#include "rendering/shader.hpp"
#include "rendering/shader_db.hpp"

#include "../assets/shaders/locations.glsl"

class bee::WindMap::Impl
{
public:

    GLuint m_ambientWindUBO;
};

bee::WindMap::WindMap()
{
    m_impl = std::make_unique<Impl>();

    m_ambientWind.direction = glm::radians(45.0f);
    m_ambientWind.strength = 0.3f;
    m_ambientWind.speed = 0.05f;

    glGenBuffers(1, &m_impl->m_ambientWindUBO);
    glBindBuffer(GL_UNIFORM_BUFFER, m_impl->m_ambientWindUBO);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(m_ambientWind), &m_ambientWind, GL_STATIC_READ);
    glBindBufferBase(GL_UNIFORM_BUFFER, AMBIENT_WIND_LOCATION, m_impl->m_ambientWindUBO);

    auto compShader = Engine.ShaderDB().Get(bee::ShaderDB::Type::POPULATE_3DTEX_PERLIN);

    const uint32_t size{ 512 };
    m_wind = Engine.Resources().Images().FromRawData(nullptr, ImageFormat::RGBA8, size, size, 1);
    compShader->Activate();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, m_wind.Retrieve()->handle);
    glBindImageTexture(0, m_wind.Retrieve()->handle, 0, GL_TRUE, 0, GL_READ_WRITE, GL_RGBA8);
    glDispatchCompute(size / 8, size / 8, size / 8);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    compShader->Deactivate();
}

bee::WindMap::~WindMap()
{
    glDeleteTextures(1, &m_wind.Retrieve()->handle);
}

bee::ResourceHandle<bee::Image3D> bee::WindMap::GetWindImage()
{
	return m_wind;
}