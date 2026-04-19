//=====================================================================
//
// GameSound.h - A single-header multi-channel audio playback library
//
// Features:
//   - Multi-channel WAV audio playback with software mixing
//   - Per-channel volume control
//   - WAV file caching with reference counting
//   - Low latency audio output
//   - Windows waveOut API backend (default on Windows)
//   - SDL2 audio backend (default on Linux/macOS, optional on Windows)
//
// How to use (single file project, most common):
//
//     #define GAMESOUND_IMPLEMENTATION
//     #include "GameSound.h"
//
//     int main() {
//         GameSound sound;
//
//         // Play explosion sound (once, 80% volume)
//         int explosion = sound.PlayWAV("assets/explosion.wav", 1, 800);
//
//         // Play background music (infinite loop, 50% volume)
//         int bgm = sound.PlayWAV("assets/bgm.wav", 0, 500);
//
//         // Check if explosion is still playing
//         if (sound.IsPlaying(explosion)) {
//             // ...
//         }
//
//         // Adjust BGM volume dynamically
//         sound.SetVolume(bgm, 300);
//
//         // Stop BGM
//         sound.StopWAV(bgm);
//
//         // Stop all sounds when game ends
//         sound.StopAll();
//         // All cached WAV data is freed automatically on destruction
//
//         return 0;
//     }
//
// Multi-file project: add this line before #include in the main .cpp file
//     #define GAMESOUND_IMPLEMENTATION
//     #include "GameSound.h"
// In other .cpp files, add this line
//     #define GAMESOUND_NO_IMPLEMENTATION
//     #include "GameSound.h"
//
// Compile command (Windows, waveOut backend):
//     g++ -o game.exe main.cpp -mwindows -lwinmm
//
// Compile with SDL2 backend (Windows):
//     g++ -o game.exe main.cpp -I<SDL2_path>/include -L<SDL2_path>/lib -lSDL2 -lwinmm -DGAMESOUND_USE_SDL=1
//
// Compile command (Linux/macOS, SDL2 backend is default):
//     g++ -o game main.cpp -lSDL2
//
// Last Modified: 2026/04/19
//
//=====================================================================
#ifndef GAMESOUND_H
#define GAMESOUND_H

// Default behavior: include enables implementation (good for single file projects)
#ifndef GAMESOUND_NO_IMPLEMENTATION
#ifndef GAMESOUND_IMPLEMENTATION
#define GAMESOUND_IMPLEMENTATION
#endif
#endif

// Platform detection
#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
    #ifndef GAMESOUND_USE_SDL
        #define GAMESOUND_USE_SDL 0
    #endif
#else
    #ifndef GAMESOUND_USE_SDL
        #define GAMESOUND_USE_SDL 1
    #endif
#endif

#if GAMESOUND_USE_SDL
    #define SDL_MAIN_HANDLED
    #ifndef SDL_h_
        // Try common SDL2 header paths (Linux/macOS may use either)
        #if defined(__has_include)
            #if __has_include(<SDL2/SDL.h>)
                #include <SDL2/SDL.h>
            #elif __has_include(<SDL.h>)
                #include <SDL.h>
            #else
                #error "GameSound.h: Cannot find SDL2 header. Install SDL2 or provide include path."
            #endif
        #else
            #include <SDL2/SDL.h>
        #endif
    #endif
#endif

#include <string>
#include <unordered_map>
#include <vector>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//---------------------------------------------------------------------
// Link libraries (MSVC only, on Windows only)
//---------------------------------------------------------------------
#if defined(_MSC_VER) && defined(_WIN32) && !GAMESOUND_USE_SDL
#pragma comment(lib, "winmm.lib")
#endif

//---------------------------------------------------------------------
// GameSound Class
//---------------------------------------------------------------------
class GameSound {
public:
    GameSound();
    ~GameSound();

    // Play a WAV file, returns channel ID
    // Parameters:
    //   filename - path to WAV file
    //   repeat   - 0=infinite loop, 1=play once (default), >1=play N times
    //   volume   - volume level (0-1000, default=1000)
    // Returns:
    //   >0  - channel ID (success)
    //   -1  - file not found or load failed
    //   -2  - audio device not initialized
    //   -3  - memory allocation failed
    //   -4  - channel limit reached (max 32 simultaneous sounds)
    int PlayWAV(const char* filename, int repeat = 1, int volume = 1000);

    // Stop a specific channel
    // Returns: 0 on success, -1 if channel not found
    int StopWAV(int channel);

    // Check if a channel is currently playing
    // Returns: 1 if playing, 0 if not playing, -1 if channel not found
    int IsPlaying(int channel);

    // Set volume for a channel (0-1000)
    // Returns: 0 on success, -1 if channel not found
    int SetVolume(int channel, int volume);

    // Stop all channels
    void StopAll();

    // Set master volume (affects all channels, 0-1000)
    // Returns: 0 on success
    int SetMasterVolume(int volume);

    // Get master volume (0-1000)
    int GetMasterVolume() const;

#ifdef GAMESOUND_IMPLEMENTATION
    // Check if audio backend is initialized
    bool IsInitialized() const { return initialized_; }
#endif

#ifdef GAMESOUND_IMPLEMENTATION
private:
    // WAV data structure
    struct WavData {
        uint8_t* buffer;
        uint32_t size;
        uint32_t sample_rate;
        uint16_t channels;
        uint16_t bits_per_sample;
        int ref_count;

        WavData() : buffer(NULL), size(0), sample_rate(0), channels(0), 
                    bits_per_sample(0), ref_count(0) {}
        ~WavData() { if (buffer) delete[] buffer; }
    };

    // Channel state
    struct Channel {
        int id;
        WavData* wav;
        uint32_t position;
        int repeat;
        int volume;
        bool is_playing;

        Channel() : id(0), wav(NULL), position(0), repeat(1), 
                    volume(1000), is_playing(false) {}
    };

    // Member variables
    std::unordered_map<std::string, WavData*> wav_cache_;
    std::unordered_map<int, Channel*> channels_;
    int64_t next_channel_id_;
    bool initialized_;
    int master_volume_;  // Global volume multiplier (0-1000)

#if GAMESOUND_USE_SDL
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
    bool sdl_audio_init_by_self_;  // true if we initialized SDL audio ourselves
#else
    HWAVEOUT h_wave_out_;
    WAVEFORMATEX wfx_;
    WAVEHDR* wave_hdr_[2];  // Double buffering
    int current_hdr_;       // Which buffer is currently being filled
    CRITICAL_SECTION lock_;
    volatile bool closing_; // Flag to signal callback to stop
#endif

    // Maximum number of simultaneous channels
    static const int MAX_CHANNELS = 32;

    // Buffer size configuration (can be overridden before #include)
    // Note: BUFFER_SAMPLES is the number of audio frames (time points),
    // NOT the number of int16_t values.
    // For 44100Hz/stereo/16bit output: each frame = 2 int16_t = 4 bytes
    //  512  = ~12ms  latency (may stutter with waveOut callback mode)
    // 1024  = ~23ms  latency (may stutter with waveOut callback mode)
    // 2048  = ~46ms  latency (stable, default)
    // 4096  = ~93ms  latency (very stable, higher latency)
#ifndef GAMESOUND_BUFFER_SAMPLES
    #define GAMESOUND_BUFFER_SAMPLES 2048
#endif

    static const int BUFFER_FRAMES = GAMESOUND_BUFFER_SAMPLES;  // frames (time points)
    static const int OUTPUT_CHANNELS = 2;                        // stereo output
    static const int BUFFER_TOTAL_SAMPLES = BUFFER_FRAMES * OUTPUT_CHANNELS;  // total int16_t values
    static const int BUFFER_BYTES = BUFFER_TOTAL_SAMPLES * sizeof(int16_t);   // bytes per buffer
    int32_t mix_buffer_[BUFFER_TOTAL_SAMPLES];  // int32_t to prevent overflow during mixing

    // Internal methods
    int16_t ReadSample(Channel* ch);
    void ClampAndConvert(int32_t* input, int16_t* output, int count);
    WavData* LoadWAVFromFile(const char* filename);
    WavData* ConvertToTargetFormat(WavData* src);
    WavData* LoadOrCacheWAV(const char* filename);
    int AllocateChannel();
    void ReleaseChannel(int channel_id);
    void MixAudio(int16_t* output_buffer, int sample_count);

#if GAMESOUND_USE_SDL
    static void SDLCALL SDLAudioCallback(void* userdata, Uint8* stream, int len);
#else
    static void CALLBACK WaveOutCallback(HWAVEOUT hwo, UINT uMsg, 
                                          DWORD_PTR dwInstance, 
                                          DWORD_PTR dwParam1, DWORD_PTR dwParam2);
#endif

    bool InitAudioBackend();
    void ShutdownAudioBackend();
#endif
};

#ifdef GAMESOUND_IMPLEMENTATION
//=====================================================================
// Implementation
//=====================================================================

#ifndef GAMESOUND_DEBUG
#define GAMESOUND_DEBUG 0
#endif

#define GS_DEBUG_PRINT(fmt, ...) do { \
    if (GAMESOUND_DEBUG) printf("[GameSound] " fmt "\n", ##__VA_ARGS__); \
} while(0)

//---------------------------------------------------------------------
// Constructor & Destructor
//---------------------------------------------------------------------
inline GameSound::GameSound() 
    : next_channel_id_(1), initialized_(false), master_volume_(1000)
#if GAMESOUND_USE_SDL
    , audio_device_(0), sdl_audio_init_by_self_(false)
#else
    , h_wave_out_(NULL), current_hdr_(0), closing_(false)
#endif
{
    GS_DEBUG_PRINT("Constructor called");
#if !GAMESOUND_USE_SDL
    InitializeCriticalSection(&lock_);
#endif
    initialized_ = InitAudioBackend();
    GS_DEBUG_PRINT("InitAudioBackend returned: %d", initialized_);
}

inline GameSound::~GameSound() {
    GS_DEBUG_PRINT("Destructor started");

    // First shutdown audio backend (stops callbacks, prevents deadlock)
    GS_DEBUG_PRINT("  Calling ShutdownAudioBackend...");
    ShutdownAudioBackend();
    GS_DEBUG_PRINT("  ShutdownAudioBackend done");

    // Then clean up channels and cache (no lock needed since callbacks are stopped)
    GS_DEBUG_PRINT("  Cleaning %u channels...", (unsigned)channels_.size());
    for (std::unordered_map<int, Channel*>::iterator it = channels_.begin();
         it != channels_.end(); ++it) {
        delete it->second;
    }
    channels_.clear();
    GS_DEBUG_PRINT("  Channels cleaned");

    // Free all cached WAV data
    GS_DEBUG_PRINT("  Cleaning %u cached WAVs...", (unsigned)wav_cache_.size());
    for (std::unordered_map<std::string, WavData*>::iterator it = wav_cache_.begin();
         it != wav_cache_.end(); ++it) {
        delete it->second;
    }
    wav_cache_.clear();
    GS_DEBUG_PRINT("  WAV cache cleaned");

#if !GAMESOUND_USE_SDL
    GS_DEBUG_PRINT("  Deleting critical section...");
    DeleteCriticalSection(&lock_);
    GS_DEBUG_PRINT("Destructor finished");
#endif
}

//---------------------------------------------------------------------
// Read PCM sample from channel
//---------------------------------------------------------------------
inline int16_t GameSound::ReadSample(Channel* ch) {
    if (!ch || !ch->wav || !ch->wav->buffer) return 0;

    uint32_t pos = ch->position;
    if (pos >= ch->wav->size) return 0;

    if (ch->wav->bits_per_sample == 16) {
        // Fixed: cast to uint8_t to prevent sign extension
        int16_t sample = (int16_t)((uint16_t)(uint8_t)ch->wav->buffer[pos] | 
                                    ((uint16_t)(uint8_t)ch->wav->buffer[pos + 1] << 8));
        return sample;
    } else if (ch->wav->bits_per_sample == 8) {
        uint8_t sample8 = ch->wav->buffer[pos];
        return (int16_t)((sample8 - 128) << 8); // Convert to 16-bit
    }
    return 0;
}

//---------------------------------------------------------------------
// Clamp and convert 32-bit to 16-bit with overflow protection
//---------------------------------------------------------------------
inline void GameSound::ClampAndConvert(int32_t* input, int16_t* output, int count) {
    for (int i = 0; i < count; i++) {
        int32_t sample = input[i];
        if (sample > 32767) sample = 32767;
        else if (sample < -32768) sample = -32768;
        output[i] = (int16_t)sample;
    }
}

//---------------------------------------------------------------------
// Load WAV file from disk
//---------------------------------------------------------------------
inline GameSound::WavData* GameSound::LoadWAVFromFile(const char* filename) {
#if _WIN32
    FILE* f = NULL;
    if (fopen_s(&f, filename, "rb") != 0 || f == NULL) {
        return NULL;
    }
#else
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
#endif

    // Read WAV header (44 bytes)
    char header[44];
    if (fread(header, 1, 44, f) != 44) {
        fclose(f);
        return NULL;
    }

    // Validate RIFF header
    if (header[0] != 'R' || header[1] != 'I' || header[2] != 'F' || header[3] != 'F') {
        fclose(f);
        return NULL;
    }

    // Validate WAVE format
    if (header[8] != 'W' || header[9] != 'A' || header[10] != 'V' || header[11] != 'E') {
        fclose(f);
        return NULL;
    }

    // Parse format chunk
    uint16_t audio_format = (uint16_t)(header[20] | (header[21] << 8));
    if (audio_format != 1) { // Only support PCM
        fclose(f);
        return NULL;
    }

    WavData* wav = new WavData();
    wav->channels = (uint16_t)((uint8_t)header[22] | ((uint8_t)header[23] << 8));
    wav->sample_rate = (uint32_t)((uint8_t)header[24] | ((uint8_t)header[25] << 8) |
                                   ((uint8_t)header[26] << 16) | ((uint8_t)header[27] << 24));
    wav->bits_per_sample = (uint16_t)((uint8_t)header[34] | ((uint8_t)header[35] << 8));

    // Validate format parameters
    if (wav->channels == 0 || wav->channels > 2) {
        delete wav; fclose(f); return NULL;
    }
    if (wav->sample_rate == 0) {
        delete wav; fclose(f); return NULL;
    }
    if (wav->bits_per_sample != 8 && wav->bits_per_sample != 16) {
        delete wav; fclose(f); return NULL;
    }

    // Find data chunk
    fseek(f, 12, SEEK_SET);
    bool found_data = false;
    while (!found_data) {
        char chunk_id[4];
        uint32_t chunk_size = 0;
        if (fread(chunk_id, 1, 4, f) != 4) break;
        if (fread(&chunk_size, 4, 1, f) != 1) break;

        if (chunk_id[0] == 'd' && chunk_id[1] == 'a' &&
            chunk_id[2] == 't' && chunk_id[3] == 'a') {
            found_data = true;
            wav->size = chunk_size;
        } else {
            // WAV spec: chunks are padded to even size
            uint32_t skip = chunk_size + (chunk_size % 2);
            fseek(f, skip, SEEK_CUR);
        }
    }

    if (!found_data || wav->size == 0 || wav->size > 100 * 1024 * 1024) {
        delete wav;
        fclose(f);
        return NULL;
    }

    // Read PCM data
    wav->buffer = new uint8_t[wav->size];
    if (fread(wav->buffer, 1, wav->size, f) != wav->size) {
        delete wav;
        fclose(f);
        return NULL;
    }

    fclose(f);

    GS_DEBUG_PRINT("Loaded WAV: %s", filename);
    GS_DEBUG_PRINT("  sample_rate=%u, channels=%u, bits=%u, size=%u", 
                   wav->sample_rate, wav->channels, wav->bits_per_sample, wav->size);

    // Convert to target format (44100Hz, stereo, 16-bit)
    WavData* converted = ConvertToTargetFormat(wav);
    if (converted) {
        GS_DEBUG_PRINT("  Converted: rate=%u, ch=%u, size=%u",
                       converted->sample_rate, converted->channels, converted->size);
    } else {
        GS_DEBUG_PRINT("  Conversion FAILED!");
    }
    delete wav; // Free original format data

    return converted;
}

//---------------------------------------------------------------------
// Convert WAV to target format (44100Hz, stereo, 16-bit)
//---------------------------------------------------------------------
inline GameSound::WavData* GameSound::ConvertToTargetFormat(WavData* src) {
    if (!src || !src->buffer || src->size == 0 ||
        src->sample_rate == 0 || src->channels == 0) {
        return NULL;
    }

    const uint32_t target_rate = 44100;
    const uint16_t target_channels = 2;
    const uint16_t target_bps = 16;

    // Decode original data to 16-bit samples
    uint32_t bytes_per_sample = src->bits_per_sample / 8;
    uint32_t total_samples = src->size / bytes_per_sample;
    uint32_t samples_per_channel = total_samples / src->channels;

    // Step 1: Decode to 16-bit array
    int16_t* decoded = new int16_t[total_samples];
    for (uint32_t i = 0; i < total_samples; i++) {
        if (src->bits_per_sample == 16) {
            // Fixed: cast to uint8_t to prevent sign extension
            decoded[i] = (int16_t)((uint16_t)(uint8_t)src->buffer[i * 2] | 
                                    ((uint16_t)(uint8_t)src->buffer[i * 2 + 1] << 8));
        } else if (src->bits_per_sample == 8) {
            decoded[i] = (int16_t)((src->buffer[i] - 128) << 8);
        }
    }

    // Step 2: Resample (linear interpolation)
    double ratio = (double)target_rate / src->sample_rate;
    double step = (double)src->sample_rate / target_rate;
    uint32_t new_samples_per_ch = (uint32_t)(samples_per_channel * ratio);
    uint32_t new_total_samples = new_samples_per_ch * src->channels;

    int16_t* resampled = new int16_t[new_total_samples];
    for (uint16_t ch = 0; ch < src->channels; ch++) {
        double src_index = 0;
        for (uint32_t i = 0; i < new_samples_per_ch; i++) {
            uint32_t idx = (uint32_t)src_index;
            if (idx >= samples_per_channel) idx = samples_per_channel - 1;
            double frac = src_index - idx;

            int16_t s0 = decoded[idx * src->channels + ch];
            int16_t s1 = (idx + 1 < samples_per_channel) ?
                         decoded[(idx + 1) * src->channels + ch] : s0;

            resampled[i * src->channels + ch] = (int16_t)(s0 * (1.0 - frac) + s1 * frac);
            src_index += step;
        }
    }
    delete[] decoded;

    // Step 3: Convert mono to stereo
    int16_t* stereo = NULL;
    uint32_t stereo_samples = 0;

    if (src->channels == 1) {
        stereo_samples = new_samples_per_ch * 2; // L+R
        stereo = new int16_t[stereo_samples];
        for (uint32_t i = 0; i < new_samples_per_ch; i++) {
            stereo[i * 2] = resampled[i];     // Left
            stereo[i * 2 + 1] = resampled[i]; // Right
        }
        delete[] resampled;
    } else {
        stereo = resampled;
        stereo_samples = new_total_samples;
    }

    // Step 4: Create new WavData
    WavData* dst = new WavData();
    dst->sample_rate = target_rate;
    dst->channels = target_channels;
    dst->bits_per_sample = target_bps;
    dst->size = stereo_samples * sizeof(int16_t);
    dst->buffer = new uint8_t[dst->size];
    memcpy(dst->buffer, stereo, dst->size);

    delete[] stereo;
    return dst;
}

//---------------------------------------------------------------------
// Load or get cached WAV
//---------------------------------------------------------------------
inline GameSound::WavData* GameSound::LoadOrCacheWAV(const char* filename) {
    std::string key(filename);
    std::unordered_map<std::string, WavData*>::iterator it = wav_cache_.find(key);
    if (it != wav_cache_.end()) {
        it->second->ref_count++;
        return it->second;
    }

    WavData* wav = LoadWAVFromFile(filename);
    if (wav) {
        wav->ref_count = 1;
        wav_cache_[key] = wav;
    }
    return wav;
}

//---------------------------------------------------------------------
// Allocate unique channel ID
//---------------------------------------------------------------------
inline int GameSound::AllocateChannel() {
    // Check channel limit
    if ((int)channels_.size() >= MAX_CHANNELS) {
        return 0;  // No more channels available
    }

    // Wrap around if exceeds 32700 to prevent int overflow
    if (next_channel_id_ > 32700) {
        next_channel_id_ = 1;
    }

    // Find unused channel ID
    while (channels_.count((int)next_channel_id_)) {
        next_channel_id_++;
        // Safety check: if all IDs are used, wrap again
        if (next_channel_id_ > 32700) {
            next_channel_id_ = 1;
        }
    }

    int id = (int)next_channel_id_;
    next_channel_id_++;
    return id;
}

//---------------------------------------------------------------------
// Release channel and decrement WAV ref count
//---------------------------------------------------------------------
inline void GameSound::ReleaseChannel(int channel_id) {
    std::unordered_map<int, Channel*>::iterator it = channels_.find(channel_id);
    if (it != channels_.end()) {
        if (it->second->wav) {
            it->second->wav->ref_count--;
        }
        delete it->second;
        channels_.erase(it);
    }
}

//---------------------------------------------------------------------
// Software mixer
//---------------------------------------------------------------------
inline void GameSound::MixAudio(int16_t* output_buffer, int sample_count) {
    // Clear output buffer (sample_count is total samples for all channels)
    for (int i = 0; i < sample_count; i++) {
        mix_buffer_[i] = 0;
    }

    // Note: no lock here - called from audio callback which already holds the lock
    // Public APIs (PlayWAV/StopWAV/StopAll) lock before modifying channels_

#if !GAMESOUND_USE_SDL
    // waveOut callback runs in a separate thread, we need explicit locking
    EnterCriticalSection(&lock_);
#endif

    // Mix all active channels
    std::unordered_map<int, Channel*>::iterator it = channels_.begin();
    while (it != channels_.end()) {
        Channel* ch = it->second;
        if (!ch->is_playing) {
            ++it;
            continue;
        }

        float vol = (ch->volume / 1000.0f) * (master_volume_ / 1000.0f);
        
        // bytes_per_frame = channels * bytes_per_sample
        // For stereo 16-bit: 2 * 2 = 4 bytes per frame
        int bytes_per_sample = ch->wav->bits_per_sample / 8;
        int bytes_per_frame = bytes_per_sample * ch->wav->channels;
        
        // sample_count is total output samples (stereo = L+R pairs)
        // We need to mix frames (one frame = all channels for one time point)
        int frames_to_mix = sample_count / ch->wav->channels;
        
        // Reduce mix count if channel data is insufficient
        uint32_t remaining_bytes = ch->wav->size - ch->position;
        uint32_t remaining_frames = remaining_bytes / bytes_per_frame;
        
        GS_DEBUG_PRINT("MixAudio: ch=%d, position=%u, size=%u, remaining_frames=%u, frames_to_mix=%d",
                       ch->id, ch->position, ch->wav->size, remaining_frames, frames_to_mix);
        
        // If less than one frame remains, skip to end handling
        if (remaining_frames == 0) {
            frames_to_mix = 0;
            GS_DEBUG_PRINT("MixAudio: ch=%d, no remaining frames, setting frames_to_mix=0", ch->id);
        } else if ((uint32_t)frames_to_mix > remaining_frames) {
            frames_to_mix = (int)remaining_frames;
            GS_DEBUG_PRINT("MixAudio: ch=%d, limiting frames_to_mix to %d", ch->id, frames_to_mix);
        }

        // Mix samples - one frame at a time
        for (int frame = 0; frame < frames_to_mix; frame++) {
            // For stereo interleaved data: LRLRLR...
            // Each frame in the buffer is: [L0, R0], [L1, R1], ...
            // For channel ch_idx, the sample is at:
            //   position + frame * bytes_per_frame + ch_idx * bytes_per_sample
            uint32_t frame_start = ch->position + frame * bytes_per_frame;
            for (uint16_t ch_idx = 0; ch_idx < ch->wav->channels; ch_idx++) {
                uint32_t sample_pos = frame_start + ch_idx * bytes_per_sample;
                int16_t sample = 0;
                if (sample_pos + 1 < ch->wav->size) {
                    // Read 16-bit sample directly from interleaved data
                    sample = (int16_t)((uint16_t)(uint8_t)ch->wav->buffer[sample_pos] |
                                       ((uint16_t)(uint8_t)ch->wav->buffer[sample_pos + 1] << 8));
                }

                // Calculate output index based on channel
                int out_idx = frame * ch->wav->channels + ch_idx;

                // Apply volume and accumulate
                int32_t adjusted_sample = (int32_t)(sample * vol);
                mix_buffer_[out_idx] += adjusted_sample;
            }
        }
        // Advance position by all mixed frames
        ch->position += frames_to_mix * bytes_per_frame;
        
        // Handle loop/end logic
        if (ch->position >= ch->wav->size) {
            if (ch->repeat == 0) {
                // Infinite loop
                ch->position = 0;
            } else if (ch->repeat > 1) {
                // Finite loop
                ch->position = 0;
                ch->repeat--;
            } else {
                // Playback finished — inline release, avoid double lookup
                if (ch->wav) {
                    ch->wav->ref_count--;
                }
                delete ch;
                it = channels_.erase(it);
                continue;
            }
        }

        ++it;
    }

#if !GAMESOUND_USE_SDL
    LeaveCriticalSection(&lock_);
#endif

    // Limiter processing (prevent overflow)
    ClampAndConvert(mix_buffer_, output_buffer, sample_count);
}

//---------------------------------------------------------------------
// Audio callback (waveOut)
//---------------------------------------------------------------------
#if !GAMESOUND_USE_SDL
inline void CALLBACK GameSound::WaveOutCallback(HWAVEOUT hwo, UINT uMsg,
                                                 DWORD_PTR dwInstance,
                                                 DWORD_PTR dwParam1,
                                                 DWORD_PTR dwParam2) {
    if (uMsg != WOM_DONE) return;

    GameSound* sound = (GameSound*)dwInstance;

    if (sound->closing_) return;

    WAVEHDR* hdr = (WAVEHDR*)dwParam1;

    int total_samples = BUFFER_TOTAL_SAMPLES;
    int16_t output_buffer[BUFFER_TOTAL_SAMPLES];
    sound->MixAudio(output_buffer, total_samples);

    if (sound->closing_) return;

    memcpy(hdr->lpData, output_buffer, BUFFER_BYTES);
    hdr->dwBufferLength = BUFFER_BYTES;
    hdr->dwFlags = 0;
    waveOutPrepareHeader(hwo, hdr, sizeof(WAVEHDR));
    waveOutWrite(hwo, hdr, sizeof(WAVEHDR));
}
#endif

//---------------------------------------------------------------------
// Audio callback (SDL2)
//---------------------------------------------------------------------
#if GAMESOUND_USE_SDL
inline void SDLCALL GameSound::SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    GameSound* sound = (GameSound*)userdata;
    int total_samples = len / sizeof(int16_t);
    // SDL may request more samples than mix_buffer_ can hold, so mix in chunks
    int chunk_samples = BUFFER_TOTAL_SAMPLES;
    int16_t* out = (int16_t*)stream;
    int offset = 0;
    while (offset < total_samples) {
        int to_mix = chunk_samples;
        if (offset + to_mix > total_samples) {
            to_mix = total_samples - offset;
        }
        sound->MixAudio(out + offset, to_mix);
        offset += to_mix;
    }
}
#endif

//---------------------------------------------------------------------
// Initialize audio backend
//---------------------------------------------------------------------
inline bool GameSound::InitAudioBackend() {
#if GAMESOUND_USE_SDL
    GS_DEBUG_PRINT("InitAudioBackend: SDL2");

    SDL_SetMainReady();

    // Initialize SDL audio subsystem if not already done
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
            GS_DEBUG_PRINT("  SDL_InitSubSystem(AUDIO) failed: %s", SDL_GetError());
            return false;
        }
        sdl_audio_init_by_self_ = true;
        GS_DEBUG_PRINT("  SDL audio subsystem initialized (by GameSound)");
    } else {
        GS_DEBUG_PRINT("  SDL audio subsystem already initialized");
    }

    SDL_AudioSpec desired;
    memset(&desired, 0, sizeof(desired));
    desired.freq = 44100;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = BUFFER_FRAMES;
    desired.callback = SDLAudioCallback;
    desired.userdata = this;

    audio_device_ = SDL_OpenAudioDevice(NULL, 0, &desired, &audio_spec_, 0);
    if (audio_device_ == 0) {
        GS_DEBUG_PRINT("  SDL_OpenAudioDevice failed: %s", SDL_GetError());
        return false;
    }

    GS_DEBUG_PRINT("  SDL audio device opened, freq=%d, channels=%d, samples=%d, format=%d",
                    audio_spec_.freq, audio_spec_.channels, audio_spec_.samples, audio_spec_.format);
    SDL_PauseAudioDevice(audio_device_, 0); // Start playback
    return true;
#else
    GS_DEBUG_PRINT("InitAudioBackend: waveOut");
    // Setup audio format
    wfx_.wFormatTag = WAVE_FORMAT_PCM;
    wfx_.nSamplesPerSec = 44100;
    wfx_.wBitsPerSample = 16;
    wfx_.nChannels = 2;
    wfx_.nBlockAlign = (wfx_.wBitsPerSample / 8) * wfx_.nChannels;
    wfx_.nAvgBytesPerSec = wfx_.nSamplesPerSec * wfx_.nBlockAlign;
    wfx_.cbSize = 0;

    // Open audio device
    MMRESULT result = waveOutOpen(&h_wave_out_, WAVE_MAPPER, &wfx_, 
                                   (DWORD_PTR)WaveOutCallback, 
                                   (DWORD_PTR)this, CALLBACK_FUNCTION);
    if (result != MMSYSERR_NOERROR) {
        GS_DEBUG_PRINT("  waveOutOpen failed: error=%u", result);
        h_wave_out_ = NULL;
        return false;
    }

    GS_DEBUG_PRINT("  waveOut device opened");

    // Prepare and submit initial buffers (double buffering)
    for (int i = 0; i < 2; i++) {
        wave_hdr_[i] = new WAVEHDR();
        memset(wave_hdr_[i], 0, sizeof(WAVEHDR));
        wave_hdr_[i]->lpData = (LPSTR)new char[BUFFER_BYTES];
        wave_hdr_[i]->dwBufferLength = BUFFER_BYTES;
        memset(wave_hdr_[i]->lpData, 0, BUFFER_BYTES);
        
        MMRESULT prep = waveOutPrepareHeader(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
        GS_DEBUG_PRINT("  waveOut: Prepared buffer %d, result=%u", i, prep);
        
        MMRESULT wr = waveOutWrite(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
        GS_DEBUG_PRINT("  waveOut: Submitted buffer %d, result=%u", i, wr);
    }

    GS_DEBUG_PRINT("  Initial buffers submitted");
    return true;
#endif
}

//---------------------------------------------------------------------
// Shutdown audio backend
//---------------------------------------------------------------------
inline void GameSound::ShutdownAudioBackend() {
#if GAMESOUND_USE_SDL
    if (audio_device_) {
        GS_DEBUG_PRINT("  SDL: Closing audio device...");
        SDL_CloseAudioDevice(audio_device_);
        GS_DEBUG_PRINT("  SDL: Audio device closed");
        audio_device_ = 0;
    }
    // Only quit audio subsystem if we initialized it ourselves
    if (sdl_audio_init_by_self_ && SDL_WasInit(SDL_INIT_AUDIO)) {
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        GS_DEBUG_PRINT("  SDL: Audio subsystem quit (self-init)");
    }
#else
    if (h_wave_out_) {
        // Set closing flag BEFORE reset to prevent callback from re-entering
        GS_DEBUG_PRINT("  waveOut: Setting closing flag...");
        closing_ = true;
        
        // Small delay to let any in-flight callback complete
        GS_DEBUG_PRINT("  waveOut: Calling waveOutReset...");
        MMRESULT reset_result = waveOutReset(h_wave_out_);
        GS_DEBUG_PRINT("  waveOut: waveOutReset returned: %u", reset_result);
        
        // Give callback thread time to exit
        Sleep(50);
        
        GS_DEBUG_PRINT("  waveOut: Calling waveOutClose...");
        MMRESULT close_result = waveOutClose(h_wave_out_);
        GS_DEBUG_PRINT("  waveOut: waveOutClose returned: %u", close_result);
        h_wave_out_ = NULL;
    } else {
        GS_DEBUG_PRINT("  waveOut: h_wave_out_ is NULL, skipping reset/close");
    }
    
    // Clean up both buffers
    for (int i = 0; i < 2; i++) {
        if (wave_hdr_[i]) {
            GS_DEBUG_PRINT("  waveOut: Cleaning buffer %d...", i);
            if (wave_hdr_[i]->lpData) {
                delete[] wave_hdr_[i]->lpData;
                wave_hdr_[i]->lpData = NULL;
            }
            delete wave_hdr_[i];
            wave_hdr_[i] = NULL;
        }
    }
    GS_DEBUG_PRINT("  waveOut: WAVEHDR buffers cleaned");
#endif
}

//---------------------------------------------------------------------
// Public API: PlayWAV
//---------------------------------------------------------------------
inline int GameSound::PlayWAV(const char* filename, int repeat, int volume) {
    if (!initialized_) return -2;

    // Load or get cached WAV
    WavData* wav = LoadOrCacheWAV(filename);
    if (!wav) return -1;

    // Allocate new channel (returns 0 if limit reached)
    int ch_id = AllocateChannel();
    if (ch_id == 0) return -4;  // Channel limit reached
    Channel* ch = new Channel();
    ch->id = ch_id;
    ch->wav = wav;
    ch->position = 0;
    ch->repeat = repeat;
    ch->volume = (volume < 0) ? 0 : (volume > 1000 ? 1000 : volume);
    ch->is_playing = true;

#if GAMESOUND_USE_SDL
    SDL_LockAudioDevice(audio_device_);
#else
    EnterCriticalSection(&lock_);
#endif
    channels_[ch_id] = ch;
#if GAMESOUND_USE_SDL
    SDL_UnlockAudioDevice(audio_device_);
#else
    LeaveCriticalSection(&lock_);
#endif

    return ch_id;
}

//---------------------------------------------------------------------
// Public API: StopWAV
//---------------------------------------------------------------------
inline int GameSound::StopWAV(int channel) {
#if GAMESOUND_USE_SDL
    SDL_LockAudioDevice(audio_device_);
#else
    EnterCriticalSection(&lock_);
#endif
    std::unordered_map<int, Channel*>::iterator it = channels_.find(channel);
    if (it == channels_.end()) {
#if GAMESOUND_USE_SDL
        SDL_UnlockAudioDevice(audio_device_);
#else
        LeaveCriticalSection(&lock_);
#endif
        return -1;
    }
    ReleaseChannel(channel);
#if GAMESOUND_USE_SDL
    SDL_UnlockAudioDevice(audio_device_);
#else
    LeaveCriticalSection(&lock_);
#endif
    return 0;
}

//---------------------------------------------------------------------
// Public API: IsPlaying
//---------------------------------------------------------------------
inline int GameSound::IsPlaying(int channel) {
#if GAMESOUND_USE_SDL
    SDL_LockAudioDevice(audio_device_);
#else
    EnterCriticalSection(&lock_);
#endif
    std::unordered_map<int, Channel*>::iterator it = channels_.find(channel);
    int result = -1;
    if (it != channels_.end()) {
        result = it->second->is_playing ? 1 : 0;
    }
#if GAMESOUND_USE_SDL
    SDL_UnlockAudioDevice(audio_device_);
#else
    LeaveCriticalSection(&lock_);
#endif
    return result;
}

//---------------------------------------------------------------------
// Public API: SetVolume
//---------------------------------------------------------------------
inline int GameSound::SetVolume(int channel, int volume) {
#if GAMESOUND_USE_SDL
    SDL_LockAudioDevice(audio_device_);
#else
    EnterCriticalSection(&lock_);
#endif
    std::unordered_map<int, Channel*>::iterator it = channels_.find(channel);
    int result = -1;
    if (it != channels_.end()) {
        it->second->volume = (volume < 0) ? 0 : (volume > 1000 ? 1000 : volume);
        result = 0;
    }
#if GAMESOUND_USE_SDL
    SDL_UnlockAudioDevice(audio_device_);
#else
    LeaveCriticalSection(&lock_);
#endif
    return result;
}

//---------------------------------------------------------------------
// Public API: StopAll
//---------------------------------------------------------------------
inline void GameSound::StopAll() {
#if GAMESOUND_USE_SDL
    SDL_LockAudioDevice(audio_device_);
#else
    EnterCriticalSection(&lock_);
#endif
    std::vector<int> channel_ids;
    for (std::unordered_map<int, Channel*>::iterator it = channels_.begin();
         it != channels_.end(); ++it) {
        channel_ids.push_back(it->first);
    }
    for (size_t i = 0; i < channel_ids.size(); i++) {
        ReleaseChannel(channel_ids[i]);
    }
    channels_.clear();
#if GAMESOUND_USE_SDL
    SDL_UnlockAudioDevice(audio_device_);
#else
    LeaveCriticalSection(&lock_);
#endif
}

//---------------------------------------------------------------------
// Public API: SetMasterVolume
//---------------------------------------------------------------------
inline int GameSound::SetMasterVolume(int volume) {
    master_volume_ = (volume < 0) ? 0 : (volume > 1000 ? 1000 : volume);
    return 0;
}

//---------------------------------------------------------------------
// Public API: GetMasterVolume
//---------------------------------------------------------------------
inline int GameSound::GetMasterVolume() const {
    return master_volume_;
}

#endif // GAMESOUND_IMPLEMENTATION
#endif // GAMESOUND_H
