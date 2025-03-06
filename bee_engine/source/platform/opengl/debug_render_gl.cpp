#include <precompiled/engine_precompiled.hpp>
#include "rendering/debug_render.hpp"

#include <platform/opengl/open_gl.hpp>
#include "tools/log.hpp"
#include "core/transform.hpp"
#include "rendering/render_components.hpp"
#include <glm/gtc/type_ptr.hpp>
#include "core/engine.hpp"
#include "core/ecs.hpp"
#include "platform/opengl/shader_gl.hpp"
#include "platform/opengl/open_gl.hpp"
#include <math/geometry.hpp>

using namespace bee;
using namespace glm;

class bee::DebugRenderer::Impl
{
public:
    Impl();
	bool AddLine(const vec3& from, const vec3& to, const vec4& color);
	void Render(const mat4& view, const mat4& projection);

    static int const				m_maxLines = 1048576;
    int								m_linesCount = 0;
    struct VertexPosition3DColor { glm::vec3 Position; glm::vec4 Color; };
    VertexPosition3DColor*			m_vertexArray = nullptr;
    unsigned int					debug_program = 0;
    unsigned int					m_linesVAO = 0;
    unsigned int					m_linesVBO = 0;
};


bee::DebugRenderer::DebugRenderer()
{
    m_categoryFlags =
		DebugCategory::General |
		DebugCategory::Gameplay |
		DebugCategory::Physics |
		DebugCategory::Rendering |
        DebugCategory::AINavigation |
		DebugCategory::AIDecision |
		DebugCategory::Editor;

    m_impl = std::make_unique<Impl>();
}

DebugRenderer::~DebugRenderer()
{
	// TODO: Cleanup!
}

void DebugRenderer::Render()
{
	//Pick the first camera (TODO: add option to set a camera or a camera entity)
	auto cameraView = Engine.ECS().Registry.view<Transform, CameraComponent>();

	//TODO set default value if no camera exists

	Camera frameCamera{};

	if (cameraView.begin() != cameraView.end()) {
		auto cameraTransform = Engine.ECS().Registry.get<Transform>(cameraView.front()).World();
		auto& cameraComponent = Engine.ECS().Registry.get<CameraComponent>(cameraView.front());

		if (cameraComponent.isOrthographic) {
			frameCamera = Camera::Orthographic(
				cameraTransform[3],
				cameraTransform[3] + cameraTransform * vec4(World::FORWARD, 0.0f),
				cameraComponent.aspectRatio,
				cameraComponent.fieldOfView,
				cameraComponent.nearClip,
				cameraComponent.farClip
			);
		}
		else {
			frameCamera = Camera::Perspective(
				cameraTransform[3],
				cameraTransform[3] + cameraTransform * vec4(World::FORWARD, 0.0f),
				cameraComponent.aspectRatio,
				cameraComponent.fieldOfView,
				cameraComponent.nearClip,
				cameraComponent.farClip
			);
		}
	}
	else {
		Log::Warn("RENDERER: No camera exists in the scene to render from.");
	}

	m_impl->Render(frameCamera.GetView(), frameCamera.GetProjection());
}

void DebugRenderer::AddLine(
	DebugCategory::Enum category,
	const vec3& from,
	const vec3& to,
	const vec4& color)
{
    if (!(m_categoryFlags & category)) return;
	m_impl->AddLine(from, to, color);
}

bee::DebugRenderer::Impl::Impl()
{
	m_vertexArray = new VertexPosition3DColor[m_maxLines * 2];

	const auto* const vsSource =
		"#version 460 core												\n\
		layout (location = 1) in vec3 a_position;						\n\
		layout (location = 2) in vec4 a_color;							\n\
		layout (location = 1) uniform mat4 u_worldviewproj;				\n\
		out vec4 v_color;												\n\
																		\n\
		void main()														\n\
		{																\n\
			v_color = a_color;											\n\
			gl_Position = u_worldviewproj * vec4(a_position, 1.0);		\n\
		}";

	const auto* const fsSource =
		"#version 460 core												\n\
		in vec4 v_color;												\n\
		out vec4 frag_color;											\n\
																		\n\
		void main()														\n\
		{																\n\
			frag_color = v_color;										\n\
		}";

	GLuint vertShader = 0;
	GLuint fragShader = 0;
	GLboolean res = GL_FALSE;

	debug_program = glCreateProgram();
	LabelGL(GL_PROGRAM, debug_program, "Debug Renderer Program");

	res = Shader::CompileShader(&vertShader, GL_VERTEX_SHADER, vsSource);
	if (!res)
	{
		Log::Error("DebugRenderer failed to compile vertex shader");
		return;
	}

	res = Shader::CompileShader(&fragShader, GL_FRAGMENT_SHADER, fsSource);
	if (!res)
	{
		Log::Error("DebugRenderer failed to compile fragment shader");
		return;
	}

	glAttachShader(debug_program, vertShader);
	glAttachShader(debug_program, fragShader);

	if (!Shader::LinkProgram(debug_program))
	{
		glDeleteShader(vertShader);
		glDeleteShader(fragShader);
		glDeleteProgram(debug_program);
		Log::Error("DebugRenderer failed to link shader program");
		return;
	}

	glDeleteShader(vertShader);
	glDeleteShader(fragShader);

	glCreateVertexArrays(1, &m_linesVAO);
	glBindVertexArray(m_linesVAO);
	LabelGL(GL_VERTEX_ARRAY, m_linesVAO, "Debug Lines VAO");

	// Allocate VBO
	glGenBuffers(1, &m_linesVBO);	
	glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
	LabelGL(GL_BUFFER, m_linesVBO, "Debug Lines VBO");

	// Allocate into VBO
	const auto size = sizeof(m_vertexArray);
	glBufferData(GL_ARRAY_BUFFER, size, &m_vertexArray[0], GL_STREAM_DRAW);

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(
		1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPosition3DColor),
		reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Position)));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(
		2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexPosition3DColor),
		reinterpret_cast<void*>(offsetof(VertexPosition3DColor, Color)));

	glBindVertexArray(0); // TODO: Only do this when validating OpenGL

}

bool bee::DebugRenderer::Impl::AddLine(const vec3& from, const vec3& to, const vec4& color)
{
	if (m_linesCount < m_maxLines)
	{
		m_vertexArray[m_linesCount * 2].Position = from;
		m_vertexArray[m_linesCount * 2 + 1].Position = to;
		m_vertexArray[m_linesCount * 2].Color = color;
		m_vertexArray[m_linesCount * 2 + 1].Color = color;
		++m_linesCount;
		return true;
	}
	return false;
}

void bee::DebugRenderer::Impl::Render(const mat4& view, const mat4& projection)
{
	// Render debug lines
	glm::mat4 vp = projection * view;
	glUseProgram(debug_program);
	glUniformMatrix4fv(1, 1, false, value_ptr(vp));
	glBindVertexArray(m_linesVAO);

	if (m_linesCount > 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_linesVBO);
		glBufferData(
			GL_ARRAY_BUFFER,
			sizeof(VertexPosition3DColor) * (m_maxLines * 2),
			&m_vertexArray[0],
			GL_DYNAMIC_DRAW);
		glDrawArrays(GL_LINES, 0, m_linesCount * 2);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	m_linesCount = 0;
}
