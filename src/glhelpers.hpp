#pragma once

#define GL_CHECK_ERROR() CheckGlError(__LINE__)
void CheckGlError(int line);

struct ShaderDesc {
	 std::string m_source;
	 GLuint m_type;
	 
	 static GLuint CreateShader(const char *src, GLenum type);
	 static GLuint CreateShaderProgram(const std::vector<ShaderDesc>& shaders);
};


