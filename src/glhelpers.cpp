#include <GL/glew.h>
#include <stdio.h>
#include <cassert>
#include <cstddef>
#include <vector>
#include <string>

#include "glhelpers.hpp"

void CheckGlError(int line)
{
  GLenum err;
  if((err = glGetError()) != GL_NO_ERROR) {
    printf("GL Error %u at line %i\n", err, line);
    assert(false);
  }
}

GLuint ShaderDesc::CreateShader(const char *src, GLenum type)
{
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL);
  glCompileShader(shader);
  GL_CHECK_ERROR();

  GLint status;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
  printf("Shader compile status %d\n", status);

  GLchar errorLog[512];
  glGetShaderInfoLog(shader, 512, NULL, errorLog);
  if(errorLog[0])
    printf("Shader error log: %s\n", errorLog);

  return shader;
}

GLuint ShaderDesc::CreateShaderProgram(const std::vector<ShaderDesc>& shaders)
{
  std::vector<GLuint> shaderIds;
  for(auto& s : shaders)
    shaderIds.push_back(CreateShader(s.m_source.c_str(), s.m_type));

  GLuint program = glCreateProgram();
  for(auto& i : shaderIds)
    glAttachShader(program, i);
  glLinkProgram(program);

  GLint status;
  glGetProgramiv(program, GL_LINK_STATUS, &status);
  printf("Shader link status %d\n", status);

  GLchar error_log[512];
  glGetProgramInfoLog(program, 512, NULL, error_log);
  if(error_log[0] != 0)
    printf("Program error log: %s\n", error_log);

  for(auto& i : shaderIds)
    glDeleteShader(i);

  return program;
}
