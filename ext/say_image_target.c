#include "say.h"

static say_context *say_image_target_make_context(void *data) {
  return say_context_create();
}

static GLuint say_current_fbo = 0;
static say_context *say_fbo_last_context = NULL;

void say_fbo_make_current(GLuint fbo) {
  say_context *context = say_context_current();

  if (context != say_fbo_last_context ||
      fbo != say_current_fbo) {
    say_current_fbo = fbo;
    say_fbo_last_context = context;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  }
}

static GLuint say_current_rbo = 0;
static say_context *say_rbo_last_context = NULL;

void say_rbo_make_current(GLuint rbo) {
  say_context *context = say_context_current();

  if (context != say_rbo_last_context ||
      rbo != say_current_rbo) {
    say_current_rbo = rbo;
    say_rbo_last_context = context;

    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
  }
}

void say_image_target_will_delete(GLuint fbo, GLuint rbo) {
  if (say_current_fbo == fbo)
    say_current_fbo = 0;

  if (say_current_rbo == rbo)
    say_current_rbo = 0;
}

bool say_image_target_is_available() {
  say_context_ensure();
  return GLEW_EXT_framebuffer_object || GLEW_VERSION_3_0;
}

say_image_target *say_image_target_create() {
  say_context_ensure();
  say_image_target *target = malloc(sizeof(say_image_target));

  target->target = say_target_create();
  target->img    = NULL;

  glGenFramebuffers(1, &target->fbo);
  glGenRenderbuffers(1, &target->rbo);

  return target;
}

void say_image_target_free(say_image_target *target) {
  say_context_ensure();
  say_image_target_will_delete(target->fbo, target->rbo);

  glDeleteRenderbuffers(1, &target->rbo);
  glDeleteFramebuffers(1, &target->fbo);

  say_target_free(target->target);
  free(target);
}

void say_image_target_set_image(say_image_target *target, say_image *image) {
  say_context_ensure();
  target->img = image;

  if (target->img) {
    say_target_set_custom_data(target->target, target);
    say_target_set_context_proc(target->target, say_image_target_make_context);
    say_target_set_bind_hook(target->target, (say_bind_hook)say_image_target_bind);

    say_vector2 size = say_image_get_size(image);

    say_target_set_size(target->target, size);
    say_view_set_size(target->target->view, size);
    say_view_set_center(target->target->view, say_make_vector2(size.x / 2.0,
                                                               size.y / 2.0));

    say_target_make_current(target->target);

    say_image_bind(image);
    glGenerateMipmap(GL_TEXTURE_2D);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, image->texture, 0);

    say_rbo_make_current(target->rbo);
    glRenderbufferStorage(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT,
                          say_image_get_width(image),
                          say_image_get_height(image));

    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, target->rbo);
  }
}

say_image *say_image_target_get_image(say_image_target *target) {
  return target->img;
}

void say_image_target_update(say_image_target *target) {
  if (target->img) {
    say_target_update(target->target);
    say_image_mark_out_of_date(target->img);
  }
}

void say_image_target_bind(say_image_target *target) {
  say_context_ensure();
  say_fbo_make_current(target->fbo);
  glClear(GL_DEPTH_BUFFER_BIT);
}

void say_image_target_unbind() {
  if (say_image_target_is_available())
    say_fbo_make_current(0);
}
