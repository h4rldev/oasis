#include "raylib.h"
#include <clay.h>

#include <oasis/renderer.h>
#include <oasis/ui/layout.h>
#include <oasis/ui/window.h>
#include <oasis/utils.h>

static Clay_RenderCommandArray prepare_layout(Clay_Context *clay_ctx) {
  Clay_SetCurrentContext(clay_ctx);
#if DEBUG == 1
  Clay_SetDebugModeEnabled(true);
#endif
  Clay_SetLayoutDimensions((Clay_Dimensions){
    .width = GetScreenWidth(),
    .height = GetScreenHeight(),
  });

  Vector2 mouse_position = GetMousePosition();
  Vector2 scroll_delta = GetMouseWheelMoveV();

  Clay_SetPointerState((Clay_Vector2){
                         mouse_position.x,
                         mouse_position.y,
                       },
                       IsMouseButtonDown(0));

  Clay_UpdateScrollContainers(true,
                              (Clay_Vector2){scroll_delta.x, scroll_delta.y}, GetFrameTime());

  return layout();
}

int begin_ui_window(int width, int height, const char *title,
                    unsigned int flags) {
  renderer_init(width, height, title, flags);

  Font fonts[1];
  // Using the default font for now, this will be changed later, and perhaps
  // configurable sometime way down the line
  fonts[0] = LoadFontEx("./resources/JBrainsMonoNF.ttf", 48, 0, 400);
  SetTextureFilter(fonts[0].texture, TEXTURE_FILTER_BILINEAR);

  uint64_t required_memory = Clay_MinMemorySize();

  int screen_width = GetScreenWidth();
  int screen_height = GetScreenHeight();

  Clay_Arena memory_top = Clay_CreateArenaWithCapacityAndMemory(
    required_memory, malloc(required_memory));
  Clay_Context *clay_ctx_top = Clay_Initialize(
    memory_top,
    (Clay_Dimensions){.width = screen_width, .height = screen_height},
    (Clay_ErrorHandler){handle_error, NULL});
  Clay_SetMeasureTextFunction(renderer_measure_text, fonts);

  Clay_Arena memory_bottom = Clay_CreateArenaWithCapacityAndMemory(
    required_memory, malloc(required_memory));
  Clay_Context *clay_ctx_bottom = Clay_Initialize(
    memory_bottom,
    (Clay_Dimensions){.width = screen_width, .height = screen_height},
    (Clay_ErrorHandler){handle_error, NULL});
  Clay_SetMeasureTextFunction(renderer_measure_text, fonts);

  while (!WindowShouldClose()) {
    Clay_RenderCommandArray render_commands_top = prepare_layout(clay_ctx_top);
    Clay_RenderCommandArray render_commands_bottom =
      prepare_layout(clay_ctx_bottom);

    BeginDrawing();
    ClearBackground(BLACK);

    renderer_render(render_commands_top, fonts);
    renderer_render(render_commands_bottom, fonts);
    EndDrawing();
  }

  clay_raylib_close();

  return 0;
}
