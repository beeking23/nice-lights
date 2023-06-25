#include <GL/glew.h>
#include <vector>
#include <cassert>
#include <cstdio>
#include <string>
#include "glhelpers.hpp"
#include "vertexdesc.hpp"

GLuint VertexDesc::CreateInterleavedVAO(const std::vector<VertexDesc>& vertexDesc, void *vertexData, GLsizeiptr size, GLsizeiptr stride, GLuint divisor)
{
  GLuint vaoID;
  glCreateVertexArrays(1, &vaoID);
  GL_CHECK_ERROR();
  
  GLuint bufferID;
  glCreateBuffers(1, &bufferID);
  GL_CHECK_ERROR();
  
  glNamedBufferStorage(bufferID, size, vertexData, 0);
  GL_CHECK_ERROR();

  static const int bindingIndex = 0;
  
  glVertexArrayVertexBuffer(vaoID, bindingIndex, bufferID, 0, stride);
  GL_CHECK_ERROR();  

  for(auto& desc : vertexDesc) {
    if(desc.m_attrib >= 0) {
      glVertexArrayAttribFormat(vaoID, desc.m_attrib, desc.m_count, desc.m_type, GL_FALSE, desc.m_offset);
      GL_CHECK_ERROR();
      
      glVertexArrayAttribBinding(vaoID, desc.m_attrib, bindingIndex);
      GL_CHECK_ERROR();
      
      glEnableVertexArrayAttrib(vaoID, desc.m_attrib);
      GL_CHECK_ERROR();
    }
  }

  glVertexArrayBindingDivisor(vaoID, bindingIndex, divisor);
  GL_CHECK_ERROR();
  
  return vaoID;
}

GLuint VertexDesc::CreateArrayOfArraysVAO(std::vector<VertexDesc>& vertexDesc)
{
  GLuint vaoID;
  glCreateVertexArrays(1, &vaoID);
  GL_CHECK_ERROR();

  int bindingIndex = 0;
  for(auto& desc : vertexDesc)
    desc.Create(vaoID, bindingIndex++);

  return vaoID;
}

void VertexDesc::Create(GLuint vaoID, int bindingIndex)
{
  m_bufferID = 0;
  if(m_attrib < 0)
    return;
  
  glCreateBuffers(1, &m_bufferID);
  GL_CHECK_ERROR();
  
  glNamedBufferStorage(m_bufferID, m_vertexDataSize, m_vertexData, GL_DYNAMIC_STORAGE_BIT);
  GL_CHECK_ERROR();
  
  glVertexArrayVertexBuffer(vaoID, bindingIndex, m_bufferID, 0, m_stride);
  GL_CHECK_ERROR();  

  glVertexArrayAttribFormat(vaoID, m_attrib, m_count, m_type, GL_FALSE, m_offset);
  GL_CHECK_ERROR();
      
  glVertexArrayAttribBinding(vaoID, m_attrib, bindingIndex);
  GL_CHECK_ERROR();
      
  glEnableVertexArrayAttrib(vaoID, m_attrib);
  GL_CHECK_ERROR();

  glVertexArrayBindingDivisor(vaoID, bindingIndex, m_divisor);
  GL_CHECK_ERROR();
}
