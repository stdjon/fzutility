#include "./FZ-1_API.h"
#include <assert.h>
#include <stdio.h>

#define TRACE //printf("@-> %u\n", __LINE__)

namespace Casio::FZ_1::API {

static FzFileHeader &header_(void *b){
    assert(b);
    return static_cast<UnknownBlock*>(b)->header;
}

//------------------------------------------------------------------------------

Block *MemoryBlocks::block(size_t n) const {
    return static_cast<Block*>(block_data(n));
}

BankBlock *MemoryBlocks::bank_block(size_t n) const {
    return static_cast<BankBlock*>(block_data(n));
}

VoiceBlock *MemoryBlocks::voice_block(size_t n) const {
    return static_cast<VoiceBlock*>(block_data(n));
}

EffectBlock *MemoryBlocks::effect_block(size_t n) const {
    return static_cast<EffectBlock*>(block_data(n));
}

WaveBlock *MemoryBlocks::wave_block(size_t n) const {
    return static_cast<WaveBlock*>(block_data(n));
}

FzFileHeader *MemoryBlocks::header() const {
    if(auto *b = block(0)) {
        return &header_(b);
    }
    return nullptr;
}

BankBlockFileHeader *MemoryBlocks::bank_header() const {
    // Bank file, or full file with 1 or more bank
    if( (file_type_ == TYPE_BANK) ||
        ((file_type_ == TYPE_FULL) && header()->bank_count) ) {
        return static_cast<BankBlockFileHeader*>(block_data(0));
    }
    return nullptr;
}

VoiceBlockFileHeader *MemoryBlocks::voice_header() const {
    // Voice file, or full file with 0 banks and 1 or more voices
    if( (file_type_ == TYPE_VOICE) ||
        ((file_type_ == TYPE_FULL) &&
            !header()->bank_count && header()->voice_count) ) {
        return static_cast<VoiceBlockFileHeader*>(block_data(0));
    }
    return nullptr;
}

EffectBlockFileHeader *MemoryBlocks::effect_header() const {
    // Effect file, or full file with 0 banks and 0 voices
    if( (file_type_ == TYPE_EFFECT) ||
        ((file_type_ == TYPE_FULL) &&
            !header()->bank_count && !header()->voice_count) ) {
        return static_cast<EffectBlockFileHeader*>(block_data(0));
    }
    return nullptr;
}

void *MemoryBlocks::block_data(size_t n) const {
    return (n < count_) ? &static_cast<UnknownBlock*>(data_)[n] : nullptr;
}


void MemoryBlocks::load(std::unique_ptr<uint8_t[]> &&storage, size_t count) {
    storage_ = std::move(storage);
    data_ = storage_.get();
    count_ = count;
    if(count_) {
        file_type_ = FzFileType{ header()->file_type };
    } else {
        file_type_ = TYPE_UNKNOWN;
    }
}


//------------------------------------------------------------------------------

BlockLoader::BlockLoader(std::string_view filename) {
    FILE *file = fopen(filename.data(), "rb");
    if(!file) {
        return;
    }
    fseek(file, 0, SEEK_END);
    size_ = ftell(file);
    if(size_) {
        rewind(file);
        storage_ = std::make_unique<uint8_t[]>(size_);
        size_t r = fread(storage_.get(), size_, 1, file);
        fclose(file);
        if(r != 1) {
            storage_ = nullptr;
            size_ = 0;
        }
    }
}

Result BlockLoader::load(MemoryBlocks &mb) {
    if(!size_) {
        return RESULT_NO_BLOCKS;
    }
    if(size_ % 1024) {
        return RESULT_BAD_FILE_SIZE;
    }
    const auto &h = header_(storage_.get());
    if(h.indicator != FzFileHeader::INDICATOR) {
        return RESULT_BAD_HEADER;
    }
    size_t blocks = size_ / 1024;
    if(blocks != h.block_count) {
        return RESULT_BAD_BLOCK_COUNT;
    }
    mb.load(std::move(storage_), blocks);
    return RESULT_OK;
}


} // Casio::FZ_1::API
