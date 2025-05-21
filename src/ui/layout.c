#include <clay.h>

#include <oasis/ui/layout.h>
#include <oasis/utils.h>
#include <stdio.h>

bool pressed1 = false;
bool pressed0 = false;

static int randomize_helper(FILE *in) {
  unsigned int seed;

  if (!in)
    return -1;

  if (fread(&seed, sizeof seed, 1, in) == 1) {
    fclose(in);
    srand(seed);
    return 0;
  }

  fclose(in);
  return -1;
}

void handle_mouse_interaction_0(Clay_ElementId element_id,
                                Clay_PointerData pointer_data,
                                intptr_t user_data) {
  if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    pressed0 = true;
  else
    pressed0 = false;
}

void handle_mouse_interaction_1(Clay_ElementId element_id,
                                Clay_PointerData pointer_data,
                                intptr_t user_data) {
  if (pointer_data.state == CLAY_POINTER_DATA_PRESSED_THIS_FRAME)
    pressed1 = true;
  else
    pressed1 = false;
}

void text(Clay_String text, Clay_Color color) {

  CLAY({0}) {
    CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                      .fontId = 0,
                      .fontSize = 32,
                      .textAlignment = CLAY_TEXT_ALIGN_CENTER,
                      .textColor = color,
                    }));
  };
}

void button_0(Clay_String text, Clay_Color color) {
  static bool color_changed = false;
  static Clay_Color actual_color;

  Clay_Color colors[] = {
    {255, 0, 0, 255},   {0, 255, 0, 255},   {0, 0, 255, 255},
    {255, 255, 0, 255}, {255, 0, 255, 255}, {0, 255, 255, 255},
  };

  randomize_helper(fopen("/dev/urandom", "r"));

  CLAY({
    .id = CLAY_ID("button-0"),
  }) {
    Clay_OnHover(handle_mouse_interaction_0, 0);

    if (!color_changed)
      actual_color = color;

    if (pressed0) {
      actual_color = colors[rand() % 6];
      oasis_log(stdout, LOG_LEVEL_INFO,
                "button_0: pressed and changing color to %s\n",
                clay_color_to_hex(actual_color));
      color_changed = true;
      pressed0 = false;
    }

    CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                      .fontId = 0,
                      .fontSize = 32,
                      .textColor = actual_color,
                    }));
  }
}

void button_1(Clay_String text, Clay_Color color) {
  static bool color_changed = false;
  static Clay_Color actual_color;

  Clay_Color colors[] = {
    {255, 0, 0, 255},   {0, 255, 0, 255},   {0, 0, 255, 255},
    {255, 255, 0, 255}, {255, 0, 255, 255}, {0, 255, 255, 255},
  };

  randomize_helper(fopen("/dev/urandom", "r"));

  CLAY({
    .id = CLAY_ID("button-1"),
  }) {
    Clay_OnHover(handle_mouse_interaction_1, 0);

    if (!color_changed) {
      actual_color = color;
    }

    if (pressed1) {
      actual_color = colors[rand() % 6];
      oasis_log(stdout, LOG_LEVEL_INFO,
                "button_1: pressed and changing color to %s\n",
                clay_color_to_hex(actual_color));
      color_changed = true;
      pressed1 = false;
    }

    CLAY_TEXT(text, CLAY_TEXT_CONFIG({
                      .fontId = 0,
                      .fontSize = 32,
                      .textColor = actual_color,
                    }));
  }
}

Clay_RenderCommandArray layout(void) {
  Clay_BeginLayout();

  Clay_Sizing layout_expand = {
    .width = CLAY_SIZING_GROW(0),
    .height = CLAY_SIZING_GROW(0),
  };

  if (Clay_PointerOver(Clay_GetElementId(CLAY_STRING("button-0")))) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  } else if (Clay_PointerOver(Clay_GetElementId(CLAY_STRING("button-1")))) {
    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
  } else {
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
  }

  Clay_Color content_bg_color = {0, 0, 0, 0};

  CLAY({.id = CLAY_ID("main-container"),
        .backgroundColor = content_bg_color,
        .layout = {
          .layoutDirection = CLAY_TOP_TO_BOTTOM,
          .sizing = layout_expand,
          .childGap = 10,
          .childAlignment = {.y = CLAY_ALIGN_Y_CENTER},
        }}) {
    CLAY({.id = CLAY_ID("text-cell"),
          .layout = {.sizing = {.width = CLAY_SIZING_PERCENT(1.0),
                                .height = CLAY_SIZING_FIXED(50)},
                     .childGap = 10,
                     .childAlignment = {.y = CLAY_ALIGN_Y_CENTER,
                                        .x = CLAY_ALIGN_X_CENTER}}}){
      CLAY({.id = CLAY_ID("text-container"),
            .layout = {
              .childGap = 0,
              .padding =
                {
                  .top = 20,
                  .bottom = 20,
                },
            }}){Clay_Color white = {255, 255, 255, 255};

    text(CLAY_STRING("Hello "), white);
    text(CLAY_STRING("from "), white);
    button_0(CLAY_STRING("Clay"), white);
    text(CLAY_STRING(", "), white);
    text(CLAY_STRING("and "), white);
    button_1(CLAY_STRING("Oasis"), white);
    text(CLAY_STRING("!"), white);
  }
};
}
;

Clay_RenderCommandArray render_commands = Clay_EndLayout();
return render_commands;
}
