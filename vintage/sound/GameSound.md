# GameSound.h 技术规格文档

## 概述

`GameSound.h` 是一个轻量级多声道音频播放库，为游戏提供低延迟、多通道 WAV 音频播放能力。支持 Windows waveOut 和 SDL2 两种后端，通过软件混音实现每个声道独立音量控制和全局音量控制。Windows 默认使用 waveOut 后端（零外部依赖），Linux/macOS 默认使用 SDL2 后端。

## API 接口

```cpp
class GameSound {
public:
    GameSound();
    ~GameSound();

    // 播放 WAV 文件，返回 channel ID
    // 返回值: >0 表示 channel ID，<=0 表示错误码
    //   -1: 文件不存在或加载失败
    //   -2: 音频设备未初始化
    //   -3: 内存分配失败
    int PlayWAV(const char* filename, int repeat = 1, int volume = 1000);

    // 停止指定 channel
    // 返回值: 0 成功，-1 channel 不存在或未播放
    int StopWAV(int channel);

    // 检查 channel 是否正在播放
    // 返回值: 1 正在播放，0 未播放，-1 channel 不存在（已释放）
    // 注意：播放结束后 channel 自动释放，IsPlaying 返回 -1 而非 0
    int IsPlaying(int channel);

    // 动态调整单个 channel 音量 (0-1000)
    // 返回值: 0 成功，-1 channel 不存在
    int SetVolume(int channel, int volume);

    // 停止所有 channel
    void StopAll();

    // 设置全局音量（影响所有 channel，0-1000）
    // 返回值: 0 成功
    int SetMasterVolume(int volume);

    // 获取全局音量 (0-1000)
    int GetMasterVolume() const;

    // 检查音频后端是否初始化成功
    // 仅在 GAMESOUND_IMPLEMENTATION 模式下可用
    bool IsInitialized() const;
};
```

### IsPlaying 返回值注意事项

`IsPlaying` 返回三态值：
- `1`：channel 正在播放
- `0`：channel 存在但未播放（理论上不会出现，因为未播放的 channel 会自动释放）
- `-1`：channel 已不存在（播放结束后自动释放）

判断播放是否结束应使用 `IsPlaying(ch) != 1`，而非 `!IsPlaying(ch)`，因为 `-1` 是 truthy 值，`!(-1) == 0` 不会触发结束条件。

## 架构设计

```
GameSound
├── WAV 缓存层 (WavCache)
│   └── unordered_map<string, WavData*>  // 文件名 → WAV 数据
├── Channel 管理层 (ChannelManager)
│   └── unordered_map<int, Channel*>     // channel ID → 播放状态
│   └── int master_volume_               // 全局音量 (0-1000)
└── 音频输出层 (Backend)
    ├── Windows: waveOut API (双缓冲区 + 软件混音)
    └── SDL2: SDL_Audio (软件混音)
```

## 核心数据结构

### WAV 数据缓存

```cpp
struct WavData {
    uint8_t* buffer;          // PCM 数据（已转换为 44100Hz / 立体声 / 16-bit）
    uint32_t size;            // PCM 数据大小（字节）
    uint32_t sample_rate;     // 采样率（固定 44100）
    uint16_t channels;        // 声道数（固定 2）
    uint16_t bits_per_sample; // 位深（固定 16）
    int ref_count;            // 引用计数

    ~WavData();               // 释放 buffer
};
```

**缓存策略**：
- 文件名作为 key，使用 `std::unordered_map<std::string, WavData*>` 存储
- `PlayWAV` 时检查缓存，若已存在则直接复用，`ref_count++`
- Channel 销毁时，对应 WavData 的 `ref_count--`
- `GameSound` 析构时遍历缓存，强制释放所有 WavData
- **设计选择**：WAV 缓存不做运行时驱逐，所有缓存数据在 GameSound 析构时统一释放。这是有意为之的简化设计——游戏场景下音效文件数量有限且生命周期与 GameSound 对象一致，运行时驱逐带来的复杂度（如判断何时安全释放、ref_count 为 0 但稍后可能再次播放同一文件）不值得收益。

### Channel 状态

```cpp
struct Channel {
    int id;                   // Channel 唯一 ID
    WavData* wav;             // 指向缓存的 WAV 数据
    uint32_t position;        // 当前播放位置（字节偏移）
    int repeat;               // 剩余重复次数（1=播放一次，0=无限循环，>1=N次）
    int volume;               // 单个 channel 音量 (0-1000)
    bool is_playing;          // 是否正在播放

    Channel();
};
```

### Channel 数量限制

- 最多同时播放 **32 个声音**（`MAX_CHANNELS = 32`）
- `PlayWAV` 在 channel 数量达到上限时返回 `-4`
- **原因**：32 个声道足以覆盖游戏常见场景（BGM + 多种音效同时播放），同时限制混音的计算量和削波风险

### Channel ID 分配策略

- 使用单调递增计数器 `int64_t next_channel_id_`，初始值为 1
- 超过 32700 时回绕到 1，并检查是否被占用
- 返回时转换为 `int`
- **目的**：避免短时间内复用刚释放的 ID，防止上层逻辑混淆

```cpp
int AllocateChannel() {
    if ((int)channels_.size() >= MAX_CHANNELS) {
        return 0;  // No more channels available
    }
    if (next_channel_id_ > 32700) {
        next_channel_id_ = 1;
    }
    while (channels_.count((int)next_channel_id_)) {
        next_channel_id_++;
        if (next_channel_id_ > 32700) next_channel_id_ = 1;
    }
    int id = (int)next_channel_id_;
    next_channel_id_++;
    return id;
}
```

## 音频后端实现

### Windows waveOut 方案

#### 初始化流程

```cpp
// 1. 设置音频格式
WAVEFORMATEX wfx = {0};
wfx.wFormatTag = WAVE_FORMAT_PCM;
wfx.nSamplesPerSec = 44100;
wfx.wBitsPerSample = 16;
wfx.nChannels = 2;  // 立体声输出
wfx.nBlockAlign = (wfx.wBitsPerSample / 8) * wfx.nChannels;  // = 4
wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;  // = 176400

// 2. 打开音频设备（回调模式）
waveOutOpen(&h_wave_out_, WAVE_MAPPER, &wfx, 
            (DWORD_PTR)WaveOutCallback, (DWORD_PTR)this, CALLBACK_FUNCTION);

// 3. 准备并提交双缓冲区
for (int i = 0; i < 2; i++) {
    wave_hdr_[i]->lpData = new char[BUFFER_BYTES];
    wave_hdr_[i]->dwBufferLength = BUFFER_BYTES;
    memset(wave_hdr_[i]->lpData, 0, BUFFER_BYTES);
    waveOutPrepareHeader(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
    waveOutWrite(h_wave_out_, wave_hdr_[i], sizeof(WAVEHDR));
}
```

#### 缓冲区大小设计

**关键概念：帧 vs 样本**

- **帧（frame）**：一个时间点的所有声道数据。立体声一帧 = L + R = 2 个 int16_t = 4 字节
- **样本（sample）**：单个声道的一个 int16_t 值

`GAMESOUND_BUFFER_SAMPLES` 宏定义的是**帧数**（时间点数量），不是 int16_t 数量。

相关常量：
```cpp
BUFFER_FRAMES      = GAMESOUND_BUFFER_SAMPLES       // 帧数（时间点）
OUTPUT_CHANNELS    = 2                                // 立体声输出
BUFFER_TOTAL_SAMPLES = BUFFER_FRAMES * OUTPUT_CHANNELS  // 总 int16_t 数量
BUFFER_BYTES       = BUFFER_TOTAL_SAMPLES * sizeof(int16_t)  // 字节数
```

| GAMESOUND_BUFFER_SAMPLES | 帧数 | 总 int16_t | 字节数 | 每缓冲区时长 | 延迟 | 说明 |
|---|---|---|---|---|---|---|
| 512 | 512 | 1024 | 2048 | ~12ms | ~23ms | 可能断续 |
| 1024 | 1024 | 2048 | 4096 | ~23ms | ~46ms | 可能断续 |
| 2048 | 2048 | 4096 | 8192 | ~46ms | ~92ms | 稳定（默认） |
| 4096 | 4096 | 8192 | 16384 | ~93ms | ~186ms | 很稳定，高延迟 |

**为什么默认值是 2048 帧（46ms）**：waveOut CALLBACK_FUNCTION 模式下，回调间隔约等于单缓冲区播放时长。如果缓冲区时长小于回调间隔，waveOut 在等待下一个缓冲区时会产生静音间隙（断续感 + 播放时间拉长）。实测 512/1024 帧的缓冲区会导致约 2 倍播放时长，2048 帧以上可消除此问题。

#### 双缓冲区设计

**为什么需要双缓冲区**：单缓冲区在播放完成后，需要等待回调函数准备下一个缓冲区，产生播放间隙。双缓冲区让一个缓冲区播放时，另一个在回调中填充，实现无缝衔接。

```cpp
void CALLBACK WaveOutCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, 
                               DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
    if (uMsg != WOM_DONE) return;

    GameSound* sound = (GameSound*)dwInstance;
    
    if (sound->closing_) return;
    
    WAVEHDR* hdr = (WAVEHDR*)dwParam1;  // 已播放完的缓冲区

    // 混音
    int16_t output_buffer[BUFFER_TOTAL_SAMPLES];
    sound->MixAudio(output_buffer, BUFFER_TOTAL_SAMPLES);
    
    if (sound->closing_) return;

    // 复用缓冲区
    memcpy(hdr->lpData, output_buffer, BUFFER_BYTES);
    hdr->dwBufferLength = BUFFER_BYTES;
    hdr->dwFlags = 0;
    waveOutPrepareHeader(hwo, hdr, sizeof(WAVEHDR));
    waveOutWrite(hwo, hdr, sizeof(WAVEHDR));
}
```

**关键点**：
- waveOut 回调参数 `dwParam1` 就是已播放完的 `WAVEHDR` 指针
- 两个缓冲区交替播放，waveOut 自动管理
- 延迟计算：双缓冲总延迟 ≈ 2 × 单缓冲区时长 ≈ 92ms（默认 2048 帧）

#### 软件混音器设计

**重要**：立体声 WAV 的样本是交错存储的 `[L0, R0, L1, R1, ...]`，必须按"帧"处理。

```cpp
// 全局混音缓冲区（int32_t 防止累加溢出）
// 大小 = BUFFER_TOTAL_SAMPLES = BUFFER_FRAMES * OUTPUT_CHANNELS
int32_t mix_buffer_[BUFFER_TOTAL_SAMPLES];

void MixAudio(int16_t* output_buffer, int sample_count) {
    // sample_count = 总 int16_t 数量（帧数 × 声道数）

    // 清空混音缓冲区
    for (int i = 0; i < sample_count; i++) {
        mix_buffer_[i] = 0;
    }

    // 线程安全：waveOut 需要显式加锁；SDL 回调已持有 SDL 锁，无需再加
#if !GAMESOUND_USE_SDL
    EnterCriticalSection(&lock_);
#endif

    for (auto& pair : channels_) {
        Channel* ch = pair.second;
        if (!ch->is_playing) continue;

        // 音量 = 单个 channel 音量 × 全局音量
        float vol = (ch->volume / 1000.0f) * (master_volume_ / 1000.0f);
        
        int bytes_per_sample = ch->wav->bits_per_sample / 8;      // 2
        int bytes_per_frame = bytes_per_sample * ch->wav->channels;  // 4
        
        // frames_to_mix = 总样本数 / 声道数
        int frames_to_mix = sample_count / ch->wav->channels;
        
        // 检查剩余数据是否足够
        uint32_t remaining_bytes = ch->wav->size - ch->position;
        uint32_t remaining_frames = remaining_bytes / bytes_per_frame;
        if (remaining_frames == 0) {
            frames_to_mix = 0;
        } else if ((uint32_t)frames_to_mix > remaining_frames) {
            frames_to_mix = (int)remaining_frames;
        }

        // 逐帧逐声道混音（直接从交错数据读取，不修改 position）
        for (int frame = 0; frame < frames_to_mix; frame++) {
            uint32_t frame_start = ch->position + frame * bytes_per_frame;
            for (uint16_t ch_idx = 0; ch_idx < ch->wav->channels; ch_idx++) {
                uint32_t sample_pos = frame_start + ch_idx * bytes_per_sample;
                int16_t sample = 0;
                if (sample_pos + 1 < ch->wav->size) {
                    sample = (int16_t)((uint16_t)(uint8_t)ch->wav->buffer[sample_pos] |
                                       ((uint16_t)(uint8_t)ch->wav->buffer[sample_pos + 1] << 8));
                }
                int out_idx = frame * ch->wav->channels + ch_idx;
                mix_buffer_[out_idx] += (int32_t)(sample * vol);
            }
        }
        // 一次性推进 position
        ch->position += frames_to_mix * bytes_per_frame;

        // 处理循环/结束逻辑
        if (ch->position >= ch->wav->size) {
            if (ch->repeat == 0) {
                ch->position = 0;  // 无限循环
            } else if (ch->repeat > 1) {
                ch->position = 0;
                ch->repeat--;
            } else {
                // 播放结束：内联释放，避免 ReleaseChannel 的二次查找
                if (ch->wav) ch->wav->ref_count--;
                delete ch;
                it = channels_.erase(it);
                continue;
            }
        }
    }

#if !GAMESOUND_USE_SDL
    LeaveCriticalSection(&lock_);
#endif

    ClampAndConvert(mix_buffer_, output_buffer, sample_count);
}
```

#### 线程安全

- waveOut 回调在独立线程执行，访问 `channels_` 需要 `CRITICAL_SECTION` 加锁
- 主线程调用 `PlayWAV`/`StopWAV`/`SetVolume` 时需加锁
- `closing_` 必须是 `volatile`，因为跨线程访问

#### 析构安全

析构顺序：先关音频后端（停止回调、防止死锁），再清理 channel 和缓存（此时回调已停，无需加锁）。

```cpp
~GameSound() {
    // 1. 关闭音频后端（停止回调）
    ShutdownAudioBackend();

    // 2. 清理 channel 和缓存（回调已停止，无需加锁）
    for (auto& pair : channels_) delete pair.second;
    channels_.clear();
    for (auto& pair : wav_cache_) delete pair.second;
    wav_cache_.clear();

#if !GAMESOUND_USE_SDL
    DeleteCriticalSection(&lock_);
#endif
}
```

waveOut 的 ShutdownAudioBackend：
```cpp
closing_ = true;
waveOutReset(h_wave_out_);
Sleep(50);
waveOutClose(h_wave_out_);
for (int i = 0; i < 2; i++) {
    delete[] wave_hdr_[i]->lpData;
    delete wave_hdr_[i];
}
```

SDL 的 ShutdownAudioBackend：
```cpp
SDL_CloseAudioDevice(audio_device_);
audio_device_ = 0;
// 仅在 GameSound 自己初始化了 audio subsystem 时才退出
// 如果是 GameLib.SDL.h 初始化的，不应退出（否则会破坏 GameLib 的音频）
if (sdl_audio_init_by_self_ && SDL_WasInit(SDL_INIT_AUDIO)) {
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
```

### 重采样设计

#### 为什么需要重采样

音频设备固定输出格式（44100Hz / 立体声 / 16-bit），但 WAV 文件可能具有不同采样率、声道数或位深。**所有 WAV 在加载时统一转换**，混音器无需实时重采样。

#### 转换策略：加载时统一转换

**优点**：
- 混音器逻辑简化，不需要实时重采样
- 性能好，转换只在加载时做一次

**缺点**：
- 内存占用增加（22050Hz → 44100Hz 数据量翻倍）
- 对于游戏音效（通常 < 500KB），内存开销可接受

#### 线性插值算法

```
原始样本：  s[i]        s[i+1]
            |           |
            |           |
目标位置：  |--- frac --|
            |
         output = s[i] * (1-frac) + s[i+1] * frac
```

**步骤**：
1. 解码原始数据为 16-bit 样本数组
2. 按采样率比率进行线性插值重采样
3. 单声道 → 立体声：左右声道复制相同数据
4. 创建新的 WavData（44100Hz, 2ch, 16-bit）

**核心代码**：

```cpp
WavData* ConvertToTargetFormat(WavData* src) {
    // 防御性参数验证（即使 LoadWAVFromFile 已拦截，独立调用时仍需保护）
    if (!src || !src->buffer || src->size == 0 ||
        src->sample_rate == 0 || src->channels == 0) {
        return NULL;
    }

    const uint32_t target_rate = 44100;
    const uint16_t target_channels = 2;

    // 1. 解码为 16-bit（注意符号扩展）
    int16_t* decoded = new int16_t[total_samples];
    for (uint32_t i = 0; i < total_samples; i++) {
        if (src->bits_per_sample == 16) {
            decoded[i] = (int16_t)((uint16_t)(uint8_t)src->buffer[i * 2] | 
                                    ((uint16_t)(uint8_t)src->buffer[i * 2 + 1] << 8));
        } else {
            decoded[i] = (int16_t)((src->buffer[i] - 128) << 8);
        }
    }

    // 2. 线性插值重采样
    double ratio = (double)target_rate / src->sample_rate;
    uint32_t new_samples_per_ch = (uint32_t)(samples_per_channel * ratio);
    
    for (uint16_t ch = 0; ch < src->channels; ch++) {
        double src_index = 0;
        for (uint32_t i = 0; i < new_samples_per_ch; i++) {
            uint32_t idx = (uint32_t)src_index;
            double frac = src_index - idx;
            int16_t s0 = decoded[idx * src->channels + ch];
            int16_t s1 = (idx + 1 < samples_per_component) ? 
                         decoded[(idx + 1) * src->channels + ch] : s0;
            resampled[i * src->channels + ch] = (int16_t)(s0 * (1.0 - frac) + s1 * frac);
            src_index += ratio;
        }
    }

    // 3. 单声道 → 立体声
    if (src->channels == 1) {
        for (uint32_t i = 0; i < new_samples_per_ch; i++) {
            stereo[i * 2] = resampled[i];     // Left
            stereo[i * 2 + 1] = resampled[i]; // Right
        }
    }

    // 4. 创建新 WavData
    WavData* dst = new WavData();
    dst->sample_rate = target_rate;
    dst->channels = target_channels;
    dst->bits_per_sample = 16;
    dst->size = stereo_samples * sizeof(int16_t);
    dst->buffer = new uint8_t[dst->size];
    memcpy(dst->buffer, stereo, dst->size);
    return dst;
}
```

### WAV 文件解析

```cpp
WavData* LoadWAVFromFile(const char* filename) {
    FILE* f = fopen(filename, "rb");
    char header[44];
    fread(header, 1, 44, f);

    // 验证 RIFF/WAVE header
    // 解析 format chunk（注意：必须 cast 到 uint8_t 防止符号扩展）
    uint16_t audio_format = (uint16_t)(header[20] | (header[21] << 8));
    uint16_t channels = (uint16_t)((uint8_t)header[22] | ((uint8_t)header[23] << 8));
    uint32_t sample_rate = (uint32_t)((uint8_t)header[24] | ((uint8_t)header[25] << 8) | 
                                       ((uint8_t)header[26] << 16) | ((uint8_t)header[27] << 24));
    uint16_t bits_per_sample = (uint16_t)((uint8_t)header[34] | ((uint8_t)header[35] << 8));

    // 验证格式参数合法性（防止除零和未定义行为）
    if (channels == 0 || channels > 2) { delete wav; fclose(f); return NULL; }
    if (sample_rate == 0) { delete wav; fclose(f); return NULL; }
    if (bits_per_sample != 8 && bits_per_sample != 16) { delete wav; fclose(f); return NULL; }

    // 查找 data chunk（跳过可能的中间 chunk）
    fseek(f, 12, SEEK_SET);
    while (!found_data) {
        char chunk_id[4];
        uint32_t chunk_size;
        fread(chunk_id, 1, 4, f);
        fread(&chunk_size, 4, 1, f);
        if (chunk_id == "data") {
            wav->size = chunk_size;
            found_data = true;
        } else {
            // WAV 规范：chunk 按 2 字节对齐，奇数大小有 1 字节 padding
            fseek(f, chunk_size + (chunk_size % 2), SEEK_CUR);  // 跳过非 data chunk
        }
    }

    // 验证 data chunk 大小
    if (!found_data || wav->size == 0 || wav->size > 100 * 1024 * 1024) {
        delete wav; fclose(f); return NULL;
    }

    // 读取 PCM 数据
    wav->buffer = new uint8_t[wav->size];
    fread(wav->buffer, 1, wav->size, f);

    // 调用 ConvertToTargetFormat 转换
    WavData* converted = ConvertToTargetFormat(wav);
    delete wav;
    return converted;
}
```

### SDL2 方案

当 `_WIN32` 未定义或 `GAMESOUND_USE_SDL != 0` 时启用。混音算法与 waveOut 完全相同。

#### 头文件引入

使用 SDL2 后端时，GameSound.h 会自动包含 `<SDL2/SDL.h>`、定义 `SDL_MAIN_HANDLED`，并在初始化时调用 `SDL_SetMainReady()`。用户无需手动处理：

```cpp
#define GAMESOUND_USE_SDL 1
#define GAMESOUND_IMPLEMENTATION
#include "GameSound.h"

int main() {
    GameSound sound;
    // ...
}
```

如果用户已在 GameSound.h 之前引入了 SDL.h（如 GameLib.SDL.h），GameSound.h 会检测 `SDL_h_` 宏并跳过重复包含，但仍会定义 `SDL_MAIN_HANDLED`（如果尚未定义）。

#### 与 GameLib.SDL.h 共用

GameSound.h 和 GameLib.SDL.h 可以安全共用：
- `SDL_MAIN_HANDLED`：两者都用 `#ifndef` 保护，不会重复定义
- `SDL_SetMainReady()`：多次调用是安全的（SDL 内部只设一个标志），GameSound.h 和 GameLib.SDL.h 都会调用
- `SDL_Init` vs `SDL_InitSubSystem`：GameSound.h 只初始化 AUDIO 子系统，GameLib.SDL.h 初始化 VIDEO+TIMER，`SDL_WasInit` 检查避免冲突

#### 初始化

```cpp
SDL_SetMainReady();  // SDL_MAIN_HANDLED 已定义，必须调用此函数

// 仅在 audio subsystem 未初始化时初始化（与 GameLib.SDL.h 兼容）
if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
    SDL_InitSubSystem(SDL_INIT_AUDIO);
}

SDL_AudioSpec desired = {0};
desired.freq = 44100;
desired.format = AUDIO_S16SYS;
desired.channels = 2;
desired.samples = BUFFER_FRAMES;       // SDL 的 samples 是帧数
desired.callback = SDLAudioCallback;
desired.userdata = this;

audio_device_ = SDL_OpenAudioDevice(NULL, 0, &desired, &audio_spec_, 0);
SDL_PauseAudioDevice(audio_device_, 0);  // 开始播放
```

**注意**：
- SDL 的 `desired.samples` 是帧数（与 `GAMESOUND_BUFFER_SAMPLES` 含义一致），而 waveOut 的 `dwBufferLength` 是字节数（需要用 `BUFFER_BYTES`）
- `SDL_OpenAudioDevice` 最后参数传 0（不允许格式变更），因为 WASAPI 设备原生格式可能是 float32，如果允许变更会导致回调收到 float32 格式而非 S16，MixAudio 输出的 int16 数据会被误读为 float32 导致无声音

#### SDL 回调的特殊处理

SDL 回调的 `len` 参数可能大于 `BUFFER_TOTAL_SAMPLES`（如 WASAPI 下 len=16384，而 BUFFER_TOTAL_SAMPLES=4096），需要分块混音：

```cpp
void SDLCALL SDLAudioCallback(void* userdata, Uint8* stream, int len) {
    GameSound* sound = (GameSound*)userdata;
    int total_samples = len / sizeof(int16_t);
    int chunk_samples = BUFFER_TOTAL_SAMPLES;
    int16_t* out = (int16_t*)stream;
    int offset = 0;
    while (offset < total_samples) {
        int to_mix = chunk_samples;
        if (offset + to_mix > total_samples) to_mix = total_samples - offset;
        sound->MixAudio(out + offset, to_mix);
        offset += to_mix;
    }
}
```

#### 线程安全

- SDL 回调期间已持有 `SDL_LockAudioDevice` 锁，MixAudio 内不再加锁
- 公共 API（PlayWAV/StopWAV/StopAll）使用 `SDL_LockAudioDevice/SDL_UnlockAudioDevice` 保护

## 错误处理

| 错误码 | 含义 | 触发场景 |
|--------|------|---------|
| `> 0` | 成功，返回 channel ID | 正常播放 |
| `-1` | 文件不存在或加载失败 | 文件路径错误、格式不支持 |
| `-2` | 音频设备未初始化 | `GameSound` 构造失败 |
| `-3` | 内存分配失败 | WAV 文件过大 |
| `-4` | Channel 数量达到上限 | 同时播放超过 32 个声音 |

## 编译说明

### Windows (waveOut 后端，默认)

```bash
g++ -o game.exe game.cpp -mwindows -lwinmm
```

**注意**：必须链接 `-lwinmm` 库。

### Windows (SDL2 后端)

```bash
g++ -o game.exe game.cpp -I<SDL2_include_path> -L<SDL2_lib_path> -lSDL2 -lwinmm -DGAMESOUND_USE_SDL=1
```

**注意**：
- 只需链接 `-lSDL2` 和 `-lwinmm`，无需 `-lmingw32 -lSDL2main`
- GameSound.h 自动定义 `SDL_MAIN_HANDLED` 并调用 `SDL_SetMainReady()`，无需用户处理 SDL_main
- `main()` 函数签名不受 SDL2 影响

### Linux / macOS (SDL2 后端，默认)

在非 Windows 平台上，`GAMESOUND_USE_SDL` 默认为 1，无需手动定义。

```bash
# Linux (SDL2 通常通过包管理器安装)
g++ -o game game.cpp -lSDL2

# macOS (Homebrew 安装的 SDL2)
g++ -o game game.cpp -lSDL2 -I$(brew --prefix sdl2)/include -L$(brew --prefix sdl2)/lib
```

**注意**：
- 无需 `-lwinmm`（Windows 专属库）
- 无需 `-DGAMESOUND_USE_SDL=1`（非 Windows 平台自动启用 SDL 后端）
- SDL2 头文件路径会自动检测（`<SDL2/SDL.h>` 或 `<SDL.h>`）

## 使用示例

```cpp
#define GAMESOUND_IMPLEMENTATION
#include "GameSound.h"

int main() {
    GameSound sound;

    // 播放爆炸音效（播放 1 次，音量 80%）
    int explosion_ch = sound.PlayWAV("assets/explosion.wav", 1, 800);

    // 播放 BGM（无限循环，音量 50%）
    int bgm_ch = sound.PlayWAV("assets/bgm.wav", 0, 500);

    // 调整全局音量（影响所有 channel）
    sound.SetMasterVolume(600);  // 60% 全局音量

    // 动态调整单个 channel 音量
    sound.SetVolume(bgm_ch, 300);  // BGM 音量 30%

    // 停止 BGM
    sound.StopWAV(bgm_ch);

    // 检查是否还在播放（注意返回值三态）
    if (sound.IsPlaying(explosion_ch) == 1) {
        // 还在播放
    } else {
        // 已结束（返回 -1 表示 channel 已释放，返回 0 表示未播放）
    }

    // 游戏结束时，停止所有音效
    sound.StopAll();
}

## 性能考虑

| 项目 | 策略 |
|------|------|
| WAV 缓存 | 同名 WAV 只加载一次，引用计数管理 |
| 混音性能 | O(N) 复杂度，N 为活跃 channel 数 |
| 内存占用 | WAV 数据 + 混音缓冲区（2048帧 ≈ 16KB） |
| 线程安全 | waveOut: CRITICAL_SECTION; SDL: SDL_LockAudioDevice | 保护主线程与回调线程的并发访问 |
| 跨平台 | Windows 默认 waveOut，Linux/macOS 默认 SDL2 | Windows 零依赖；其他平台自动选择 SDL2 |
| 低延迟 | 双缓冲区（2048帧），约 92ms 总延迟 |
| 播放时长精度 | 延迟 < 2%（2048帧缓冲区实测） |

## 实现约束

- **C++11 兼容**：不使用 C++14/17/20 特性
- **GCC 4.9.2 兼容**
- **跨平台**：Windows 使用 waveOut（零依赖），Linux/macOS 使用 SDL2
- **线程安全**：主线程和音频回调线程并发访问需保护
- **实现位置**：所有实现在类内部（`#ifdef GAMESOUND_IMPLEMENTATION`）

## 调试宏定义

```cpp
#ifndef GAMESOUND_DEBUG
#define GAMESOUND_DEBUG 0  // 默认关闭
#endif

#define GS_DEBUG_PRINT(fmt, ...) do { \
    if (GAMESOUND_DEBUG) printf("[GameSound] " fmt "\n", ##__VA_ARGS__); \
} while(0)
```

在包含 `GameSound.h` 之前定义为 1 以启用调试输出。

## 缓冲区配置宏

```cpp
#ifndef GAMESOUND_BUFFER_SAMPLES
#define GAMESOUND_BUFFER_SAMPLES 2048  // 默认 2048 帧
#endif
```

在包含 `GameSound.h` 之前定义以自定义缓冲区大小。**注意：此值是帧数（时间点），不是 int16_t 数量。**

## 技术决策

| 决策项 | 选择 | 原因 | 时间 |
|--------|------|------|------|
| 音频后端 | Windows: waveOut; Linux/macOS: SDL2 | waveOut 零依赖；SDL2 跨平台 | 2026-04-19 |
| 混音方式 | 软件混音 | 每个 channel 独立音量控制 | 2026-04-19 |
| 缓冲区大小 | 2048 帧（默认） | waveOut 回调间隔约等于单缓冲区时长，小于此值会产生断续 | 2026-04-19 |
| 缓冲区单位 | 帧（frame）而非 int16_t | 44100Hz/stereo/16bit 下帧含义明确，避免混淆 | 2026-04-19 |
| 缓冲模式 | 双缓冲区 | 避免单缓冲区的播放间隙 | 2026-04-19 |
| 混音缓冲区类型 | int32_t | 防止多声道累加时溢出 | 2026-04-19 |
| 混音读取方式 | 直接计算位置 | 避免临时修改 position 引发的副作用 | 2026-04-19 |
| 重采样策略 | 加载时统一转换 | 混音器简化，运行时性能好 | 2026-04-19 |
| 重采样算法 | 线性插值 | 简单高效，游戏音效足够 | 2026-04-19 |
| 目标格式 | 44100Hz / 立体声 / 16-bit | 标准 CD 音质，waveOut 兼容 | 2026-04-19 |
| WAV 解析 | 仅支持 PCM | 游戏音效都是 PCM，简化实现 | 2026-04-19 |
| 声道 ID 分配 | 单调递增 + 回绕 | 避免短时间复用，防混淆 | 2026-04-19 |
| 声道 ID 上限 | 32700 回绕 | 防止 int32 溢出 | 2026-04-19 |
| 声道数量上限 | 32 (MAX_CHANNELS) | 32 足以覆盖游戏常见场景，限制混音计算量和削波风险 | 2026-04-19 |
| 线程安全 | CRITICAL_SECTION | Windows 原生，比 mutex 轻量 | 2026-04-19 |
| 析构安全 | volatile closing_ 标志 | 防止 waveOut 回调死锁 | 2026-04-19 |
| 音量范围 | 0-1000 | 整数运算，避免浮点精度问题 | 2026-04-19 |
| 全局音量 | master_volume_ 成员 | 一个接口控制所有 channel | 2026-04-19 |
| IsPlaying 返回值 | 三态（1/0/-1） | 区分"播放中"/"未播放"/"不存在" | 2026-04-19 |
| 类型安全 | uint8_t cast | 防止 char 符号扩展 bug | 2026-04-19 |
| 实现方式 | header-only 单文件 | stb 风格，易于集成 | 2026-04-19 |
| WAV 缓存驱逐 | 不驱逐，析构时统一释放 | 游戏音效数量有限且生命周期与 GameSound 一致，运行时驱逐增加复杂度不值得 | 2026-04-19 |
| SDL2 头文件引入 | GameSound.h 自动包含 SDL.h，自动检测路径 | 支持 `<SDL2/SDL.h>` 和 `<SDL.h>` 两种路径，兼容不同平台和发行版 | 2026-04-19 |
| SDL_MAIN_HANDLED | GameSound.h 自动定义 | 无需 -lmingw32 -lSDL2main，与 GameLib.SDL.h 兼容 | 2026-04-19 |
| 跨平台后端选择 | Windows 默认 waveOut，其他平台默认 SDL2 | waveOut 零依赖适合 Windows；SDL2 是 Linux/macOS 唯一选项 | 2026-04-19 |
| SDL2 链接 | 仅 -lSDL2（Linux/macOS 无需 -lwinmm） | -lwinmm 是 Windows 专属；SDL_MAIN_HANDLED 不需要 SDL2main 入口点适配 | 2026-04-19 |
| SDL 音频格式 | 不允许格式变更（传 0） | WASAPI 原生 float32，允许变更会导致 MixAudio 的 int16 输出被误读为 float32 → 无声音 | 2026-04-19 |
| SDL 回调分块混音 | len 可能大于 BUFFER_TOTAL_SAMPLES | WASAPI 下 len=16384 而 BUFFER_TOTAL_SAMPLES=4096，必须分块 | 2026-04-19 |
| SDL MixAudio 不加锁 | 回调已持有 SDL 锁 | 回调中再次 SDL_LockAudioDevice 会死锁 | 2026-04-19 |
| SDL 析构安全 | 仅退出自己初始化的 subsystem | 避免破坏 GameLib.SDL.h 或其他库的 SDL 音频 | 2026-04-19 |
| 编译兼容 | GCC 4.9.2 / C++11 | Dev-C++ 5 自带编译器 | 2026-04-19 |
| MixAudio channel 释放 | 内联 erase 替代 ReleaseChannel | 避免 ReleaseChannel 的二次 find 查找，迭代器生命周期更清晰 | 2026-04-19 |
| WAV 参数验证 | channels/sample_rate/bits_per_sample 合法性检查 | 防止 ConvertToTargetFormat 除零和未初始化内存访问 | 2026-04-19 |
| WAV data chunk 验证 | size != 0 且 ≤ 100MB | 防止超大值导致 bad_alloc 或 0 导致未定义行为 | 2026-04-19 |
| WAV chunk padding | 奇数大小跳过 padding 字节 | WAV 规范要求 chunk 按 2 字节对齐，奇数大小有 1 字节填充 | 2026-04-19 |
| ConvertToTargetFormat 入口验证 | NULL/空 buffer/size=0/sample_rate=0/channels=0 检查 | 独立调用时的除零保护，不依赖 LoadWAVFromFile 的上游拦截 | 2026-04-19 |