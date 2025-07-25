#ifndef CASIO_FZ_1
#define CASIO_FZ_1

// Based on information in:
//  - "CASIO FZ DATA TRANSFERS" by Jeff McClintock
//  - "CASIO DIGITAL SAMPLING KEYBOARD MODEL FZ-1 DATA STRUCTURES" by T. Sasaki

#include <stdint.h>
#include <array>

namespace Casio::FZ_1 {

//------------------------------------------------------------------------------
// FZCom File Header (for *.fzb, *.fze, *.fzf, *fzv formats)

enum FzFileType: uint8_t {
    TYPE_UNKNOWN = 255,
    TYPE_FULL = 0,
    TYPE_VOICE = 1,
    TYPE_BANK = 2,
    TYPE_EFFECT = 3,
};

struct FzFileHeader {
    static constexpr uint32_t INDICATOR = 0x1c679068;

    uint32_t indicator = INDICATOR; // FZCom file indicator (always = 476549224)
    int16_t version = 0; // FZCom file format version no. (always = 1)
    uint8_t file_type = 0; // Type of file (Full:0, Voice:1 Bank:2, ,Effects:3)
    uint8_t bank_count = 0; // Number of banks in this file
    uint8_t voice_count = 0; // Number of voices in this file
    uint8_t unused1_ = 0; // unused? (set to 0)
    int16_t block_count = 0; // Number of blocks in the file (1 block = 1024 bytes)
    int16_t wave_block_count = 0; // Number of PCM data blocks
    int16_t unused2_ = 0; // Reserved (set to 0)
};
static_assert(sizeof(FzFileHeader) == 16, "Casio FZ-1 FzFileHeader struct should be 12 bytes");


struct Block {};

//------------------------------------------------------------------------------
// Bank / BankBlock

struct Bank {
    static constexpr size_t MAXV = 64; // Maximum number of voices

    uint16_t voice_count = 0; // 0-64 Number of voices used in the bank
    uint8_t midi_hi[MAXV] = { 0 }; // Hi-Note settings (MIDI note number)
    uint8_t midi_lo[MAXV] = { 0 }; // Lo-Note settings (MIDI note number)
    uint8_t velocity_hi[MAXV] = { 0 }; // 1-127 Velocity split hi
    uint8_t velocity_lo[MAXV] = { 0 }; // 1-127 Velocity split lo
    uint8_t midi_origin[MAXV] = { 0 }; // Original note (MIDI note number)
    uint8_t midi_channel[MAXV] = { 0 }; // 0-15 Recieve MIDI channel
    uint8_t gchn[MAXV] = { 0 }; // Each bit represents an 'individual output'
    uint8_t area_volume[MAXV] = { 0 }; // 1-127 Area volume (Usually 127)
    uint16_t vp[MAXV] = { 0 }; // 0-63 Voice number to use in area
    char name[14] = { 0 }; // Bank name, last two bytes should always be 0
};
static_assert(sizeof(Bank) == 656, "Casio FZ-1 Bank struct should be 656 bytes");

struct BankBlock: Block, Bank {
    uint8_t pad_[1024 - sizeof(Bank)] = { 0 };
};
static_assert(sizeof(BankBlock) == 1024, "Casio FZ-1 BankBlock struct should be 1024 bytes");

struct BankBlockFileHeader: Block, Bank {
    uint8_t pad_[1000 - sizeof(Bank)] = { 0 };
    FzFileHeader header;
    uint8_t pad2_[24 - sizeof(FzFileHeader)] = { 0 };
};
static_assert(sizeof(BankBlockFileHeader) == 1024, "Casio FZ-1 BankBlockFileHeader struct should be 1024 bytes");


//------------------------------------------------------------------------------
// Voice / VoiceBlock

struct Voice {
    int32_t data_start = 0; // Sample data start address
    int32_t data_end = 0; // Sample data end address
    int32_t play_start = 0; // Sample play start address
    int32_t play_end = 0; // Sample play end address (usually =data_end - 2)

    int16_t loop = 0;
        // 0x0000 No waveform,
        // 0x01d7 Normal sound,(default)
        // 0x101d Reversed,
        // 0x2014 Cueing sound (played with WHEEL),
        // 0x0013 Synthesised waveform.

    // FZ supports up to 8 loops
    int8_t loop_sustain_point = 0; // 0-8 sustain loop, 8 = No sustain loop
    int8_t loop_end_point = 0; // 0-8 last loop, 8 = execute all loops

    // Loop starts, upper 8 bits are loop fine settings
    // (set =play_end for unused loop)
    int32_t loop_start[8] = { 0 };

    // Loop ends, MSB = Skip or Trace (set =play_end for unused loop)
    int32_t loop_end[8] = { 0 };
    int16_t loop_xfade_time[8] = { 0 }; // 0-1023 Loop cross fade time

    // 1-1022 Loop time: 16ms to 16s (default is 64),
    // 1024 = continuous loop,
    // 1023 = loop during sustain
    uint16_t loop_time[8] = { 0 };

    // 0-255 loop pitch correction times 1/256 semitone
    int16_t pitch_correction = 0;

    int8_t filter = 0; // 0-127 Filter cut off frequency
    int8_t filter_q = 0; // 0-127 Filter resonance (Q) lower 3 bits ignored

    // The FZ has 8 stage envelopes (rate/level type)
    // A good default is to set all dca_rate[] to 0xc0, and dcf_rate[] to 0x90
    // except for dca_rate[0]=0x7f,dcf_rate[0]=0x7f,
    // dcf_stop[0] to 0xff , dca_stop[0] to 0xff
    int8_t dca_sustain = 0; // 0-7 Sustain section # on DCA envelope
    int8_t dca_end = 0; // 0-7 End section # on DCA envelope (default to 7)
    int8_t dca_rate[8] = { 0 }; // 0-127 Envelope rates MSB = direction up or down
    uint8_t dca_end_level[8] = { 0 }; // 0-255 Envelope section end level
    int8_t dcf_sustain = 0; // 0-7 Sustain section # on DCF envelope
    int8_t dcf_end = 0; // 0-7 End section # on DCF envelope (default to 7)
    int8_t dcf_rate[8] = { 0 }; // 0-127 Envelope rates MSB = direction up or down
    uint8_t dcf_end_level[8] = { 0 }; // 0-255 Envelope section end level
                         //
    uint16_t lfo_delay = 0; // 0-65535 LFO Delay time 2 ms steps
    uint8_t lfo_name = 0;
        // 0-Sine, 1-Saw up, 2-Saw down, 3-triangle,4-rectangle, 5 random.
        // MSB=phase sync (default MSB =1)
    uint8_t lfo_attack = 0; // 1-127
    int8_t lfo_rate = 0; // 0-127 (default to 0x40)
    int8_t lfo_pitch = 0; // 0-127 LFO effect on Pitch
    int8_t lfo_amplitude = 0; // 0-127 LFO effect on amplitude
    int8_t lfo_filter = 0; // 0-127 LFO effect on filter
    int8_t lfo_filter_q = 0; // 0-127 LFO effect on filter Q
    int8_t velocity_filter_q_key_follow = 0; // -127 to 127 ,velocity effect on filter Q
    int8_t amplitude_key_follow = 0; // DCA amplitude Key follow
    int8_t amplitude_rate_key_follow = 0; // DCA rate Key follow
    int8_t filter_key_follow = 0; // DCF amplitude Key follow
    int8_t filter_rate_key_follow = 0; // DCF rate Key follow

    // -127 to 127 Key Velocity effect on DCA envelope amplitude
    // (default to 0x30)
    int8_t velocity_amplitude_key_follow = 0;

    int8_t velocity_amplitude_rate_key_follow = 0; // -127 to 127 Key Velocity effect on DCA envelope rate
    int8_t velocity_filter_key_follow = 0; // Velocity effect on filter envelope amplitude
    int8_t velocity_filter_rate_key_follow = 0; // Velocity effect on filter envelope rate
    uint8_t midi_hi = 0; // Hi note (MIDI note number) (default to 0x60)
    uint8_t midi_lo = 0; // Low note (MIDI note number) (default to 0x24)
    uint8_t midi_origin = 0; // Original note (MIDI note number) (default to 0x48)
    uint8_t frequency = 0; // sampling freq 0 - 36khz, 1 - 18khz, 2 - 9khz
    char name[14] = { 0 }; // ASCII name. last 2 bytes always 0
};
static_assert(sizeof(Voice) == 192, "Casio FZ-1 Voice struct should be 192 bytes");

struct VoicePad_: Voice {
    uint8_t pad_[256 - sizeof(Voice)] = { 0 };
};
static_assert(sizeof(VoicePad_) == 256, "Casio FZ-1 VoicePad_ struct should be 256 bytes");

struct VoiceBlock: Block {
    VoicePad_ v1;
    VoicePad_ v2;
    VoicePad_ v3;
    VoicePad_ v4;

    Voice &operator[](size_t n);
    const Voice &operator[](size_t n) const;
};
static_assert(sizeof(VoiceBlock) == 1024, "Casio FZ-1 VoiceBlock struct should be 1024 bytes");

struct VoiceBlockFileHeader: Block {
    VoicePad_ v1;
    VoicePad_ v2;
    VoicePad_ v3;
    Voice v4;
    uint8_t pad1_[40];
    FzFileHeader header;
    uint8_t pad2_[24 - sizeof(FzFileHeader)] = { 0 };

    Voice &operator[](size_t n);
    const Voice &operator[](size_t n) const;
};
static_assert(sizeof(VoiceBlockFileHeader) == 1024, "Casio FZ-1 VoiceBlockFileHeader struct should be 1024 bytes");

//------------------------------------------------------------------------------
// Effect / EffectBlock

struct Effect {
    int8_t pitchbend_depth = 0; // bender depth
    int8_t master_volume = 0; // master volume
    int8_t sustain_switch = 0; // sustain switch ON/OFF

    // Modulation to...
    int8_t modulation_lfo_pitch = 0;
    int8_t modulation_lfo_amplitude = 0;
    int8_t modulation_lfo_filter = 0;
    int8_t modulation_lfo_filter_q = 0;
    int8_t modulation_filter = 0;
    int8_t modulation_amplitude = 0;
    int8_t modulation_filter_q = 0;

    // Foot volume to...
    int8_t footvolume_lfo_pitch = 0;
    int8_t footvolume_lfo_amplitude = 0;
    int8_t footvolume_lfo_filter = 0;
    int8_t footvolume_lfo_filter_q = 0;
    int8_t footvolume_amplitude = 0;
    int8_t footvolume_filter = 0;
    int8_t footvolume_filter_q = 0;

    // Aftertouch to...
    int8_t aftertouch_lfo_pitch = 0;
    int8_t aftertouch_lfo_amplitude = 0;
    int8_t aftertouch_lfo_filter = 0;
    int8_t aftertouch_lfo_filter_q = 0;
    int8_t aftertouch_amplitude = 0;
    int8_t aftertouch_filter = 0;
    int8_t aftertouch_filter_q = 0;
};
static_assert(sizeof(Effect) == 24, "Casio FZ-1 Effect struct should be 24 bytes");

struct EffectPad1_ {
   uint8_t pad_[960] = { 0 };
};

struct EffectPad_: EffectPad1_, Effect {};
static_assert(sizeof(EffectPad_) == 984, "Casio FZ-1 EffectPad_ struct should be 984 bytes");

struct EffectBlock: Block, EffectPad_ {
    uint8_t pad_[1024 - 984] = { 0 };
};
static_assert(sizeof(EffectBlock) == 1024, "Casio FZ-1 EffectBlock struct should be 1024 bytes");

struct EffectBlockFileHeader: Block, EffectPad_ {
    uint8_t pad_1[16];
    FzFileHeader header;
    uint8_t pad2_[24 - sizeof(FzFileHeader)] = { 0 };
};
static_assert(sizeof(EffectBlockFileHeader) == 1024, "Casio FZ-1 EffectBlockFileHeader struct should be 1024 bytes");

//------------------------------------------------------------------------------
// Wave Block

struct Wave {
    int16_t samples[512];
};
static_assert(sizeof(Wave) == 1024, "Casio FZ-1 Wave struct should be 1024 bytes");

struct WaveBlock: Block, Wave {};
static_assert(sizeof(WaveBlock) == 1024, "Casio FZ-1 WaveBlock struct should be 1024 bytes");

//------------------------------------------------------------------------------

// used internally: represents an as-yet unparsed block
struct UnknownBlock: Block {
    uint8_t pad_[1000];
    FzFileHeader header;
    uint8_t pad2_[24 - sizeof(FzFileHeader)] = { 0 };
};
static_assert(sizeof(UnknownBlock) == 1024, "Casio FZ-1 UnknownBlock struct should be 1024 bytes");

} // Casio::FZ_1

#endif //CASIO_FZ_1
