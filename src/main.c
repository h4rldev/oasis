#define CLAY_IMPLEMENTATION
#include <clay.h>

#include <oasis/audio/playback.h>
#include <oasis/renderer.h>
#include <oasis/ui/window.h>
#include <oasis/utils.h>

int main(void) {
  /*return begin_ui_window(1024, 768, "Oasis",
                         FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);*/
  return playback_play("./resources/test.flac");
}
