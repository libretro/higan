#pragma once

//license: GPLv3
//started: 2019-02-19

#include <emulator/emulator.hpp>
#include <emulator/thread.hpp>
#include <emulator/scheduler.hpp>
#include <emulator/cheat.hpp>

#include <component/processor/z80/z80.hpp>
#include <component/video/tms9918/tms9918.hpp>
#include <component/audio/sn76489/sn76489.hpp>

namespace higan::ColecoVision {
  extern Scheduler scheduler;
  extern Cheat cheat;

  struct Thread : higan::Thread {
    auto create(double frequency, function<void ()> entryPoint) -> void {
      higan::Thread::create(frequency, entryPoint);
      scheduler.append(*this);
    }

    auto destroy() -> void {
      scheduler.remove(*this);
      higan::Thread::destroy();
    }

    inline auto synchronize(Thread& thread) -> void {
      if(clock() >= thread.clock()) scheduler.resume(thread);
    }
  };

  struct Model {
    inline static auto ColecoVision() -> bool;
    inline static auto ColecoAdam() -> bool;
  };

  struct Region {
    inline static auto NTSC() -> bool;
    inline static auto PAL() -> bool;
  };

  #include <cv/controller/controller.hpp>

  #include <cv/cpu/cpu.hpp>
  #include <cv/vdp/vdp.hpp>
  #include <cv/psg/psg.hpp>

  #include <cv/system/system.hpp>
  #include <cv/cartridge/cartridge.hpp>
}

#include <cv/interface/interface.hpp>
