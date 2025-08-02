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
    check_bank(*bank_block);

    BankBlockFileHeader *bank_header = mb.bank_header();
    CHECK(bank_header);
    check_bank(*bank_header);
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

T_(test_load_dump_load, {
    auto bl1 = API::BlockLoader("fz_data/voice.fzv");
    API::MemoryBlocks mb1;
    auto r1 = bl1.load(mb1);
    CHECK(r1 == API::RESULT_OK);
    CHECK(!mb1.is_empty());
    CHECK(mb1);
    CHECK(mb1.file_type() == TYPE_VOICE);
    CHECK(mb1.count() == 5);

    uint8_t memory[5 * 1024];
    size_t bytes = 0;
    auto bd1 = API::BlockDumper(memory);
    auto r2 = bd1.dump(mb1, &bytes);
    CHECK(r2 == API::RESULT_OK);
    CHECK(bytes = 5 * 1024);

    auto bl2 = API::BlockLoader(memory);
    API::MemoryBlocks mb2;
    auto r3 = bl2.load(mb2);
    CHECK(r3 == API::RESULT_OK);
    CHECK(!mb2.is_empty());
    CHECK(mb2);
    CHECK(mb2.file_type() == TYPE_VOICE);
    CHECK(mb2.count() == 5);

    auto bd2 = API::BlockDumper("fz_data/tmp");
    auto r4 = bd2.dump(mb1, &bytes);
    CHECK(r4 == API::RESULT_OK);
    CHECK(bytes = 5 * 1024);

    auto bl3 = API::BlockLoader("fz_data/tmp");
    remove("fz_data/tmp");
    API::MemoryBlocks mb3;
    auto r5 = bl3.load(mb3);
    CHECK(r5 == API::RESULT_OK);
    CHECK(!mb3.is_empty());
    CHECK(mb3);
    CHECK(mb3.file_type() == TYPE_VOICE);
    CHECK(mb3.count() == 5);
});

T_(test_memory_object_list, {
    Effect e;
    auto me = API::MemoryEffect::create(e);
    CHECK(me);
    CHECK(me->type() == API::BT_EFFECT);
    CHECK(!me->prev());
    CHECK(!me->next());
    Bank b;
    auto mb = API::MemoryBank::create(b, me);
    CHECK(mb);
    CHECK(mb->type() == API::BT_BANK);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(!mb->next());
    Voice v;
    auto mv = API::MemoryVoice::create(v, mb);
    CHECK(mv);
    CHECK(mv->type() == API::BT_VOICE);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(mb->next() == mv);
    CHECK(mv->prev() == mb);
    CHECK(!mv->next());
    Wave w;
    auto mw = API::MemoryWave::create(w, mv);
    CHECK(mw);
    CHECK(mw->type() == API::BT_WAVE);
    CHECK(!me->prev());
    CHECK(me->next() == mb);
    CHECK(mb->prev() == me);
    CHECK(mb->next() == mv);
    CHECK(mv->prev() == mb);
    CHECK(mv->next() == mw);
    CHECK(mw->prev() == mv);
    CHECK(!mw->next());

});

T_(test_unpack_bank, {
    auto bl = API::BlockLoader("fz_data/bank.fzb");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    API::MemoryObjectPtr mo;
    CHECK(!mo);
    auto r2 = mb.unpack(mo);
    CHECK(r2 == API::RESULT_OK);
    CHECK(mo);
    // mo 'anchors' the start of the list, so if we did mo = mo->next() we'd
    // start to drop objects (i.e. !mo->prev() would always hold)
    auto n = mo;
    CHECK(n);
    CHECK(!n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_BANK);
    auto *b = n->bank();
    CHECK(b);
    check_bank(*b);
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_VOICE);
    auto *v = n->voice();
    CHECK(v);
    check_voice(*v);
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(!n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
});

T_(test_unpack_full, {
    auto bl = API::BlockLoader("fz_data/full.fzf");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    API::MemoryObjectPtr mo;
    CHECK(!mo);
    auto r2 = mb.unpack(mo);
    CHECK(r2 == API::RESULT_OK);
    CHECK(mo);
    // mo 'anchors' the start of the list, so if we did mo = mo->next() we'd
    // start to drop objects (i.e. !mo->prev() would always hold)
    auto n = mo;
    CHECK(n);
    CHECK(!n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_EFFECT);
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_VOICE);
    auto *v = n->voice();
    CHECK(v);
    check_voice(*v);
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
    n = n->next();
    CHECK(n);
    CHECK(n->prev());
    CHECK(!n->next());
    CHECK(n->type() == API::BT_WAVE);
    CHECK(n->wave());
});

T_(test_empty_pack_error, {
    API::MemoryObjectPtr mo;
    API::MemoryBlocks mb;
    auto r = API::MemoryObject::pack(mo, mb);
    CHECK(r == API::RESULT_NO_BLOCKS);
    CHECK(!mb.count());
});

T_(test_pack_basic, {
    Voice v1;
    auto mv1 = API::MemoryVoice::create(v1);
    CHECK(mv1);
    Wave w1;
    auto mw1 = API::MemoryWave::create(w1, mv1);
    CHECK(mw1);
    API::MemoryBlocks mb;
    auto r1 = API::MemoryObject::pack(mv1, mb, TYPE_FULL);
    CHECK(r1 == API::RESULT_OK);

    Effect e2;
    auto me2 = API::MemoryEffect::create(e2);
    CHECK(me2);
    Voice v2;
    auto mv2 = API::MemoryVoice::create(v2, me2);
    CHECK(mv2);
    Wave w2;
    auto mw2 = API::MemoryWave::create(w2, mv2);
    CHECK(mw2);
    auto r2 = API::MemoryObject::pack(me2, mb, TYPE_FULL);
    CHECK(r2 == API::RESULT_OK);
});

T_(test_memory_object_insert, {
    Effect e;
    auto me = API::MemoryEffect::create(e);
    CHECK(me);
    CHECK(!me->prev());
    CHECK(!me->next());

    Bank b;
    auto mb1 = API::MemoryBank::create(b);
    CHECK(mb1);
    CHECK(!mb1->prev());
    CHECK(!mb1->next());
    auto mb2 = API::MemoryBank::create(b);
    CHECK(mb2);
    CHECK(!mb2->prev());
    CHECK(!mb2->next());

    Voice v;
    auto mv = API::MemoryVoice::create(v);
    CHECK(mv);
    CHECK(!mv->prev());
    CHECK(!mv->next());

    Wave w;
    auto mw1 = API::MemoryWave::create(w);
    CHECK(mw1);
    CHECK(!mw1->prev());
    CHECK(!mw1->next());
    auto mw2 = API::MemoryWave::create(w);
    CHECK(mw2);
    CHECK(!mw2->prev());
    CHECK(!mw2->next());

    auto p = me->insert_after(mb1);
    CHECK(p);
    CHECK(p == mb1);
    CHECK(!p->prev());
    CHECK(p->next() == me);

    auto q = mv->insert_before(mw1);
    CHECK(q);
    CHECK(q == mv);
    CHECK(!q->prev());
    CHECK(q->next() == mw1);

    auto r = mw2->insert_before(p);
    CHECK(r);
    CHECK(r == mw2);
    CHECK(!r->prev());
    CHECK(r->next() == p);

    auto s = mb2->insert_after(q);
    CHECK(s);
    CHECK(s == q);
    CHECK(!s->prev());
    CHECK(s->next() == mb2);
});

T_(test_xml_roundtrip_bank, {
    auto bl = API::BlockLoader("fz_data/bank.fzb");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(mb.bank(0));
    check_bank(*mb.bank(0));
    CHECK(r == API::RESULT_OK);
    API::MemoryObjectPtr mo;
    CHECK(!mo);
    auto r2 = mb.unpack(mo);
    CHECK(r2 == API::RESULT_OK);
    CHECK(mo);
    CHECK(mo->type() == API::BT_BANK);
    CHECK(mo->bank());
    check_bank(*mo->bank());
    auto xd = API::XmlDumper("fz_data/bank.fzml");
    auto r3 = xd.dump(mo);
    CHECK(r3 == API::RESULT_OK);
    API::MemoryObjectPtr mo2;
    CHECK(!mo2);
    auto xl = API::XmlLoader("fz_data/bank.fzml");
    auto r4 = xl.load(mo2);
    CHECK(r4 == API::RESULT_OK);
    remove("fz_data/bank.fzml");
    CHECK(mo2);
    CHECK(mo2->type() == API::BT_BANK);
    CHECK(mo2->bank());
    check_bank(*mo2->bank());
});

T_(test_xml_roundtrip_full, {
    auto bl = API::BlockLoader("fz_data/full.fzf");
    API::MemoryBlocks mb;
    auto r = bl.load(mb);
    CHECK(r == API::RESULT_OK);
    API::MemoryObjectPtr mo;
    CHECK(!mo);
    auto r2 = mb.unpack(mo);
    CHECK(r2 == API::RESULT_OK);
    CHECK(mo);
    CHECK(mo->next());
    CHECK(mo->next()->type() == API::BT_VOICE);
    CHECK(mo->next()->voice());
    check_voice(*mo->next()->voice());
    auto xd = API::XmlDumper("fz_data/full.fzml");
    auto r3 = xd.dump(mo);
    CHECK(r3 == API::RESULT_OK);
    API::MemoryObjectPtr mo2;
    CHECK(!mo2);
    auto xl = API::XmlLoader("fz_data/full.fzml");
    auto r4 = xl.load(mo2);
    CHECK(r4 == API::RESULT_OK);
    remove("fz_data/full.fzml");
    CHECK(mo2);
    CHECK(mo2->next());
    CHECK(mo2->next()->type() == API::BT_VOICE);
    CHECK(mo2->next()->voice());
    check_voice(*mo2->next()->voice());
});

//------------------------------------------------------------------------------
    }// end of Tests::Tests()

    void check_bank(Bank &bank) {
        CHECK(bank.voice_count == 1);
        CHECK(bank.midi_hi[0] == 96);
        CHECK(bank.midi_lo[0] == 36);
        CHECK(bank.velocity_hi[0] == 127);
        CHECK(bank.velocity_lo[0] == 1);
        CHECK(bank.midi_origin[0] == 72);
        CHECK(bank.midi_channel[0] == 0);
        CHECK(bank.output_mask[0] == 255);
        CHECK(bank.area_volume[0] == 0);
        CHECK(bank.voice_index[0] == 0);
        CHECK(bank.name[12] == 0);
        CHECK(bank.name[13] == 0);
        CHECK(std::string{ bank.name } == "BBBBBBBBBBBB");
    }

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
