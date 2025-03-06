#pragma once

#include <glad/include/glad/glad.h>
#include <string>

namespace bee
{
#ifdef BEE_DEBUG
void InitDebugMessages();
#else
inline void InitDebugMessages() {}
#endif
void RenderQuad();
void RenderCube();
void LabelGL(GLenum type, GLuint name, const std::string& label);
void PushDebugGL(std::string_view message);
void PopDebugGL();
}  // namespace bee
