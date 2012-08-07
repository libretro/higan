/*
  audio.ao (2010-11-15)
  authors: Nach, RedDwarf
  contributor: Kernigh
*/

#include <stdio.h> // snprintf
#include <ao/ao.h>

namespace ruby {

class pAudioAO {
public:
  ao_device *audio_device;

  struct {
    int16_t *data;  // signed 16-bit samples, native byte order
    unsigned index;
    unsigned size;
  } buffer;

  struct {
    unsigned frequency;
    unsigned latency;
  } settings;

  bool cap(const string& name) {
    if(name == Audio::Frequency) return true;
    if(name == Audio::Latency) return true;
    return false;
  }

  any get(const string& name) {
    if(name == Audio::Frequency) return settings.frequency;
    if(name == Audio::Latency) return settings.latency;
    return false;
  }

  bool set(const string& name, const any& value) {
    if(name == Audio::Frequency) {
      settings.frequency = any_cast<unsigned>(value);
      if(audio_device) init();
      return true;
    }

    if(name == Audio::Latency) {
      settings.latency = any_cast<unsigned>(value);
      if(audio_device) init();
      return true;
    }

    return false;
  }

  void sample(uint16_t l_sample, uint16_t r_sample) {
    buffer.data[buffer.index++] = l_sample;
    buffer.data[buffer.index++] = r_sample;
    if(buffer.index < buffer.size) return;

    ao_play(audio_device, (char *)buffer.data, buffer.size * sizeof(*buffer.data));
    buffer.index = 0;
  }

  void clear() {
  }

  bool init() {
    term();

    int driver_id = ao_default_driver_id(); //ao_driver_id((const char*)driver)
    if(driver_id < 0) return false;

    // libao >= 1.0.0 added a new field driver_format.matrix,
    // need { 0 } to avoid a crash by bad pointer
    ao_sample_format driver_format = { 0 };
    driver_format.bits = 16;
    driver_format.channels = 2;
    driver_format.rate = settings.frequency;
    driver_format.byte_format = AO_FMT_NATIVE;

    ao_option *options = 0;
    ao_info *di = ao_driver_info(driver_id);
    if(!di) return false;
    if(!strcmp(di->short_name, "alsa")) {
      char latency_str[32];

      // reduce latency (default was 500ms)
      snprintf(latency_str, sizeof latency_str, "%u", settings.latency);
      ao_append_option(&options, "buffer_time", latency_str);
    }

    audio_device = ao_open_live(driver_id, &driver_format, options);
    if(!audio_device) return false;

    // (2 samples per stereo frame) * (frames per second) * (seconds of latency)
    buffer.size = 2 * unsigned(settings.frequency * settings.latency / 1000.0 + 0.5);
    buffer.data = new int16_t[buffer.size];
    buffer.index = 0;

    return true;
  }

  void term() {
    if(buffer.data) {
      delete[] buffer.data;
      buffer.data = 0;
    }

    if(audio_device) {
      ao_close(audio_device);
      audio_device = 0;
    }
  }

  pAudioAO() {
    audio_device = 0;
    ao_initialize();

    buffer.data = 0;

    settings.frequency = 22050;
    settings.latency = 80;
  }

  ~pAudioAO() {
    term();
    ao_shutdown();
  }
};

DeclareAudio(AO)

};
