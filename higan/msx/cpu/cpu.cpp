#include <msx/msx.hpp>

namespace higan::MSX {

CPU cpu;
#include "memory.cpp"
#include "serialization.cpp"

auto CPU::main() -> void {
  if(io.irqLine) irq(1, 0x0038, 0xff);
  instruction();
}

auto CPU::step(uint clocks) -> void {
  Thread::step(clocks);
  synchronize(vdp);
  synchronize(psg);
  for(auto peripheral : peripherals) synchronize(*peripheral);
}

auto CPU::synchronizing() const -> bool {
  return scheduler.synchronizing();
}

auto CPU::power() -> void {
  Z80::bus = this;
  Z80::power();
  Thread::create(system.colorburst(), [&] {
    while(true) scheduler.synchronize(), main();
  });

  r.pc = 0x0000;  //reset vector address

  if(Model::MSX()     ) ram.allocate (64_KiB);
  if(Model::MSX2()    ) ram.allocate(256_KiB);
  if(Model::MSX2Plus()) ram.allocate(256_KiB);

  slot[0] = {3, 0, {0, 0, 0, 0}};
  slot[1] = {2, 1, {0, 0, 0, 0}};
  slot[2] = {1, 2, {0, 0, 0, 0}};
  slot[3] = {0, 3, {0, 0, 0, 0}};

  io = {};
}

auto CPU::setIRQ(bool line) -> void {
  io.irqLine = line;
}

}
