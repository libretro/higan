ifeq ($(core),sfc)
  corename := sfc
  include sfc/GNUmakefile
  include gb/GNUmakefile
  flags += -DSFC_SUPERGAMEBOY
else ifeq ($(core),)
  corename := sfc
  include sfc/GNUmakefile
  include gb/GNUmakefile
  flags += -DSFC_SUPERGAMEBOY
endif

ifeq ($(platform),windows)
  name := higan_$(corename)_libretro.dll
else ifeq ($(platform),macos)
  name := higan_$(corename)_libretro.dylib
else
  name := higan_$(corename)_libretro.so
endif

# Avoid polluting the resulting binary with symbols that aren't part
# of the libretro API.
ifeq ($(platform),macos)
  link += -Wl,-exported_symbol,retro_*
else
  link += -Wl,--version-script=target-libretro/link.T -Wl,--no-undefined
endif

#include fc/GNUmakefile
#include ms/GNUmakefile
#include md/GNUmakefile
#include pce/GNUmakefile
#include gba/GNUmakefile
#include ws/GNUmakefile
include processor/GNUmakefile

# rules
objects := libretro_$(corename) $(objects)
objects := $(patsubst %,obj/%.o,$(objects))

obj/libretro_$(corename).o: target-libretro/libretro.cpp
	$(compiler) $(cppflags) $(flags) -c $< -o $@ -Dlibretro_use_$(corename)

# targets
build: $(objects)
	$(strip $(compiler) -o out/$(name) $(objects) $(link))

