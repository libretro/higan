#include "../higan.hpp"
#include "platform.cpp"
#include "video.cpp"
#include "audio.cpp"
#include "input.cpp"
#include "states.cpp"

Emulator emulator;

auto Emulator::create(shared_pointer<higan::Interface> instance, string location) -> void {
  interface = instance;

  system = {};
  system.name = Location::base(location).trimRight("/", 1L);
  system.data = location;
  system.templates = {Path::templates, interface->name(), "/"};

  string configuration = file::read({location, "settings.bml"});
  if(!configuration) {
    auto system = higan::Node::System::create();
    system->name = interface->name();
    system->setProperty("location", location);
    configuration = higan::Node::serialize(system);
  }

  //peripherals may have been renamed or deleted since last run; remove them from the configuration now
  auto document = BML::unserialize(configuration);
  for(auto node : document) validateConfiguration(node, document);
  configuration = BML::serialize(document);

  interface->load(configuration);
  root = interface->root();

  nodeManager.bind(root);
  programWindow.setTitle(system.name);
  programWindow.show(home);
  programWindow.show(nodeManager);
  programWindow.setFocused();

  sound.create(settings.audio.driver);
  sound.setContext(programWindow.handle());
  sound.setBlocking(false);
  sound.setFrequency(48000.0);
  audioSettings.eventActivate();

  inputManager.create();

  Application::onMain({&Emulator::main, this});

  for(auto& display : root->find<higan::Node::Video>()) {
    auto viewport = shared_pointer_make<ViewportWindow>();
    display->setProperty<shared_pointer<ViewportWindow>>("viewport", viewport);
    viewport->create(display);
    viewports.append(viewport);
  }

  //this call will bind all inputs on account of InputManager::devices currently being empty
  inputManager.poll();

  power(false);
}

auto Emulator::main() -> void {
  if(Application::modal()) return (void)usleep(20 * 1000);

  inputManager.poll();
  hotkeys.poll();

  bool viewportFocused = false;
  for(auto& viewport : viewports) {
    if(viewport->focused()) viewportFocused = true;
  }

  if(!system.power || (!viewportFocused && settings.input.unfocused == "Pause")) {
    usleep(20 * 1000);
  } else {
    interface->run();
  }
}

auto Emulator::quit() -> void {
  Application::quit();  //stop processing callbacks and timers
  if(!interface) return;

  if(auto location = root->property("location")) {
    file::write({location, "settings.bml"}, higan::Node::serialize(root));
  }

  viewports.reset();
  sound.reset();
  inputManager.reset();
  Instances::inputManager.destruct();
  Instances::programWindow.destruct();
  root = {};
  interface->unload();
  interface.reset();
  interfaces.reset();
}

auto Emulator::power(bool on) -> void {
  if(system.power = on) {
    for(auto& viewport : viewports) {
      viewport->show(programWindow);
      Application::processEvents();
    }
    videoUpdateColors();
    audioUpdateEffects();
    interface->power();
    //powering on the system latches static settings
    nodeManager.refreshSettings();
  } else {
    for(auto& viewport : viewports) {
      viewport->hide();
    }
    if(programWindow.minimized()) programWindow.setMinimized(false);
    if(!programWindow.maximized()) programWindow.setAlignment(Alignment::Center);
    programWindow.setFocused();
  }
  systemMenu.power.setChecked(on);
  toolsMenu.saveStateMenu.setEnabled(on);
  toolsMenu.loadStateMenu.setEnabled(on);
}

//used to prevent connecting the same (emulated) physical device to multiple ports simultaneously
auto Emulator::connected(string location) -> higan::Node::Port {
  for(auto& peripheral : root->find<higan::Node::Peripheral>()) {
    if(location == peripheral->property("location")) return peripheral->parent.acquire();
  }
  return {};
}

auto Emulator::validateConfiguration(Markup::Node node, Markup::Node parent) -> void {
  for(auto property : node.find("property")) {
    if(property["name"].text() != "location") continue;
    auto location = property["value"].text();
    //if the peripheral is missing, remove it from the tree
    if(!directory::exists(location)) parent.remove(node);
  }
  for(auto branch : node) validateConfiguration(branch, node);
}
