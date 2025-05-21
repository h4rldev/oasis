#include <oasis/renderer.h>

// Wrappers for renderer function definitions, these are for simpler usage
// accross renderer, and to make it easier to switch renderers

void renderer_init(int width, int height, const char *title,
                   unsigned int flags) {
  // TODO: Initialize renderer
  _RENDERER_INIT(width, height, title, flags);
}

void renderer_close(void) { _RENDERER_CLOSE(); }

void renderer_render(Clay_RenderCommandArray render_commands, Font *fonts) {
  _RENDERER_RENDER(render_commands, fonts);
}

inline Clay_Dimensions renderer_measure_text(Clay_StringSlice text,
                                             Clay_TextElementConfig *config,
                                             void *user_data) {
  return _RENDERER_MEASURE_TEXT(text, config, user_data);
}
