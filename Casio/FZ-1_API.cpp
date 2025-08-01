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

Result MemoryBlocks::unpack(MemoryObjectPtr& object) {
    object = nullptr;
    auto *h = header();
    if(!h) {
        return RESULT_NO_BLOCKS;
    }
    MemoryObjectPtr
        current,
        first;
    // Effects are always stored in the first block of any valid input data
    if((file_type_ == TYPE_FULL) || (file_type_ == TYPE_EFFECT)) {
        Effect *e = effect_header();
        assert(e);
        current = MemoryEffect::create(*e, current);
        if(!first) { first = current; }
    }
    size_t
        bank_count = h->bank_count,
        voice_count = h->voice_count,
        wave_count = h->wave_block_count;
    for(size_t i = 0; i < bank_count; i++) {
        if(Bank *b = bank(i)) {
            current = MemoryBank::create(*b, current);
            if(!first) { first = current; }
        } else {
            return RESULT_MISSING_BANK;
        }
    }
    for(size_t i = 0; i < voice_count; i++) {
        if(Voice *v = voice(i)) {
            current = MemoryVoice::create(*v, current);
            if(!first) { first = current; }
        } else {
            return RESULT_MISSING_VOICE;
        }
    }
    for(size_t i = 0; i < wave_count; i++) {
        if(Wave *w = wave(i)) {
            current = MemoryWave::create(*w, current);
            if(!first) { first = current; }
        } else {
            return RESULT_MISSING_WAVE;
        }
    }
    object = first;
    return RESULT_OK;
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

template<typename T, typename U>
auto MemoryObject::create(U &u, MemoryObjectPtr prev) {
    auto result = std::make_shared<T>(Lock{}, u, prev);
    if(prev) {
        // link() must be called *after* the result object is fully constructed
        // (otherwise a std::bad_weak_ptr exception is thrown)
        prev->link(result);
    }
    return result;
}

MemoryObjectPtr MemoryObject::insert_after(MemoryObjectPtr obj) {
    if(auto p = prev_.lock()) {
        obj->prev_ = p;
        p->next_ = obj;
    }
    prev_ = obj;
    obj->next_ = shared_from_this();
    return obj;
}

MemoryObjectPtr MemoryObject::insert_before(MemoryObjectPtr obj) {
    if(next_) {
        obj->next_ = next_;
        next_->prev_ = obj;
    }
    next_ = obj;
    obj->prev_ = shared_from_this();
    return shared_from_this();
}

Result MemoryObject::pack(MemoryObjectPtr in, MemoryBlocks &out, FzFileType type) {
    out.reset();
    size_t
        n = 0,
        effect_count = 0,
        bank_count = 0,
        voice_count = 0,
        wave_count = 0;
    auto o = in;
    while(o) {
        switch(o->type()) {
            case BT_BANK: {
                bank_count++;
                n++;
                break;
            }
            case BT_EFFECT: {
                effect_count++;
                break;
            }
            case BT_VOICE: {
                if(!(voice_count % 4)) { n++; }
                voice_count++;
                break;
            }
            case BT_WAVE: {
                wave_count++;
                n++;
                break;
            }
            default: [[fallthrough]]
            case BT_NONE: {
                return RESULT_BAD_BLOCK;
            }
        }
        o = o->next();
    }
    if(!n) {
        return RESULT_NO_BLOCKS;
    }
    if((effect_count != 0) && (effect_count != 1)) {
        return RESULT_BAD_EFFECT_BLOCK_COUNT;
    }
    size_t voice_block_count = (voice_count + 3) / 4;
    if((bank_count + voice_block_count + wave_count) != n) {
        return RESULT_BAD_BLOCK_COUNT;
    }
    size_t storage_size = n * 1024;
    auto storage = std::make_unique<uint8_t[]>(storage_size);
    void *memory = storage.get();
    if(!memory) {
        return RESULT_NO_BLOCKS;
    }
    auto *block = static_cast<UnknownBlock*>(memory);

    auto &h = block->header;
    h.indicator = FzFileHeader::INDICATOR;
    h.version = 1;
    h.file_type = type;
    h.bank_count = bank_count;
    h.voice_count = voice_count;
    h.unused1_ = 0;
    h.block_count = n;
    h.wave_block_count = wave_count;
    h.unused2_ = 0;

    o = in;
    size_t
        voice_index = 0,
        i = 0;
    while(o && i < n) {
        const bool is_voice = o->type() == BT_VOICE;
        size_t index = is_voice ? voice_index : 0;
        if(!o->pack(block + i, index)) {
            return RESULT_BAD_BLOCK_INDEX;
        }
        if(is_voice) {
            voice_index = (voice_index + 1) % 4;
        }
        if(o->type() != BT_EFFECT) {
            i++;
        }
        o = o->next();
    }
    if(i != n) {
        return RESULT_BLOCK_COUNT_MISMATCH;
    }
    return out.load(std::move(storage), n);
}


std::shared_ptr<MemoryBank> MemoryBank::create(
    const Bank &bank, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryBank>(bank, prev);
}

bool MemoryBank::pack(Block *block, size_t index) {
    if(!index) {
        Bank *dst = static_cast<BankBlock*>(block);
        *dst = bank_;
        return true;
    }
    return false;
}


std::shared_ptr<MemoryEffect> MemoryEffect::create(
    const Effect &effect, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryEffect>(effect, prev);
}

bool MemoryEffect::pack(Block *block, size_t index) {
    if(!index) {
        Effect *dst = static_cast<EffectBlock*>(block);
        *dst = effect_;
        return true;
    }
    return false;
}


std::shared_ptr<MemoryVoice> MemoryVoice::create(
    const Voice &voice, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryVoice>(voice, prev);
}

bool MemoryVoice::pack(Block *block, size_t index) {
    if(index < 4) {
        auto *dst = static_cast<VoiceBlock*>(block);
        (*dst)[index] = voice_;
        return true;
    }
    return false;
}


std::shared_ptr<MemoryWave> MemoryWave::create(
    const Wave &wave, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryWave>(wave, prev);
}

bool MemoryWave::pack(Block *block, size_t index) {
    if(!index) {
        Wave *dst = static_cast<WaveBlock*>(block);
        *dst = wave_;
        return true;
    }
    return false;
}


//------------------------------------------------------------------------------

BlockLoader::BlockLoader(std::string_view filename) {
    FILE *file = fopen(filename.data(), "rb");
    if(!file) {
        return;
    }

    struct FileCloser {
        FileCloser(FILE *f): f_(f) {}
        ~FileCloser() { fclose(f_); }
        FILE *f_;
    } close_file(file);

    fseek(file, 0, SEEK_END);
    size_ = ftell(file);
    if(size_) {
        rewind(file);
        storage_ = std::make_unique<uint8_t[]>(size_);
        size_t r = fread(storage_.get(), size_, 1, file);
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
