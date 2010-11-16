/*
  audio.openal (2010-11-15)
  author: Nach
  contributors: byuu, wertigon, _willow_, Kernigh
*/

#include <unistd.h> // usleep

#if defined(PLATFORM_OSX)
  #include <OpenAL/al.h>
  #include <OpenAL/alc.h>
#else
  #include <AL/al.h>
  #include <AL/alc.h>
#endif

namespace ruby {

class pAudioOpenAL {
public:
  struct {
    ALCdevice *handle;
    ALCcontext *context;
    ALuint source;
    ALenum format;
  } device;

  struct {
    ALuint q_buffers[3];
    unsigned q_next;
    unsigned q_length;
  } queue;

  struct {
    int16_t *data;  // signed 16-bit samples, native byte order
    unsigned index;
    unsigned size;
  } buffer;

  struct {
    bool synchronize;
    unsigned frequency;
    unsigned latency;
  } settings;

  bool cap(const string& name) {
    if(name == Audio::Synchronize) return true;
    if(name == Audio::Frequency) return true;
    if(name == Audio::Latency) return true;
    return false;
  }

  any get(const string& name) {
    if(name == Audio::Synchronize) return settings.synchronize;
    if(name == Audio::Frequency) return settings.frequency;
    if(name == Audio::Latency) return settings.latency;
    return false;
  }

  bool set(const string& name, const any& value) {
    if(name == Audio::Synchronize) {
      settings.synchronize = any_cast<bool>(value);
      return true;
    }

    if(name == Audio::Frequency) {
      settings.frequency = any_cast<unsigned>(value);
      return true;
    }

    if(name == Audio::Latency) {
      if(settings.latency != any_cast<unsigned>(value)) {
        settings.latency = any_cast<unsigned>(value);
        update_latency();
      }
      return true;
    }

    return false;
  }

  void sample(uint16_t sl, uint16_t sr) {
    buffer.data[buffer.index++] = sl;
    buffer.data[buffer.index++] = sr;
    if(buffer.index < buffer.size) return;

    ALuint albuffer = 0;
    ALint processed = 0;
    while(true) {
      alGetSourcei(device.source, AL_BUFFERS_PROCESSED, &processed);
      while(processed--) {
        alSourceUnqueueBuffers(device.source, 1, &albuffer);
        queue.q_length--;
      }

      // wait for buffer playback to catch up to sample generation
      if(settings.synchronize == false || queue.q_length < 3) break;
      usleep(1000); // yield the cpu
    }

    if(queue.q_length < 3) {
      albuffer = queue.q_buffers[queue.q_next];
      queue.q_next = (queue.q_next + 1) % 3;

      alBufferData(albuffer, device.format, buffer.data,
                   buffer.size * sizeof(buffer.data[0]), settings.frequency);
      alSourceQueueBuffers(device.source, 1, &albuffer);
      queue.q_length++;
    }

    ALint playing;
    alGetSourcei(device.source, AL_SOURCE_STATE, &playing);
    if(playing != AL_PLAYING) alSourcePlay(device.source);
    buffer.index = 0;
  }

  void clear() {
  }

  void update_latency() {
    if(buffer.data) delete[] buffer.data;

    // (2 samples per stereo frame) * (frames per second) * (seconds of latency)
    buffer.size = 2 * unsigned(settings.frequency * settings.latency / 1000.0 + 0.5);
    buffer.data = new int16_t[buffer.size];
    buffer.index = 0;
  }

  bool init() {
    update_latency();

    bool success = false;
    if(device.handle = alcOpenDevice(NULL)) {
      if(device.context = alcCreateContext(device.handle, NULL)) {
        alcMakeContextCurrent(device.context);

        alGenBuffers(3, queue.q_buffers);
        queue.q_next = 0;
        queue.q_length = 0;

        alGenSources(1, &device.source);

        //alSourcef (device.source, AL_PITCH, 1.0);
        //alSourcef (device.source, AL_GAIN, 1.0);
        //alSource3f(device.source, AL_POSITION, 0.0, 0.0, 0.0);
        //alSource3f(device.source, AL_VELOCITY, 0.0, 0.0, 0.0);
        //alSource3f(device.source, AL_DIRECTION, 0.0, 0.0, 0.0);
        //alSourcef (device.source, AL_ROLLOFF_FACTOR, 0.0);
        //alSourcei (device.source, AL_SOURCE_RELATIVE, AL_TRUE);

        alListener3f(AL_POSITION, 0.0, 0.0, 0.0);
        alListener3f(AL_VELOCITY, 0.0, 0.0, 0.0);
        ALfloat listener_orientation[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
        alListenerfv(AL_ORIENTATION, listener_orientation);

        success = true;
      }
    }

    if(success == false) {
      term();
      return false;
    }

    return true;
  }

  void term() {
    if(alIsSource(device.source) == AL_TRUE) {
      alDeleteSources(1, &device.source);
      device.source = 0;

      alDeleteBuffers(3, queue.q_buffers);
    }

    if(device.context) {
      alcMakeContextCurrent(NULL);
      alcDestroyContext(device.context);
      device.context = 0;
    }

    if(device.handle) {
      alcCloseDevice(device.handle);
      device.handle = 0;
    }

    if(buffer.data) {
      delete[] buffer.data;
      buffer.data = 0;
    }
  }

  pAudioOpenAL() {
    device.source = 0;
    device.handle = 0;
    device.context = 0;
    device.format = AL_FORMAT_STEREO16;

    queue.q_next = 0;
    queue.q_length = 0;

    buffer.data = 0;
    buffer.index = 0;
    buffer.size = 0;

    settings.synchronize = true;
    settings.frequency = 22050;
    settings.latency = 40;
  }

  ~pAudioOpenAL() {
    term();
  }
};

DeclareAudio(OpenAL)

};
