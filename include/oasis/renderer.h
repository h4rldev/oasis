#ifndef RENDERER_H
#define RENDERER_H

// TODO: Add renderer wrappers for each renderer

#if RENDERER == RAYLIB
#include <oasis/renderers/renderer_raylib.h>
#define _RENDERER_INIT(x, y, title, flags)                                     \
  clay_raylib_initialize(x, y, title, flags)
#define _RENDERER_CLOSE() clay_raylib_close()
#define _RENDERER_RENDER(render_commands, fonts)                               \
  clay_raylib_render(render_commands, fonts)
#define _RENDERER_MEASURE_TEXT(text, config, user_data)                        \
  raylib_measure_text(text, config, user_data)
#endif

void renderer_init(int width, int height, const char *title,
                   unsigned int flags);
void renderer_close(void);
void renderer_render(Clay_RenderCommandArray render_commands, Font *fonts);

Clay_Dimensions renderer_measure_text(Clay_StringSlice text,
                                      Clay_TextElementConfig *config,
                                      void *user_data);
#endif
