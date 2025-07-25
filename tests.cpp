#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <functional>

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
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));
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
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));

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
    CHECK(!mb.bank(100));
    CHECK(!mb.voice(100));

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


//------------------------------------------------------------------------------
}// end of Tests::Tests()
};


int main() {
    RUN();
    return EXIT_SUCCESS;
}
