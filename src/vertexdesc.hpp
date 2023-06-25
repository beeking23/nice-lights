#pragma once

struct VertexDesc {
  GLint m_attrib;
  GLuint m_offset;
  GLenum m_type;
  GLenum m_count;
  GLuint m_divisor;
  GLsizeiptr m_stride;
  void *m_vertexData;
  size_t m_vertexDataSize;
  GLuint m_bufferID;

  void Create(GLuint vaoID, int bindingIndex);

  static GLuint CreateInterleavedVAO(const std::vector<VertexDesc>& vertexDesc, void *vertexData, GLsizeiptr size, GLsizeiptr stride, GLuint divisor);
  static GLuint CreateArrayOfArraysVAO(std::vector<VertexDesc>& vertexDesc);  
};

