#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <functional>
#include <string>

using namespace Casio::FZ_1;


//------------------------------------------------------------------------------
// Interim unit test infrastructure:
//  - use the T_() macro to define tests, and the RUN() macro to run them all.
//  - tests are defined inside the Tests::Tests() constructor
//  - inside individual tests, use the CHECK() macro to define assertions about
//    how the code under test should behave.

struct Tests {

    constexpr static size_t TEST_COUNT = 64;
    std::function<void()> tests_[TEST_COUNT];
    size_t count_ = 0;
    bool verbose_ = false;

#define T_(name_, ...) \
    assert(count_ < TEST_COUNT); \
    tests_[count_++] = [this] { \
        printf("%s\n", #name_); \
        do { __VA_ARGS__ } while(false); \
        if(verbose_) { putchar('\n'); } \
    }

#define CHECK(X_) \
    if(verbose_) { printf("  %s ?\n", #X_); } \
    assert(X_)

#define RUN() \
    Tests t_; \
    for(size_t i = 0; t_.tests_[i]; i++) { \
        t_.tests_[i](); \
    }

    Tests() {
//------------------------------------------------------------------------------
// Actual tests

T_(test_empty_loader, {
    auto bl = API::BlockLoader(nullptr, 0);
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_NO_BLOCKS);
    CHECK(mb.is_empty());
    CHECK(!mb);
    CHECK(mb.file_type() == TYPE_UNKNOWN);
    CHECK(!mb.header());
    CHECK(!mb.bank_header());
    CHECK(!mb.voice_header());
    CHECK(!mb.bank_header());
    CHECK(!mb.effect_header());
    CHECK(!mb.bank_block(0));
    CHECK(!mb.voice_block(0));
    CHECK(!mb.effect_block(0));
    CHECK(!mb.wave_block(0));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.voice_block(100));
    CHECK(!mb.effect_block(100));
    CHECK(!mb.wave_block(100));
    CHECK(!mb.bank(0));
    CHECK(!mb.voice(0));
    CHECK(!mb.wave(0));
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
    CHECK(!mb.wave(100));
});

T_(test_load_bank, {
    auto bl = API::BlockLoader("fz_data/bank.fzb");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    CHECK(!mb.is_empty());
    CHECK(mb);
    CHECK(mb.file_type() == TYPE_BANK);
    CHECK(mb.count() == 6);

    CHECK(!mb.effect_header());
    CHECK(!mb.voice_header());
    CHECK(!mb.effect_block(0));
    CHECK(!mb.voice_block(0));
    CHECK(!mb.wave_block(0));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.voice_block(100));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.effect_block(100));
    CHECK(!mb.wave_block(100));
    CHECK(mb.bank(0));
    CHECK(mb.voice(0));
    CHECK(mb.wave(0));
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
    CHECK(!mb.wave(100));

    FzFileHeader *header = mb.header();
    CHECK(header);
    CHECK(header->indicator == FzFileHeader::INDICATOR);
    CHECK(header->version == 1);
    CHECK(header->file_type == 2);
    CHECK(header->bank_count == 1);
    CHECK(header->voice_count == 1);
    CHECK(header->block_count == 6);
    CHECK(header->wave_block_count == 4);

    BankBlock *bank_block = mb.bank_block(0);
    CHECK(bank_block);
    CHECK(bank_block->voice_count == 1);
    CHECK(bank_block->midi_hi[0] == 96);
    CHECK(bank_block->midi_lo[0] == 36);
    CHECK(bank_block->velocity_hi[0] == 127);
    CHECK(bank_block->velocity_lo[0] == 1);
    CHECK(bank_block->midi_origin[0] == 72);
    CHECK(bank_block->midi_channel[0] == 0);
    CHECK(bank_block->gchn[0] == 255);
    CHECK(bank_block->area_volume[0] == 0);
    CHECK(bank_block->vp[0] == 0);
    CHECK(bank_block->name[12] == 0);
    CHECK(bank_block->name[13] == 0);
    CHECK(std::string{ bank_block->name } == "BBBBBBBBBBBB");

    BankBlockFileHeader *bank_header = mb.bank_header();
    CHECK(bank_header);
    CHECK(bank_header->header.indicator == FzFileHeader::INDICATOR);
    CHECK(bank_header->header.version == 1);
    CHECK(bank_header->header.file_type == 2);
    CHECK(bank_header->header.bank_count == 1);
    CHECK(bank_header->header.voice_count == 1);
    CHECK(bank_header->header.block_count == 6);
    CHECK(bank_header->header.wave_block_count == 4);
    CHECK(bank_header->midi_hi[0] == 96);
    CHECK(bank_header->midi_lo[0] == 36);
    CHECK(bank_header->velocity_hi[0] == 127);
    CHECK(bank_header->velocity_lo[0] == 1);
    CHECK(bank_header->midi_origin[0] == 72);
    CHECK(bank_header->midi_channel[0] == 0);
    CHECK(bank_header->gchn[0] == 255);
    CHECK(bank_header->area_volume[0] == 0);
    CHECK(bank_header->vp[0] == 0);
    CHECK(bank_header->name[12] == 0);
    CHECK(bank_header->name[13] == 0);
    CHECK(std::string{ bank_header->name } == "BBBBBBBBBBBB");
});

T_(test_load_effect, {
    auto bl = API::BlockLoader("fz_data/effect.fze");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    CHECK(!mb.is_empty());
    CHECK(mb);
    CHECK(mb.file_type() == TYPE_EFFECT);
    CHECK(mb.count() == 1);

    CHECK(!mb.bank_header());
    CHECK(!mb.voice_header());
    CHECK(!mb.bank_block(0));
    CHECK(!mb.voice_block(0));
    CHECK(!mb.wave_block(0));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.voice_block(100));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.effect_block(100));
    CHECK(!mb.wave_block(100));
    CHECK(!mb.bank(0));
    CHECK(!mb.voice(0));
    CHECK(!mb.wave(0));
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
    CHECK(!mb.wave(100));

    FzFileHeader *header = mb.header();
    CHECK(header);
    CHECK(header->indicator == FzFileHeader::INDICATOR);
    CHECK(header->version == 1);
    CHECK(header->file_type == 3);
    CHECK(header->bank_count == 0);
    CHECK(header->voice_count == 0);
    CHECK(header->block_count == 1);
    CHECK(header->wave_block_count == 0);

    EffectBlock *effect_block = mb.effect_block(0);
    CHECK(effect_block);
    CHECK(effect_block->pitchbend_depth = 24);
    CHECK(effect_block->modulation_lfo_pitch = 64);
    CHECK(effect_block->footvolume_amplitude = 64);

    EffectBlockFileHeader *effect_header = mb.effect_header();
    CHECK(effect_header);
    CHECK(effect_header->header.indicator == FzFileHeader::INDICATOR);
    CHECK(effect_header->header.version == 1);
    CHECK(effect_header->header.file_type == 3);
    CHECK(effect_header->header.bank_count == 0);
    CHECK(effect_header->header.voice_count == 0);
    CHECK(effect_header->header.block_count == 1);
    CHECK(effect_header->header.wave_block_count == 0);
    CHECK(effect_header->pitchbend_depth = 24);
    CHECK(effect_header->modulation_lfo_pitch = 64);
    CHECK(effect_header->footvolume_amplitude = 64);
});

T_(test_load_full, {
    auto bl = API::BlockLoader("fz_data/full.fzf");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    CHECK(!mb.is_empty());
    CHECK(mb);
    CHECK(mb.file_type() == TYPE_FULL);
    CHECK(mb.count() == 5);

    CHECK(!mb.bank_header());
    CHECK(mb.effect_header());
    CHECK(mb.voice_header());
    CHECK(!mb.bank_block(0));
    CHECK(mb.effect_block(0));
    CHECK(mb.voice_block(0));
    CHECK(!mb.wave_block(0));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.voice_block(100));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.effect_block(100));
    CHECK(!mb.wave_block(100));
    CHECK(!mb.bank(0));
    CHECK(mb.voice(0));
    CHECK(mb.wave(0));
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
    CHECK(!mb.wave(100));

    EffectBlockFileHeader *effect_header = mb.effect_header();
    CHECK(effect_header);
    CHECK(effect_header->header.indicator == FzFileHeader::INDICATOR);
    CHECK(effect_header->header.version == 1);
    CHECK(effect_header->header.file_type == 0);
    CHECK(effect_header->header.bank_count == 0);
    CHECK(effect_header->header.voice_count == 1);
    CHECK(effect_header->header.block_count == 5);
    CHECK(effect_header->header.wave_block_count == 4);
    CHECK(effect_header->pitchbend_depth = 24);
    CHECK(effect_header->modulation_lfo_pitch = 64);
    CHECK(effect_header->footvolume_amplitude = 64);

    VoiceBlock *voice_block = mb.voice_block(0);
    CHECK(voice_block);
    check_voice((*voice_block)[0]);
});

T_(test_load_voice, {
    auto bl = API::BlockLoader("fz_data/voice.fzv");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    CHECK(!mb.is_empty());
    CHECK(mb);
    CHECK(mb.file_type() == TYPE_VOICE);
    CHECK(mb.count() == 5);

    CHECK(!mb.bank_header());
    CHECK(!mb.effect_header());
    CHECK(!mb.bank_block(0));
    CHECK(!mb.effect_block(0));
    CHECK(!mb.wave_block(0));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.voice_block(100));
    CHECK(!mb.bank_block(100));
    CHECK(!mb.effect_block(100));
    CHECK(!mb.wave_block(100));
    CHECK(!mb.bank(0));
    CHECK(mb.voice(0));
    CHECK(mb.wave(0));
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
    CHECK(!mb.wave(100));

    FzFileHeader *header = mb.header();
    CHECK(header);
    CHECK(header->indicator == FzFileHeader::INDICATOR);
    CHECK(header->version == 1);
    CHECK(header->file_type == 1);
    CHECK(header->bank_count == 0);
    CHECK(header->voice_count == 1);
    CHECK(header->block_count == 5);
    CHECK(header->wave_block_count == 4);

    VoiceBlock *voice_block = mb.voice_block(0);
    CHECK(voice_block);
    check_voice((*voice_block)[0]);

    VoiceBlockFileHeader *voice_header = mb.voice_header();
    CHECK(voice_header);
    CHECK(voice_header->header.indicator == FzFileHeader::INDICATOR);
    CHECK(voice_header->header.version == 1);
    CHECK(voice_header->header.file_type == 1);
    CHECK(voice_header->header.bank_count == 0);
    CHECK(voice_header->header.voice_count == 1);
    CHECK(voice_header->header.block_count == 5);
    CHECK(voice_header->header.wave_block_count == 4);

    check_voice((*voice_header)[0]);
    check_voice(*mb.voice(0));
});


T_(test_memory_object_list, {
    Effect e;
    Bank b;
    Voice v;
    Wave w;
    auto me = API::MemoryEffect::create(e);
    CHECK(me);
    CHECK(!me->prev());
    CHECK(!me->next());
    auto mb = API::MemoryBank::create(b, me);
    CHECK(mb);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(!mb->next());
    auto mv = API::MemoryVoice::create(v, mb);
    CHECK(mv);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(mb->next() == mv);
    CHECK(mv->prev() == mb);
    CHECK(!mv->next());
    auto mw = API::MemoryWave::create(w, mv);
    CHECK(mw);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(mb->next() == mv);
    CHECK(mv->prev() == mb);
    CHECK(mv->next() == mw);
    CHECK(mw->prev() == mv);
    CHECK(!mw->next());

});
//------------------------------------------------------------------------------
    }// end of Tests::Tests()

    void check_voice(Voice &voice) {
        CHECK(voice.data_start == 0);
        CHECK(voice.data_end == 1928);
        CHECK(voice.play_start == 96);
        CHECK(voice.play_end == 1920);
        CHECK(voice.loop == 0x13);
        CHECK(voice.pitch_correction == 0);
        CHECK(voice.filter == 0);
        CHECK(voice.filter_q == 0);
        CHECK(voice.dca_sustain == 0);
        CHECK(voice.dca_end == 7);
        CHECK(voice.dca_rate[0] == 127);
        CHECK(voice.dca_end_level[0] == 255);
        CHECK(voice.dcf_sustain == 0);
        CHECK(voice.dcf_end == 7);
        CHECK(voice.dcf_rate[0] == 127);
        CHECK(voice.dcf_end_level[0] == 255);
        CHECK(voice.lfo_delay == 0);
        CHECK(voice.lfo_name == 128);
        CHECK(voice.lfo_attack == 0);
        CHECK(voice.lfo_rate == 64);
        CHECK(voice.lfo_pitch == 0);
        CHECK(voice.lfo_amplitude == 0);
        CHECK(voice.lfo_filter == 0);
        CHECK(voice.lfo_filter_q == 0);
        CHECK(voice.velocity_filter_q_key_follow == 0);
        CHECK(voice.amplitude_key_follow == 0);
        CHECK(voice.amplitude_rate_key_follow == 0);
        CHECK(voice.filter_key_follow == 0);
        CHECK(voice.filter_rate_key_follow == 0);
        CHECK(voice.velocity_amplitude_key_follow == 0);
        CHECK(voice.velocity_amplitude_rate_key_follow == 0);
        CHECK(voice.velocity_filter_key_follow == 0);
        CHECK(voice.velocity_filter_rate_key_follow == 0);
        CHECK(voice.midi_hi == 96);
        CHECK(voice.midi_lo == 36);
        CHECK(voice.midi_origin == 72);
        CHECK(voice.frequency == 0);
        CHECK(voice.loop_sustain_point == 0);
        CHECK(voice.loop_end_point == 0);
        CHECK(voice.loop_start[0] == 0);
        CHECK(voice.loop_xfade_time[0] == 0);
        CHECK(voice.loop_time[0] == 100);
        CHECK(voice.name[12] == 0);
        CHECK(voice.name[13] == 0);
        CHECK(std::string{ voice.name } == "AAAAAAAAAAAA");
    }
};


int main() {
    RUN();
    return EXIT_SUCCESS;
}
