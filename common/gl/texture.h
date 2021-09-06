#pragma once

#include <GL/glew.h>

class Texture {
 public:
  GLuint ID = GL_ZERO;

  explicit Texture(bool create = true) {
    if (create) Create();
  }

  void Create() {
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);

    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    // set texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ID = texture_id;
  }

  void Bind(GLint n, GLuint program, const GLchar *name) {
    glActiveTexture(GL_TEXTURE0 + n);
    glBindTexture(GL_TEXTURE_2D, ID);
    glUniform1i(glGetUniformLocation(program, name), n);
  }

  void Fill(GLsizei width, GLsizei height, const void *pixels,
      GLenum format = GL_RED/*1:GL_RED;3:GL_RGB;4:GL_RGBA*/) {
    glBindTexture(GL_TEXTURE_2D, ID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
                 GL_UNSIGNED_BYTE, pixels);
  }

  void Delete() {
    glDeleteTextures(1, &ID);
  }
};
