#include "say.h"

static GLuint       say_current_ibo      = 0;
static say_context *say_ibo_last_context = NULL;

static void say_ibo_make_current(GLuint ibo) {
  say_context *context = say_context_current();

  if (context != say_ibo_last_context ||
      ibo != say_current_ibo) {
    say_current_ibo      = ibo;
    say_ibo_last_context = context;

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  }
}

static void say_ibo_will_delete(GLuint ibo) {
  if (ibo == say_current_ibo) {
    say_current_ibo = 0;
  }
}

say_index_buffer *say_index_buffer_create(GLenum type, size_t size) {
  say_context_ensure();

  say_index_buffer *buf = malloc(sizeof(say_index_buffer));

  glGenBuffers(1, &buf->ibo);
  buf->type = type;

  buf->buffer = say_array_create(sizeof(GLuint), NULL, NULL);
  say_array_resize(buf->buffer, size);

  say_ibo_make_current(buf->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint),
               NULL, type);

  return buf;
}

void say_index_buffer_free(say_index_buffer *buf) {
  say_context_ensure();

  say_ibo_will_delete(buf->ibo);
  glDeleteBuffers(1, &buf->ibo);
  say_array_free(buf->buffer);
  free(buf);
}

void say_index_buffer_bind(say_index_buffer *buf) {
  say_context_ensure();
  say_ibo_make_current(buf->ibo);
}

void say_index_buffer_unbind() {
  say_context_ensure();
  say_ibo_make_current(0);
}

void say_index_buffer_rebind() {
  if (say_ibo_last_context == say_context_current())
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, say_current_ibo);
}

void say_index_buffer_update_part(say_index_buffer *buf, size_t index,
                                  size_t size) {
  say_context_ensure();

  say_index_buffer_bind(buf);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                  index * sizeof(GLuint),
                  size * sizeof(GLuint),
                  say_array_get(buf->buffer, index));
}

void say_index_buffer_update(say_index_buffer *buf) {
  say_index_buffer_update_part(buf, 0, say_array_get_size(buf->buffer));
}

size_t say_index_buffer_get_size(say_index_buffer *buf) {
  return say_array_get_size(buf->buffer);
}

void say_index_buffer_resize(say_index_buffer *buf, size_t size) {
  say_context_ensure();

  say_array_resize(buf->buffer, size);

  say_index_buffer_bind(buf);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint),
               say_array_get(buf->buffer, 0), buf->type);
}

GLuint *say_index_buffer_get(say_index_buffer *buf, size_t i) {
  return say_array_get(buf->buffer, i);
}
