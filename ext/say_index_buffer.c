#include "say.h"

static void say_ibo_make_current(GLuint ibo) {
  say_context *context = say_context_current();

  if (context->ibo != ibo) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    context->ibo = ibo;
  }
}

static void say_ibo_will_delete(GLuint ibo) {
  mo_array *contexts = say_context_get_all();
  for (size_t i = 0; i < contexts->size; i++) {
    say_context *context = mo_array_get_as(contexts, i, say_context*);
    if (context->ibo == ibo)
      context->ibo = 0;
  }
}

say_index_buffer *say_index_buffer_create(GLenum type, size_t size) {
  say_context_ensure();

  say_index_buffer *buf = malloc(sizeof(say_index_buffer));

  glGenBuffers(1, &buf->ibo);
  buf->type = type;

  mo_array_init(&buf->buffer, sizeof(GLuint));
  mo_array_resize(&buf->buffer, size);

  say_ibo_make_current(buf->ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint),
               NULL, type);

  return buf;
}

void say_index_buffer_free(say_index_buffer *buf) {
  say_context_ensure();

  say_ibo_will_delete(buf->ibo);
  glDeleteBuffers(1, &buf->ibo);
  mo_array_release(&buf->buffer);
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
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, say_context_current()->ibo);
}

void say_index_buffer_update_part(say_index_buffer *buf, size_t index,
                                  size_t size) {
  say_context_ensure();

  say_index_buffer_bind(buf);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
                  index * sizeof(GLuint),
                  size * sizeof(GLuint),
                  mo_array_at(&buf->buffer, index));
}

void say_index_buffer_update(say_index_buffer *buf) {
  say_index_buffer_update_part(buf, 0, buf->buffer.size);
}

size_t say_index_buffer_get_size(say_index_buffer *buf) {
  return buf->buffer.size;
}

void say_index_buffer_resize(say_index_buffer *buf, size_t size) {
  say_context_ensure();

  mo_array_resize(&buf->buffer, size);
  mo_array_shrink(&buf->buffer);

  say_index_buffer_bind(buf);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, size * sizeof(GLuint),
               mo_array_at(&buf->buffer, 0), buf->type);
}

GLuint *say_index_buffer_get(say_index_buffer *buf, size_t i) {
  return mo_array_get_ptr(&buf->buffer, i, GLuint);
}

GLuint say_index_buffer_get_ibo(say_index_buffer *buf) {
  return buf->ibo;
}
