#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stdio.h>

#define TRACE //printf("@-> %s:%u\n", __FILE__, __LINE__)

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
    if(block_type(n) == BT_BANK) {
        return static_cast<BankBlock*>(block_data(n));
    }
    return nullptr;
}

VoiceBlock *MemoryBlocks::voice_block(size_t n) const {
    if(block_type(n) == BT_VOICE) {
        return static_cast<VoiceBlock*>(block_data(n));
    }
    return nullptr;
}

EffectBlock *MemoryBlocks::effect_block(size_t n) const {
    if( (block_type(n) == BT_EFFECT) ||
        (!n && (file_type_ == TYPE_FULL)) ) {
        return static_cast<EffectBlock*>(block_data(n));
    }
    return nullptr;
}

WaveBlock *MemoryBlocks::wave_block(size_t n) const {
    if(block_type(n) == BT_WAVE) {
        return static_cast<WaveBlock*>(block_data(n));
    }
    return nullptr;
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
    // Effect file or full file
    if((file_type_ == TYPE_EFFECT) || (file_type_ == TYPE_FULL)) {
        return static_cast<EffectBlockFileHeader*>(block_data(0));
    }
    return nullptr;
}

Bank *MemoryBlocks::bank(size_t n) const {
    if(auto *h = header()) {
        if(n < h->bank_count) {
            return bank_block(n);
        }
    }
    return nullptr;
}

Voice *MemoryBlocks::voice(size_t n) const {
    size_t
        bank_count = 0,
        voice_count = 0;
    if(auto *h = header()) {
        bank_count = h->bank_count;
        voice_count = h->voice_count;
    }
    if(n < voice_count) {
        if(auto *vb = voice_block( bank_count + ((n + 3) / 4) )) {
            return &(*vb)[n % 4];
        }
    }
    return nullptr;
}

Wave *MemoryBlocks::wave(size_t n) const {
    size_t
        offset = 0,
        wave_block_count = 0;
    if(auto *h = header()) {
        offset = h->bank_count + h->voice_count;
        wave_block_count = h->wave_block_count;
    }
    if(n < wave_block_count) {
        return wave_block(n + offset);
    }
    return nullptr;
}

BlockType MemoryBlocks::block_type(size_t n) const {
    if(n < count_) {
        return block_types_[n];
    }
    return BT_NONE;
}

char MemoryBlocks::block_type_as_char(size_t n) const {
    switch(block_type(n)) {
        case BT_BANK: return 'b';
        case BT_EFFECT: return 'e';
        case BT_VOICE: return 'v';
        case BT_WAVE: return 'w';
        default: [[fallthrough]]
        case BT_NONE: return '_';
    }
}

void MemoryBlocks::reset() {
    storage_.reset();
    block_types_.reset();
    data_ = nullptr;
    count_ = 0;
    file_type_ = TYPE_UNKNOWN;
}

void *MemoryBlocks::block_data(size_t n) const {
    return (n < count_) ? &static_cast<UnknownBlock*>(data_)[n] : nullptr;
}


Result MemoryBlocks::load(std::unique_ptr<uint8_t[]> &&storage, size_t count) {
    storage_ = std::move(storage);
    data_ = storage_.get();
    count_ = count;
    if(count_) {
        file_type_ = FzFileType{ header()->file_type };
    } else {
        file_type_ = TYPE_UNKNOWN;
    }
    return parse();
}


Result MemoryBlocks::parse() {
    block_types_ = std::make_unique<BlockType[]>(count_);
    FzFileHeader *h = header();
    size_t
        bank_blocks = h->bank_count,
        voice_blocks = h->voice_count,
        wave_blocks = h->wave_block_count,
        iterator = 0;
    if(!bank_blocks && !voice_blocks) {
        if(h->block_count == 1) {
            block_types_[0] = BT_EFFECT;
            return RESULT_OK;
        }
    }
    if(bank_blocks) {
        if(bank_blocks > count_) {
            return RESULT_BANK_BLOCK_MISMATCH;
        }
        for(size_t i = 0; i < bank_blocks; i++) {
            block_types_[iterator++] = BT_BANK;
        }
    }
    if(voice_blocks) {
        size_t voice_count = (voice_blocks + 3) / 4;
        if((voice_count + iterator) > count_) {
            return RESULT_VOICE_BLOCK_MISMATCH;
        }
        for(size_t i = 0; i < voice_count; i++) {
            block_types_[iterator++] = BT_VOICE;
        }
    }
    if(wave_blocks) {
        if(wave_blocks + iterator > count_) {
            return RESULT_WAVE_BLOCK_MISMATCH;
        }
        for(size_t i = 0; i < wave_blocks; i++) {
            block_types_[iterator++] = BT_WAVE;
        }
    }
    if(iterator != count_) {
        return RESULT_BLOCK_COUNT_MISMATCH;
    }
    return RESULT_OK;
}


//------------------------------------------------------------------------------

MemoryObjectPtr MemoryObject::prev() {
    if(prev_) {
        return prev_->shared_from_this();
    }
    return nullptr;
}

MemoryObjectPtr MemoryObject::next() {
    if(next_) {
        return next_->shared_from_this();
    }
    return nullptr;
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
        if(r == 1) {
            return;
        }
    }
    storage_ = nullptr;
    size_ = 0;
}

BlockLoader::BlockLoader(std::unique_ptr<uint8_t[]>&& storage, size_t size):
    storage_(std::move(storage)),
    size_(size) {}

BlockLoader::BlockLoader(void *storage, size_t size):
    storage_(std::make_unique<uint8_t[]>(size)),
    size_(size) {
    memcpy(storage_.get(), storage, size);
}

Result BlockLoader::load(MemoryBlocks &mb) {
    mb.reset();
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
    if(h.version != 1) {
        return RESULT_BAD_FILE_VERSION;
    }
    size_t blocks = size_ / 1024;
    if(blocks != h.block_count) {
        return RESULT_BAD_BLOCK_COUNT;
    }
    return mb.load(std::move(storage_), blocks);
}


} // Casio::FZ_1::API
