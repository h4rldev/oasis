#ifndef RENDERER_RAYLIB_H
#define RENDERER_RAYLIB_H

#if _STDC_VERSION_ < 199901L
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

Camera Raylib_camera;

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
  } customData;
} CustomLayoutElement;

Ray GetScreenToWorldPointWithZDistance(Vector2 position, Camera camera,
                                       int screenWidth, int screenHeight,
                                       float zDistance);

Clay_Dimensions Raylib_MeasureText(Clay_StringSlice text,
                                   Clay_TextElementConfig *config,
                                   void *userData);

void Clay_Raylib_Initialize(int width, int height, const char *title,
                            unsigned int flags);

void Clay_Raylib_Close(void);

void Clay_Raylib_Render(Clay_RenderCommandArray renderCommands, Font *fonts);

#endif
