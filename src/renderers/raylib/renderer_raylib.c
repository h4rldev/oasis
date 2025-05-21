#include <oasis/renderers/renderer_raylib.h>

#include <raylib.h>
#include <raymath.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <clay.h>

Camera raylib_camera;

// Get a ray trace from the screen position (i.e mouse) within a specific
// section of the screen
Ray get_screen_to_world_point_with_z_distance(Vector2 position, Camera camera,
                                              int screen_width,
                                              int screen_height,
                                              float z_distance) {
  Ray ray = {0};

  // Calculate normalized device coordinates
  // NOTE: y value is negative
  float x = (2.0f * position.x) / (float)screen_width - 1.0f;
  float y = 1.0f - (2.0f * position.y) / (float)screen_height;
  float z = 1.0f;

  // Store values in a vector
  Vector3 device_coords = {x, y, z};

  // Calculate view matrix from camera look at
  Matrix mat_view = MatrixLookAt(camera.position, camera.target, camera.up);

  Matrix mat_proj = MatrixIdentity();

  if (camera.projection == CAMERA_PERSPECTIVE) {
    // Calculate projection matrix from perspective
    mat_proj = MatrixPerspective(camera.fovy * DEG2RAD,
                                 ((double)screen_width / (double)screen_height),
                                 0.01f, z_distance);
  } else if (camera.projection == CAMERA_ORTHOGRAPHIC) {
    double aspect = (double)screen_width / (double)screen_height;
    double top = camera.fovy / 2.0;
    double right = top * aspect;

    // Calculate projection matrix from orthographic
    mat_proj = MatrixOrtho(-right, right, -top, top, 0.01, 1000.0);
  }

  // Unproject far/near points
  Vector3 near_point = Vector3Unproject(
      (Vector3){device_coords.x, device_coords.y, 0.0f}, mat_proj, mat_view);
  Vector3 far_point = Vector3Unproject(
      (Vector3){device_coords.x, device_coords.y, 1.0f}, mat_proj, mat_view);

  // Calculate normalized direction vector
  Vector3 direction = Vector3Normalize(Vector3Subtract(far_point, near_point));

  ray.position = far_point;

  // Apply calculated vectors to ray
  ray.direction = direction;

  return ray;
}

inline Clay_Dimensions raylib_measure_text(Clay_StringSlice text,
                                           Clay_TextElementConfig *config,
                                           void *user_data) {
  // Measure string size for Font
  Clay_Dimensions text_size = {0};

  float max_text_width = 0.0f;
  float line_text_width = 0;

  float text_height = config->fontSize;
  Font *fonts = (Font *)user_data;
  Font font_to_use = fonts[config->fontId];
  // Font failed to load, likely the fonts are in the wrong place relative to
  // the execution dir. RayLib ships with a default font, so we can continue
  // with that built in one.
  if (!font_to_use.glyphs) {
    font_to_use = GetFontDefault();
  }

  float scale_factor = config->fontSize / (float)font_to_use.baseSize;

  for (int i = 0; i < text.length; ++i) {
    if (text.chars[i] == '\n') {
      max_text_width = fmax(max_text_width, line_text_width);
      line_text_width = 0;
      continue;
    }
    int index = text.chars[i] - 32;
    if (font_to_use.glyphs[index].advanceX != 0)
      line_text_width += font_to_use.glyphs[index].advanceX;
    else
      line_text_width +=
          (font_to_use.recs[index].width + font_to_use.glyphs[index].offsetX);
  }

  max_text_width = fmax(max_text_width, line_text_width);

  text_size.width = max_text_width * scale_factor;
  text_size.height = text_height;

  return text_size;
}

void clay_raylib_initialize(int width, int height, const char *title,
                            unsigned int flags) {
  SetConfigFlags(flags);
  InitWindow(width, height, title);
  //    EnableEventWaiting();
}

// A MALLOC'd buffer, that we keep modifying inorder to save from so many Malloc
// and Free Calls. Call Clay_Raylib_Close() to free
static char *temp_render_buffer = NULL;
static int temp_render_buffer_len = 0;

// Call after closing the window to clean up the render buffer
void clay_raylib_close(void) {
  if (temp_render_buffer)
    free(temp_render_buffer);
  temp_render_buffer_len = 0;

  CloseWindow();
}

void clay_raylib_render(Clay_RenderCommandArray render_commands, Font *fonts) {
  for (int j = 0; j < render_commands.length; j++) {
    Clay_RenderCommand *render_command =
        Clay_RenderCommandArray_Get(&render_commands, j);
    Clay_BoundingBox bounding_box = render_command->boundingBox;
    switch (render_command->commandType) {
    case CLAY_RENDER_COMMAND_TYPE_TEXT: {
      Clay_TextRenderData *text_data = &render_command->renderData.text;
      Font font_to_use = fonts[text_data->fontId];

      int strlen = text_data->stringContents.length + 1;

      if (strlen > temp_render_buffer_len) {
        // Grow the temp buffer if we need a larger string
        if (temp_render_buffer)
          temp_render_buffer = realloc(temp_render_buffer, strlen);
        else
          temp_render_buffer = (char *)malloc(strlen);
        temp_render_buffer_len = strlen;
      }

      // Raylib uses standard C strings so isn't compatible with cheap slices,
      // we need to clone the string to append null terminator
      memcpy(temp_render_buffer, text_data->stringContents.chars,
             text_data->stringContents.length);
      temp_render_buffer[text_data->stringContents.length] = '\0';
      DrawTextEx(font_to_use, temp_render_buffer,
                 (Vector2){bounding_box.x, bounding_box.y},
                 (float)text_data->fontSize, (float)text_data->letterSpacing,
                 CLAY_COLOR_TO_RAYLIB_COLOR(text_data->textColor));

      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_IMAGE: {
      Texture2D image_texture =
          *(Texture2D *)render_command->renderData.image.imageData;
      Clay_Color tint_color = render_command->renderData.image.backgroundColor;
      if (tint_color.r == 0 && tint_color.g == 0 && tint_color.b == 0 &&
          tint_color.a == 0) {
        tint_color = (Clay_Color){255, 255, 255, 255};
      }
      DrawTextureEx(image_texture, (Vector2){bounding_box.x, bounding_box.y}, 0,
                    bounding_box.width / (float)image_texture.width,
                    CLAY_COLOR_TO_RAYLIB_COLOR(tint_color));
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_START: {
      BeginScissorMode((int)roundf(bounding_box.x), (int)roundf(bounding_box.y),
                       (int)roundf(bounding_box.width),
                       (int)roundf(bounding_box.height));
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_SCISSOR_END: {
      EndScissorMode();
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_RECTANGLE: {
      Clay_RectangleRenderData *config = &render_command->renderData.rectangle;
      if (config->cornerRadius.topLeft > 0) {
        float radius = (config->cornerRadius.topLeft * 2) /
                       (float)((bounding_box.width > bounding_box.height)
                                   ? bounding_box.height
                                   : bounding_box.width);
        DrawRectangleRounded(
            (Rectangle){bounding_box.x, bounding_box.y, bounding_box.width,
                        bounding_box.height},
            radius, 8, CLAY_COLOR_TO_RAYLIB_COLOR(config->backgroundColor));
      } else {
        DrawRectangle(bounding_box.x, bounding_box.y, bounding_box.width,
                      bounding_box.height,
                      CLAY_COLOR_TO_RAYLIB_COLOR(config->backgroundColor));
      }
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_BORDER: {
      Clay_BorderRenderData *config = &render_command->renderData.border;
      // Left border
      if (config->width.left > 0) {
        DrawRectangle(
            (int)roundf(bounding_box.x),
            (int)roundf(bounding_box.y + config->cornerRadius.topLeft),
            (int)config->width.left,
            (int)roundf(bounding_box.height - config->cornerRadius.topLeft -
                        config->cornerRadius.bottomLeft),
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      // Right border
      if (config->width.right > 0) {
        DrawRectangle(
            (int)roundf(bounding_box.x + bounding_box.width -
                        config->width.right),
            (int)roundf(bounding_box.y + config->cornerRadius.topRight),
            (int)config->width.right,
            (int)roundf(bounding_box.height - config->cornerRadius.topRight -
                        config->cornerRadius.bottomRight),
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      // Top border
      if (config->width.top > 0) {
        DrawRectangle(
            (int)roundf(bounding_box.x + config->cornerRadius.topLeft),
            (int)roundf(bounding_box.y),
            (int)roundf(bounding_box.width - config->cornerRadius.topLeft -
                        config->cornerRadius.topRight),
            (int)config->width.top, CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      // Bottom border
      if (config->width.bottom > 0) {
        DrawRectangle(
            (int)roundf(bounding_box.x + config->cornerRadius.bottomLeft),
            (int)roundf(bounding_box.y + bounding_box.height -
                        config->width.bottom),
            (int)roundf(bounding_box.width - config->cornerRadius.bottomLeft -
                        config->cornerRadius.bottomRight),
            (int)config->width.bottom,
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      if (config->cornerRadius.topLeft > 0) {
        DrawRing(
            (Vector2){roundf(bounding_box.x + config->cornerRadius.topLeft),
                      roundf(bounding_box.y + config->cornerRadius.topLeft)},
            roundf(config->cornerRadius.topLeft - config->width.top),
            config->cornerRadius.topLeft, 180, 270, 10,
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      if (config->cornerRadius.topRight > 0) {
        DrawRing(
            (Vector2){roundf(bounding_box.x + bounding_box.width -
                             config->cornerRadius.topRight),
                      roundf(bounding_box.y + config->cornerRadius.topRight)},
            roundf(config->cornerRadius.topRight - config->width.top),
            config->cornerRadius.topRight, 270, 360, 10,
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      if (config->cornerRadius.bottomLeft > 0) {
        DrawRing(
            (Vector2){roundf(bounding_box.x + config->cornerRadius.bottomLeft),
                      roundf(bounding_box.y + bounding_box.height -
                             config->cornerRadius.bottomLeft)},
            roundf(config->cornerRadius.bottomLeft - config->width.bottom),
            config->cornerRadius.bottomLeft, 90, 180, 10,
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      if (config->cornerRadius.bottomRight > 0) {
        DrawRing(
            (Vector2){roundf(bounding_box.x + bounding_box.width -
                             config->cornerRadius.bottomRight),
                      roundf(bounding_box.y + bounding_box.height -
                             config->cornerRadius.bottomRight)},
            roundf(config->cornerRadius.bottomRight - config->width.bottom),
            config->cornerRadius.bottomRight, 0.1, 90, 10,
            CLAY_COLOR_TO_RAYLIB_COLOR(config->color));
      }
      break;
    }
    case CLAY_RENDER_COMMAND_TYPE_CUSTOM: {
      Clay_CustomRenderData *config = &render_command->renderData.custom;
      CustomLayoutElement *custom_element =
          (CustomLayoutElement *)config->customData;
      if (!custom_element)
        continue;
      switch (custom_element->type) {
      case CUSTOM_LAYOUT_ELEMENT_TYPE_3D_MODEL: {
        Clay_BoundingBox root_box =
            render_commands.internalArray[0].boundingBox;
        float scale_value = CLAY__MIN(CLAY__MIN(1, 768 / root_box.height) *
                                          CLAY__MAX(1, root_box.width / 1024),
                                      1.5f);
        Ray position_ray = get_screen_to_world_point_with_z_distance(
            (Vector2){render_command->boundingBox.x +
                          render_command->boundingBox.width / 2,
                      render_command->boundingBox.y +
                          (render_command->boundingBox.height / 2) + 20},
            raylib_camera, (int)roundf(root_box.width),
            (int)roundf(root_box.height), 140);
        BeginMode3D(raylib_camera);
        DrawModel(custom_element->custom_data.model.model,
                  position_ray.position,
                  custom_element->custom_data.model.scale * scale_value,
                  WHITE); // Draw 3d model with texture
        EndMode3D();
        break;
      }
      default:
        break;
      }
      break;
    }
    default: {
      printf("Error: unhandled render command.");
      exit(1);
    }
    }
  }
}
