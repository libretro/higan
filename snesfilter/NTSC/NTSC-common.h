#include <nall/platform.hpp>
#include <nall/stdint.hpp>
using namespace nall;

#include "snes_ntsc_config_custom.h"
#include "snes_ntsc/snes_ntsc.h"
#include "snes_ntsc/snes_ntsc.c"

extern "C" {
  void filter_size(unsigned&, unsigned&);
  void filter_render(uint32_t*, unsigned, const uint32_t*, unsigned, unsigned, unsigned);
};

struct snes_ntsc_t *ntsc;
snes_ntsc_setup_t setup;
int burst;
int burst_toggle;

void initialize() {
  static bool initialized = false;
  if(initialized == true) return;
  initialized = true;

  ntsc = (snes_ntsc_t*)malloc(sizeof *ntsc);
  setup = NTSC_PROFILE;
  setup.merge_fields = 1;
  snes_ntsc_init(ntsc, &setup);

  burst = 0;
  burst_toggle = (setup.merge_fields ? 0 : 1);
}

void terminate() {
  if(ntsc) free(ntsc);
}

dllexport void filter_size(unsigned &width, unsigned &height) {
  initialize();
  width  = SNES_NTSC_OUT_WIDTH(256);
  height = height;
}

dllexport void filter_render(
  uint32_t *output, unsigned outpitch,
  const uint32_t *input, unsigned pitch, unsigned width, unsigned height
) {
  initialize();
  if(!ntsc) return;

  pitch /= sizeof *input; // convert from bytes to pixels

  if(width <= 256) {
    snes_ntsc_blit      (ntsc, input, pitch, burst, width, height, output, outpitch );
  } else {
    snes_ntsc_blit_hires(ntsc, input, pitch, burst, width, height, output, outpitch );
  }

  burst ^= burst_toggle;
}
