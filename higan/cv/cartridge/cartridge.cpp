#include <cv/cv.hpp>

namespace higan::ColecoVision {

Cartridge cartridge;
#include "serialization.cpp"

auto Cartridge::load(Node::Object parent, Node::Object from) -> void {
  port = Node::Port::create("Cartridge Slot", "Cartridge");
  port->allocate = [&] { return Node::Peripheral::create(interface->name()); };
  port->attach = [&](auto node) { connect(node); };
  port->detach = [&](auto node) { disconnect(); };
  if(from = Node::load(port, from)) {
    if(auto node = from->find<Node::Peripheral>(0)) port->connect(node);
  }
  parent->append(port);
}

auto Cartridge::connect(Node::Peripheral with) -> void {
  node = Node::Peripheral::create(interface->name());
  node->load(with);

  information = {};

  if(auto fp = platform->open(node, "metadata.bml", File::Read, File::Required)) {
    information.metadata = fp->reads();
  }

  auto document = BML::unserialize(information.metadata);

  if(auto memory = document["game/board/memory(type=ROM,content=Program)"]) {
    rom.allocate(memory["size"].natural());
    if(auto fp = platform->open(node, "program.rom", File::Read, File::Required)) {
      rom.load(fp);
    }
  }

  power();
  port->prepend(node);
}

auto Cartridge::disconnect() -> void {
  if(!node) return;
  rom.reset();
  node = {};
}

auto Cartridge::save() -> void {
}

auto Cartridge::power() -> void {
}

}
