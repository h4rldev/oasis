#ifndef RENDERER_RAYLIB_H
#define RENDERER_RAYLIB_H

#if __STDC_VERSION__ < 199901L
#error "This program requires C99, and needs to compile with -std=c99"
#endif

#include <clay.h>
#include <raylib.h>

#define CLAY_RECTANGLE_TO_RAYLIB_RECTANGLE(rectangle)                          \
  (Rectangle) {                                                                \
    .x = rectangle.x, .y = rectangle.y, .width = rectangle.width,              \
    .height = rectangle.height                                                 \
  }
#define CLAY_COLOR_TO_RAYLIB_COLOR(color)                                      \
  (Color) {                                                                    \
    .r = (unsigned char)roundf(color.r), .g = (unsigned char)roundf(color.g),  \
    .b = (unsigned char)roundf(color.b), .a = (unsigned char)roundf(color.a)   \
  }

typedef enum { CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL } CustomLayoutElementType;

typedef struct {
  Model model;
  float scale;
  Vector3 position;
  Matrix rotation;
} CustomLayoutElement_3DModel;

typedef struct {
  CustomLayoutElementType type;
  union {
    CustomLayoutElement_3DModel model;
  } custom_data;
} CustomLayoutElement;

Ray get_screen_to_world_point_with_z_distance(Vector2 position, Camera camera,
                                              int screen_width,
                                              int screen_height,
                                              float z_distance);

Clay_Dimensions raylib_measure_text(Clay_StringSlice text,
                                    Clay_TextElementConfig *config,
                                    void *user_data);

void clay_raylib_initialize(int width, int height, const char *title,
                            unsigned int flags);

void clay_raylib_close(void);

void clay_raylib_render(Clay_RenderCommandArray render_commands, Font *fonts);

#endif
