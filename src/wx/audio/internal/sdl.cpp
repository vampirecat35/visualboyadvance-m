#include "wx/audio/internal/sdl.h"

// === LOGALL writes very detailed informations to vba-trace.log ===
// #define LOGALL

// on win32 and mac, pointer typedefs only happen with AL_NO_PROTOTYPES
// on mac, ALC_NO_PROTOTYPES as well

// #define AL_NO_PROTOTYPES 1

// on mac, alc pointer typedefs ony happen for ALC if ALC_NO_PROTOTYPES
// unfortunately, there is a bug in the system headers (use of ALCvoid when
// void should be used; shame on Apple for introducing this error, and shame
// on Creative for making a typedef to void in the first place)
// #define ALC_NO_PROTOTYPES 1

#ifdef ENABLE_SDL3
#include <SDL3/SDL.h>
#else
#include <SDL.h>
#endif

#include <wx/arrstr.h>
#include <wx/log.h>
#include <wx/translation.h>
#include <wx/utils.h>

#include "core/base/ringbuffer.h"
#include "core/base/sound_driver.h"
#include "core/base/check.h"
#include "core/gba/gbaGlobals.h"
#include "core/gba/gbaSound.h"
#include "wx/config/option-proxy.h"

#ifndef LOGALL
// replace logging functions with comments
#ifdef winlog
#undef winlog
#endif
// https://stackoverflow.com/a/1306690/262458
#define winlog(x, ...) \
    do {               \
    } while (0)
#define debugState()  //
#endif

extern int emulating;

namespace audio {
namespace internal {

namespace {

class SDLAudio : public SoundDriver {
public:
    SDLAudio();
    ~SDLAudio() override;
    
    bool init(long sampleRate) override;                  // initialize the sound buffer queue
    void deinit();
    void setThrottle(unsigned short throttle_) override;  // set game speed
    void pause() override;                                // pause the secondary sound buffer
    void reset() override;   // stop and reset the secondary sound buffer
    void resume() override;  // play/resume the secondary sound buffer
    void read(uint16_t* stream, int length);
    void write(uint16_t* finalWave, int length) override;  // write the emulated sound to a sound buffer
    std::size_t buffer_size(); // The buffer size
    bool should_wait();
    
private:
#ifdef ENABLE_SDL3
    static void soundCallback(void* data, SDL_AudioStream *stream, int additional_length, int length);
#else
    static void soundCallback(void* data, uint8_t* stream, int len);
#endif
    
    RingBuffer<uint16_t> samples_buf;
    
    SDL_AudioDeviceID sound_device = 0;
    
#ifdef ENABLE_SDL3
    SDL_AudioStream *sound_stream = NULL;
    SDL_Mutex* mutex;
    SDL_Semaphore* data_available;
    SDL_Semaphore* data_read;
#else
    SDL_mutex* mutex;
    SDL_semaphore* data_available;
    SDL_semaphore* data_read;
#endif
    
    SDL_AudioSpec audio_spec;
    
    unsigned short current_rate;
    
    bool initialized = false;
    
    // Defines what delay in seconds we keep in the sound buffer
    static const double buftime;
};

// Hold up to 100 ms of data in the ring buffer
const double SDLAudio::buftime = 0.100;

SDLAudio::SDLAudio():
samples_buf(0),
sound_device(0),
current_rate(static_cast<unsigned short>(coreOptions.throttle)),
initialized(false)
{}

void SDLAudio::deinit() {
    if (!initialized)
        return;
    
    initialized = false;
    
    SDL_LockMutex(mutex);
    int is_emulating = emulating;
    emulating = 0;
#ifndef ENABLE_SDL3
    SDL_SemPost(data_available);
    SDL_SemPost(data_read);
#else
    SDL_SignalSemaphore(data_available);
    SDL_SignalSemaphore(data_read);
#endif
    SDL_UnlockMutex(mutex);
    
    SDL_Delay(100);
    
    SDL_DestroySemaphore(data_available);
    data_available = nullptr;
    SDL_DestroySemaphore(data_read);
    data_read      = nullptr;
    
    SDL_DestroyMutex(mutex);
    mutex = nullptr;
    
    SDL_CloseAudioDevice(sound_device);

    emulating = is_emulating;
}

SDLAudio::~SDLAudio() {
    deinit();
}

bool SDLAudio::should_wait() {
    return emulating && !coreOptions.speedup && current_rate && !gba_joybus_active;
}

bool SDLAudio::init(long sampleRate) {
    winlog("SDLAudio::init\n");
    if (initialized) deinit();
    
    SDL_AudioSpec audio;
    SDL_memset(&audio, 0, sizeof(audio));
    
    // for "no throttle" use regular rate, audio is just dropped
    audio.freq     = current_rate ? static_cast<int>(sampleRate * (current_rate / 100.0)) : sampleRate;
    
#ifndef ENABLE_SDL3
    audio.format   = AUDIO_S16SYS;
#else
    audio.format   = SDL_AUDIO_S16;
#endif
    
    audio.channels = 2;
    
#ifndef ENABLE_SDL3
    audio.samples  = 2048;
    audio.callback = soundCallback;
    audio.userdata = this;
    
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
#else
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) == false)
#endif
        {
            return false;
        }
    
#ifndef ENABLE_SDL3
    if (SDL_WasInit(SDL_INIT_AUDIO) < 0)
#else
        if (SDL_WasInit(SDL_INIT_AUDIO) == false)
#endif
        {
            SDL_Init(SDL_INIT_AUDIO);
        }
    
#ifndef ENABLE_SDL3
    sound_device = SDL_OpenAudioDevice(NULL, 0, &audio, NULL, 0);
#else
    sound_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &audio, soundCallback, this);
    
    if(sound_stream == NULL)
    {
        return false;
    }
    
    sound_device = SDL_GetAudioStreamDevice(sound_stream);
#endif
    
    if(sound_device == 0) {
        return false;
    }
    
    samples_buf.reset(static_cast<size_t>(std::ceil(buftime * sampleRate * 2)));
    
    mutex          = SDL_CreateMutex();
    data_available = SDL_CreateSemaphore(0);
    data_read      = SDL_CreateSemaphore(1);
    
    // turn off audio events because we are not processing them
#if SDL_VERSION_ATLEAST(3, 2, 0)
    SDL_SetEventEnabled(SDL_EVENT_AUDIO_DEVICE_ADDED, false);
    SDL_SetEventEnabled(SDL_EVENT_AUDIO_DEVICE_REMOVED, false);
#elif SDL_VERSION_ATLEAST(2, 0, 4)
    SDL_EventState(SDL_AUDIODEVICEADDED,   SDL_IGNORE);
    SDL_EventState(SDL_AUDIODEVICEREMOVED, SDL_IGNORE);
#endif
    
    return initialized = true;
}

void SDLAudio::setThrottle(unsigned short throttle_) {
    if (!initialized)
        return;
    
    if (!throttle_)
        throttle_ = 100;
}

void SDLAudio::resume() {
    if (!initialized)
        return;
    
    winlog("SDLAudio::resume\n");
    
#ifndef ENABLE_SDL3
    if (SDL_GetAudioDeviceStatus(sound_device) != SDL_AUDIO_PLAYING)
        SDL_PauseAudioDevice(sound_device, 0);
#else
    if (SDL_AudioDevicePaused(sound_device) == true)
    {
        SDL_ResumeAudioStreamDevice(sound_stream);
        SDL_ResumeAudioDevice(sound_device);
    }
#endif
}

void SDLAudio::pause() {
    if (!initialized)
        return;
    
    winlog("SDLAudio::pause\n");
    
#ifndef ENABLE_SDL3
    if (SDL_GetAudioDeviceStatus(sound_device) != SDL_AUDIO_PLAYING)
        SDL_PauseAudioDevice(sound_device, 1);
#else
    if (SDL_AudioDevicePaused(sound_device) == true)
    {
        SDL_PauseAudioStreamDevice(sound_stream);
        SDL_PauseAudioDevice(sound_device);
    }
#endif
}

void SDLAudio::reset() {
    if (!initialized)
        return;
    
    winlog("SDLAudio::reset\n");
    
    init(soundGetSampleRate());
}

std::size_t SDLAudio::buffer_size() {
    SDL_LockMutex(mutex);
    std::size_t size = samples_buf.used();
    SDL_UnlockMutex(mutex);
    
    return size;
}

void SDLAudio::read(uint16_t* stream, int length) {
    if (length <= 0)
        return;
    
    SDL_memset(stream, 0, length);
    
    // if not initialzed, paused or shutting down, do nothing
    if (!initialized || !emulating)
        return;
    
    if (!buffer_size()) {
        if (should_wait())
#ifndef ENABLE_SDL3
            SDL_SemWait(data_available);
#else
        SDL_WaitSemaphore(data_available);
#endif
        else
            return;
    }
    
    SDL_LockMutex(mutex);
    
    samples_buf.read(stream, std::min((std::size_t)(length / 2), samples_buf.used()));
    
    SDL_UnlockMutex(mutex);
    
#ifndef ENABLE_SDL3
    SDL_SemPost(data_read);
#else
    SDL_SignalSemaphore(data_read);
#endif
}

#ifndef ENABLE_SDL3
void SDLAudio::soundCallback(void* data, uint8_t* stream, int len) {
    reinterpret_cast<SDLAudio*>(data)->read(reinterpret_cast<uint16_t*>(stream), len);
}
#else
void SDLAudio::soundCallback(void* data, SDL_AudioStream *stream, int additional_length, int length) {
    uint16_t streamdata[8192];
    (void)additional_length;

    while (length > 0)
    {
        reinterpret_cast<SDLAudio*>(data)->read(reinterpret_cast<uint16_t*>(streamdata), length > sizeof(streamdata) ? sizeof(streamdata) : length);
        SDL_PutAudioStreamData(stream, streamdata, length > sizeof(streamdata) ? sizeof(streamdata) : length);
        length -= sizeof(streamdata);
    }
}
#endif

void SDLAudio::write(uint16_t* finalWave, int length) {
    if (!initialized)
        return;
    
    SDL_LockMutex(mutex);
    
#ifndef ENABLE_SDL3
    if (SDL_GetAudioDeviceStatus(sound_device) != SDL_AUDIO_PLAYING)
        SDL_PauseAudioDevice(sound_device, 0);
#else
    if (SDL_AudioDevicePaused(sound_device) == true)
    {
        SDL_ResumeAudioStreamDevice(sound_stream);
        SDL_ResumeAudioDevice(sound_device);
    }
#endif
    
    std::size_t samples = length / 4;
    std::size_t avail;
    
    while ((avail = samples_buf.avail() / 2) < samples) {
        samples_buf.write(finalWave, avail * 2);
        
        finalWave += avail * 2;
        samples -= avail;
        
        SDL_UnlockMutex(mutex);
        
#ifndef ENABLE_SDL3
        SDL_SemPost(data_available);
#else
        SDL_SignalSemaphore(data_available);
#endif
        
        if (should_wait())
#ifndef ENABLE_SDL3
            SDL_SemWait(data_read);
#else
        SDL_WaitSemaphore(data_read);
#endif
        else
            // Drop the remainder of the audio data
            return;
        
        SDL_LockMutex(mutex);
    }
    
    samples_buf.write(finalWave, samples * 2);
    
    SDL_UnlockMutex(mutex);
}

}  // namespace

std::vector<AudioDevice> GetSDLDevices() {
    std::vector<AudioDevice> devices;
    
    devices.push_back({_("Default device"), wxEmptyString});
    
    return devices;
}

std::unique_ptr<SoundDriver> CreateSDLDriver() {
    winlog("newSDL\n");
    return std::make_unique<SDLAudio>();
}

}  // namespace internal
}  // namespace audio
