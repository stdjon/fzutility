#include "Casio/FZ-1_API.h"
#include "3/tinywav/tinywav.h"
#include "3/tinyxml2/tinyxml2.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#define TRACE //printf("@-> %s:%u\n", __FILE__, __LINE__)

namespace Casio::FZ_1::API {

static const std::string
    FZ_ML_ROOT_NAME = "fz-ml",
    FZ_ML_VERSION = "0.1α",
    BANK_TAGNAME = "bank",
    EFFECT_TAGNAME = "effect",
    VOICE_TAGNAME = "voice",
    WAVE_TAGNAME = "wave";


struct FileCloser {
    FileCloser(FILE *f): f_(f) {}
    ~FileCloser() { fclose(f_); }
    FILE *f_;
};

static FzFileHeader &header_(void *b){
    assert(b);
    return static_cast<UnknownBlock*>(b)->header;
}


//------------------------------------------------------------------------------
// XmlElement read/print helpers

// read a (signed) integer
template<typename T>
static void read_value(const XmlElement &element, const char *name, T &in) {
    in = element.IntAttribute(name, 0);
}

// read an unsigned integer
template<typename T>
static void read_unsigned_value(
    const XmlElement &element, const char *name, T &in) {
    in = element.UnsignedAttribute(name, 0);
}

// read a comma-separated integer list (e.g. 1, 2, 3) into the members of an array
template<typename T>
static void read_value_array(
    const XmlElement &element, const char *name, size_t count, T *in) {
    if(auto *e = element.FirstChildElement(name)) {
        if(const char *text = e->GetText()) {
            const char *s = text;
            size_t i = 0;
            while(s && (i < count)) {
                in[i++] = atoll(s);
                s = strchr(s + 1, ',');
                if(s) { s++; }
            }
        }
    }
}

template<typename T>
void print_value(XmlPrinter &p, const char *name, const T &out) {
    if(out) {
        p.PushAttribute(name, out);
    }
}

template<typename T>
void print_value_array(XmlPrinter &p, const char *name, size_t count, const T *out) {
    p.OpenElement(name);
    for(size_t i = 0; i < count; i++) {
        p.PushText(out[i]);
        if(i < (count - 1)) { p.PushText(", "); }
    }
    p.CloseElement();
}


//------------------------------------------------------------------------------

#define FZ_RESULT_STRING(name_, _) #name_,
#define FZ_RESULT_DESCRIPTION(_, desc_) desc_,

const char *result_str(Result r) {
    static const char *strings[] = {
        FZ_EACH_RESULT(FZ_RESULT_STRING)
    };
    if(r < RESULT_END_OF_LIST_) {
        return strings[r];
    }
    return "??";
}

const char *result_description(Result r) {
    static const char *strings[] = {
        FZ_EACH_RESULT(FZ_RESULT_DESCRIPTION)
    };
    if(r < RESULT_END_OF_LIST_) {
        return strings[r];
    }
    return "??";
}

bool result_success(Result r) {
    return r == RESULT_OK;
}


//------------------------------------------------------------------------------
// MemoryBlocks

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
        if(auto *vb = voice_block( bank_count + (n / 4) )) {
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
        offset = h->bank_count + ((h->voice_count + 3) / 4);
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
        default: [[fallthrough]];
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
        return RESULT_BAD_HEADER;
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
    if(count) {
        storage_ = std::move(storage);
        data_ = storage_.get();
        count_ = count;
        file_type_ = FzFileType{ header()->file_type };
    } else {
        file_type_ = TYPE_UNKNOWN;
        return RESULT_NO_BLOCKS;
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
            return RESULT_MISMATCHED_BANK_BLOCK;
        }
        for(size_t i = 0; i < bank_blocks; i++) {
            block_types_[iterator++] = BT_BANK;
        }
    }
    if(voice_blocks) {
        size_t voice_count = (voice_blocks + 3) / 4;
        if((voice_count + iterator) > count_) {
            return RESULT_MISMATCHED_VOICE_BLOCK;
        }
        for(size_t i = 0; i < voice_count; i++) {
            block_types_[iterator++] = BT_VOICE;
        }
    }
    if(wave_blocks) {
        if(wave_blocks + iterator > count_) {
            return RESULT_MISMATCHED_WAVE_BLOCK;
        }
        for(size_t i = 0; i < wave_blocks; i++) {
            block_types_[iterator++] = BT_WAVE;
        }
    }
    if(iterator != count_) {
        return RESULT_MISMATCHED_BLOCK_COUNT;
    }
    return RESULT_OK;
}


//------------------------------------------------------------------------------
// MemoryObject

template<typename T, typename U>
auto MemoryObject::create(const U &u, MemoryObjectPtr prev) {
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
    // TODO: this current implementation assumes that the input list is sorted
    //   (i.e.) (Effect->)Bank(s)->Voice(s)->Wave(s). It will probably break if
    //   this is not the case.
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
            default:
                [[fallthrough]];
            case BT_NONE: {
                return RESULT_BAD_BLOCK;
            }
        }
        o = o->next();
    }
    if(!n) {
        return RESULT_NO_BLOCKS;
    }
    if(effect_count > 1) {
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

    block->header = {
        .indicator = FzFileHeader::INDICATOR,
        .version = 1,
        .file_type = type,
        .bank_count = static_cast<uint8_t>(bank_count),
        .voice_count = static_cast<uint8_t>(voice_count),
        .unused1_ = 0,
        .block_count = static_cast<int16_t>(n),
        .wave_block_count = static_cast<int16_t>(wave_count),
        .unused2_ = 0,
    };

    o = in;
    size_t
        voice_index = 0,
        i = 0;
    while(o && i < n) {
        const bool is_voice = o->type() == BT_VOICE;
        if(!is_voice) {
            if(voice_index) {
                voice_index = 0;
                i++;
            }
        }
        size_t index = is_voice ? voice_index : 0;
        if(!o->pack(block + i, index)) {
            return RESULT_BAD_BLOCK_INDEX;
        }
        if(is_voice) {
            voice_index++;
            if(voice_index == 4) {
                voice_index = 0;
                i++;
            }
        } else {
            if(o->type() != BT_EFFECT) {
                i++;
            }
        }
        o = o->next();
    }
    if(i != n) {
        return RESULT_MISMATCHED_BLOCK_COUNT;
    }
    return out.load(std::move(storage), n);
}


//------------------------------------------------------------------------------
// MemoryBank

template<typename U>
std::shared_ptr<MemoryBank> MemoryBank::create(
    const U &u, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryBank>(u, prev);
}

MemoryBank::MemoryBank(Lock, const XmlElement &element, MemoryObjectPtr prev):
    MemoryObject(prev) {
#define READ_VALUE_ARRAY(name_, count_) \
    read_value_array(element, #name_, count_, bank_.name_)

    const char *name = nullptr;
    element.QueryStringAttribute("name", &name);
    memcpy(bank_.name, name, 14);
    bank_.name[12] = 0;
    bank_.name[13] = 0;

    read_unsigned_value(element, "index", index_);

    unsigned int voice_count = 0;
    read_unsigned_value(element, "voice_count", voice_count);
    bank_.voice_count = voice_count;

    READ_VALUE_ARRAY(midi_hi, voice_count);
    READ_VALUE_ARRAY(midi_lo, voice_count);
    READ_VALUE_ARRAY(velocity_hi, voice_count);
    READ_VALUE_ARRAY(velocity_lo, voice_count);
    READ_VALUE_ARRAY(midi_origin, voice_count);
    READ_VALUE_ARRAY(midi_channel, voice_count);
    READ_VALUE_ARRAY(output_mask, voice_count);
    READ_VALUE_ARRAY(area_volume, voice_count);
    READ_VALUE_ARRAY(voice_index, voice_count);
#undef READ_VALUE_ARRAY
}

bool MemoryBank::pack(Block *block, size_t index) {
    if(!index) {
        Bank *dst = static_cast<BankBlock*>(block);
        *dst = bank_;
        return true;
    }
    return false;
}

void MemoryBank::print(XmlPrinter &p) {
#define PRINT_VALUE_ARRAY(name_, count_) \
    print_value_array(p, #name_, count_, bank_.name_)

    size_t voice_count = bank_.voice_count;
    p.OpenElement(BANK_TAGNAME.c_str());
    p.PushAttribute("name", bank_.name);
    p.PushAttribute("index", index_);
    p.PushAttribute("voice_count", voice_count);
    PRINT_VALUE_ARRAY(midi_hi, voice_count);
    PRINT_VALUE_ARRAY(midi_lo, voice_count);
    PRINT_VALUE_ARRAY(velocity_hi, voice_count);
    PRINT_VALUE_ARRAY(velocity_lo, voice_count);
    PRINT_VALUE_ARRAY(midi_origin, voice_count);
    PRINT_VALUE_ARRAY(midi_channel, voice_count);
    PRINT_VALUE_ARRAY(output_mask, voice_count);
    PRINT_VALUE_ARRAY(area_volume, voice_count);
    PRINT_VALUE_ARRAY(voice_index, voice_count);
    p.CloseElement();
#undef PRINT_VALUE_ARRAY
}


//------------------------------------------------------------------------------
// MemoryEffect

template<typename U>
std::shared_ptr<MemoryEffect> MemoryEffect::create(
    const U &u, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryEffect>(u, prev);
}

MemoryEffect::MemoryEffect(Lock, const XmlElement &element, MemoryObjectPtr prev):
    MemoryObject(prev) {
#define READ_VALUE(name_) \
    read_value(element, #name_, effect_.name_)

    READ_VALUE(pitchbend_depth);
    READ_VALUE(master_volume);
    READ_VALUE(sustain_switch);
    READ_VALUE(modulation_lfo_pitch);
    READ_VALUE(modulation_lfo_amplitude);
    READ_VALUE(modulation_lfo_filter);
    READ_VALUE(modulation_lfo_filter_q);
    READ_VALUE(modulation_filter);
    READ_VALUE(modulation_amplitude);
    READ_VALUE(modulation_filter_q);
    READ_VALUE(footvolume_lfo_pitch);
    READ_VALUE(footvolume_lfo_amplitude);
    READ_VALUE(footvolume_lfo_filter);
    READ_VALUE(footvolume_lfo_filter_q);
    READ_VALUE(footvolume_amplitude);
    READ_VALUE(footvolume_filter);
    READ_VALUE(footvolume_filter_q);
    READ_VALUE(aftertouch_lfo_pitch);
    READ_VALUE(aftertouch_lfo_amplitude);
    READ_VALUE(aftertouch_lfo_filter);
    READ_VALUE(aftertouch_lfo_filter_q);
    READ_VALUE(aftertouch_amplitude);
    READ_VALUE(aftertouch_filter);
    READ_VALUE(aftertouch_filter_q);
#undef READ_VALUE
}

bool MemoryEffect::pack(Block *block, size_t index) {
    if(!index) {
        Effect *dst = static_cast<EffectBlock*>(block);
        *dst = effect_;
        return true;
    }
    return false;
}

void MemoryEffect::print(XmlPrinter &p) {
#define PRINT_VALUE(name_) \
    print_value(p, #name_, effect_.name_)

    p.OpenElement(EFFECT_TAGNAME.c_str());
    PRINT_VALUE(pitchbend_depth);
    PRINT_VALUE(master_volume);
    PRINT_VALUE(sustain_switch);
    PRINT_VALUE(modulation_lfo_pitch);
    PRINT_VALUE(modulation_lfo_amplitude);
    PRINT_VALUE(modulation_lfo_filter);
    PRINT_VALUE(modulation_lfo_filter_q);
    PRINT_VALUE(modulation_filter);
    PRINT_VALUE(modulation_amplitude);
    PRINT_VALUE(modulation_filter_q);
    PRINT_VALUE(footvolume_lfo_pitch);
    PRINT_VALUE(footvolume_lfo_amplitude);
    PRINT_VALUE(footvolume_lfo_filter);
    PRINT_VALUE(footvolume_lfo_filter_q);
    PRINT_VALUE(footvolume_amplitude);
    PRINT_VALUE(footvolume_filter);
    PRINT_VALUE(footvolume_filter_q);
    PRINT_VALUE(aftertouch_lfo_pitch);
    PRINT_VALUE(aftertouch_lfo_amplitude);
    PRINT_VALUE(aftertouch_lfo_filter);
    PRINT_VALUE(aftertouch_lfo_filter_q);
    PRINT_VALUE(aftertouch_amplitude);
    PRINT_VALUE(aftertouch_filter);
    PRINT_VALUE(aftertouch_filter_q);
    p.CloseElement();
#undef PRINT_VALUE
}


//------------------------------------------------------------------------------
// MemoryVoice

template<typename U>
std::shared_ptr<MemoryVoice> MemoryVoice::create(
    const U &u, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryVoice>(u, prev);
}

MemoryVoice::MemoryVoice(Lock, const XmlElement &element, MemoryObjectPtr prev):
    MemoryObject(prev) {
#define READ_VALUE(name_) \
    read_value(element, #name_, voice_.name_)
#define READ_UNSIGNED_VALUE(name_) \
    read_unsigned_value(element, #name_, voice_.name_)
#define READ_VALUE_ARRAY(name_) \
    read_value_array(element, #name_, 8, voice_.name_)

    const char *name = nullptr;
    element.QueryStringAttribute("name", &name);
    memcpy(voice_.name, name, 14);
    voice_.name[12] = 0;
    voice_.name[13] = 0;

    read_unsigned_value(element, "index", index_);

    READ_VALUE(data_start);
    READ_VALUE(data_end);
    READ_VALUE(play_start);
    READ_VALUE(play_end);
    READ_VALUE(loop);
    READ_VALUE(loop_sustain_point);
    READ_VALUE(loop_end_point);
    READ_VALUE(pitch_correction);
    READ_VALUE(filter);
    READ_VALUE(filter_q);
    READ_VALUE(dca_sustain);
    READ_VALUE(dca_end);
    READ_VALUE(dcf_sustain);
    READ_VALUE(dcf_end);
    READ_UNSIGNED_VALUE(lfo_delay);
    READ_UNSIGNED_VALUE(lfo_name);
    READ_UNSIGNED_VALUE(lfo_attack);
    READ_VALUE(lfo_rate);
    READ_VALUE(lfo_pitch);
    READ_VALUE(lfo_amplitude);
    READ_VALUE(lfo_filter);
    READ_VALUE(lfo_filter_q);
    READ_VALUE(velocity_filter_q_key_follow);
    READ_VALUE(amplitude_key_follow);
    READ_VALUE(amplitude_rate_key_follow);
    READ_VALUE(filter_key_follow);
    READ_VALUE(filter_rate_key_follow);
    READ_VALUE(velocity_amplitude_key_follow);
    READ_VALUE(velocity_amplitude_rate_key_follow);
    READ_VALUE(velocity_filter_key_follow);
    READ_VALUE(velocity_filter_rate_key_follow);
    READ_UNSIGNED_VALUE(midi_hi);
    READ_UNSIGNED_VALUE(midi_lo);
    READ_UNSIGNED_VALUE(midi_origin);
    READ_UNSIGNED_VALUE(frequency);
    READ_VALUE_ARRAY(loop_start);
    READ_VALUE_ARRAY(loop_end);
    READ_VALUE_ARRAY(loop_xfade_time);
    READ_VALUE_ARRAY(loop_time);
    READ_VALUE_ARRAY(dca_rate);
    READ_VALUE_ARRAY(dca_end_level);
    READ_VALUE_ARRAY(dcf_rate);
    READ_VALUE_ARRAY(dcf_end_level);
#undef READ_VALUE_ARRAY
#undef READ_VALUE
}

bool MemoryVoice::pack(Block *block, size_t index) {
    if(index < 4) {
        auto *dst = static_cast<VoiceBlock*>(block);
        (*dst)[index] = voice_;
        return true;
    }
    return false;
}

void MemoryVoice::print(XmlPrinter &p) {
#define PRINT_VALUE(name_) \
    print_value(p, #name_, voice_.name_)
#define PRINT_VALUE_ARRAY(name_) \
    print_value_array(p, #name_, 8, voice_.name_)

    p.OpenElement(VOICE_TAGNAME.c_str());
    p.PushAttribute("name", voice_.name);
    p.PushAttribute("index", index_);
    PRINT_VALUE(data_start);
    PRINT_VALUE(data_end);
    PRINT_VALUE(play_start);
    PRINT_VALUE(play_end);
    PRINT_VALUE(loop);
    PRINT_VALUE(loop_sustain_point);
    PRINT_VALUE(loop_end_point);
    PRINT_VALUE(pitch_correction);
    PRINT_VALUE(filter);
    PRINT_VALUE(filter_q);
    PRINT_VALUE(dca_sustain);
    PRINT_VALUE(dca_end);
    PRINT_VALUE(dcf_sustain);
    PRINT_VALUE(dcf_end);
    PRINT_VALUE(lfo_delay);
    PRINT_VALUE(lfo_name);
    PRINT_VALUE(lfo_attack);
    PRINT_VALUE(lfo_rate);
    PRINT_VALUE(lfo_pitch);
    PRINT_VALUE(lfo_amplitude);
    PRINT_VALUE(lfo_filter);
    PRINT_VALUE(lfo_filter_q);
    PRINT_VALUE(velocity_filter_q_key_follow);
    PRINT_VALUE(amplitude_key_follow);
    PRINT_VALUE(amplitude_rate_key_follow);
    PRINT_VALUE(filter_key_follow);
    PRINT_VALUE(filter_rate_key_follow);
    PRINT_VALUE(velocity_amplitude_key_follow);
    PRINT_VALUE(velocity_amplitude_rate_key_follow);
    PRINT_VALUE(velocity_filter_key_follow);
    PRINT_VALUE(velocity_filter_rate_key_follow);
    PRINT_VALUE(midi_hi);
    PRINT_VALUE(midi_lo);
    PRINT_VALUE(midi_origin);
    PRINT_VALUE(frequency);
    PRINT_VALUE_ARRAY(loop_start);
    PRINT_VALUE_ARRAY(loop_end);
    PRINT_VALUE_ARRAY(loop_xfade_time);
    PRINT_VALUE_ARRAY(loop_time);
    PRINT_VALUE_ARRAY(dca_rate);
    PRINT_VALUE_ARRAY(dca_end_level);
    PRINT_VALUE_ARRAY(dcf_rate);
    PRINT_VALUE_ARRAY(dcf_end_level);
    p.CloseElement();
#undef PRINT_VALUE_ARRAY
#undef PRINT_VALUE
}


//------------------------------------------------------------------------------
// MemoryWave

template<typename U>
std::shared_ptr<MemoryWave> MemoryWave::create(
    const U &u, MemoryObjectPtr prev) {
    return MemoryObject::create<MemoryWave>(u, prev);
}

MemoryWave::MemoryWave(Lock, const XmlElement &element, MemoryObjectPtr prev):
    MemoryObject(prev) {

    read_unsigned_value(element, "index", index_);
    const char
        *text = element.GetText(),
        *current = text;
    size_t index = 0;
    while(current && index < 512) {
        while(isspace(*current)) { current++; }
        char tmp[5] = {0};
        memcpy(tmp, current, 4);
        uint16_t sample = strtoul(tmp, nullptr, 16);
        wave_.samples[index++] = static_cast<int16_t>(sample);
        current += 4;
    }
}

bool MemoryWave::pack(Block *block, size_t index) {
    if(!index) {
        Wave *dst = static_cast<WaveBlock*>(block);
        *dst = wave_;
        return true;
    }
    return false;
}

void MemoryWave::print(XmlPrinter &p) {
    p.OpenElement(WAVE_TAGNAME.c_str());
    p.PushAttribute("index", index_);
    char buffer[2822] = { '\n' };
    char *ptr = buffer + 1;

    for(size_t i = 0; i < 32; i++) {
        snprintf(ptr, 9, "        ");
        ptr += 8;
        for(size_t j = 0; j < 16; j++) {
            uint16_t sample = wave_.samples[(i * 16) + j];
            snprintf(ptr, 6, "%04x%c", sample, (j < 15) ? ' ' : '\n' );
            ptr += 5;
        }
    }

    snprintf(ptr, 5, "    ");
    ptr += 4;
    assert(ptr - buffer == 2821);
    p.PushText(buffer);
    p.CloseElement();
}

Result MemoryWave::dump_wav(
    std::string_view filename, SampleRate freq, size_t offset, size_t count) {

    int32_t samplerate = 0;
    switch(freq) {
        case SR_36kHz: samplerate = 36000; break;
        case SR_18kHz: samplerate = 18000; break;
        case SR_9kHz: samplerate = 9000; break;
        default: return RESULT_WAVE_BAD_SAMPLERATE;
    }
    assert(samplerate);

    auto iter = shared_from_this();
    while((offset >= 512) && iter) {
        offset -= 512;
        iter = iter->next();
        if(!iter && (offset >= 512)) {
            return RESULT_WAVE_BAD_OFFSET;
        }
    }

    TinyWav tw;
    float float_buffer[512];
    int r = tinywav_open_write(
        &tw, 1, samplerate, TW_FLOAT32, TW_INTERLEAVED, filename.data());
    if(r) {
        return RESULT_WAVE_OPEN_ERROR;
    }
    while(iter && count) {
        size_t len = 0;
        if(offset < 512) {
            len = 512 - offset;
        }
        if(len > count) {
            len = count;
        }
        auto *wave = iter->wave();
        assert(wave);
        for(size_t i = 0; i < len; i++) {
            float_buffer[i] = wave->samples[i + offset] / 32768.f;
        }

        int samples = tinywav_write_f(&tw, float_buffer, len);
        if(samples != static_cast<int>(len)) {
            return RESULT_WAVE_WRITE_ERROR;
        }
        count -= len;
        offset = 0;
        iter = iter->next();
    }
    tinywav_close_write(&tw);
    return RESULT_OK;
}

//------------------------------------------------------------------------------
// Loader

Result Loader::flag_check(uint8_t flags) {
    if(flags & FILE_OPEN_ERROR) {
        return RESULT_FILE_OPEN_ERROR;
    }
    if(flags & FILE_READ_ERROR) {
        return RESULT_FILE_READ_ERROR;
    }
    if(flags & XML_PARSE_ERROR) {
        return RESULT_XML_PARSE_ERROR;
    }
    return RESULT_OK;
}


//------------------------------------------------------------------------------
// BlockLoader

BlockLoader::BlockLoader(std::string_view filename) {
    FILE *file = fopen(filename.data(), "rb");
    if(!file) {
        flags_ |= FILE_OPEN_ERROR;
        return;
    }
    FileCloser close_file(file);

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
    flags_ |= FILE_READ_ERROR;
    storage_ = nullptr;
    size_ = 0;
}

BlockLoader::BlockLoader(std::unique_ptr<uint8_t[]>&& storage, size_t size):
    storage_(std::move(storage)),
    size_(size) {}

BlockLoader::BlockLoader(const void *storage, size_t size):
    storage_(std::make_unique<uint8_t[]>(size)),
    size_(size) {
    memcpy(storage_.get(), storage, size);
}

Result BlockLoader::load(MemoryBlocks &mb) {
    mb.reset();
    if(auto r = flag_check(flags_); !result_success(r)) {
        return r;
    }
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
    int16_t blocks = size_ / 1024;
    if(blocks != h.block_count) {
        return RESULT_BAD_BLOCK_COUNT;
    }
    return mb.load(std::move(storage_), blocks);
}


//------------------------------------------------------------------------------
// XmlLoader

XmlLoader::XmlLoader(std::string_view filename) {
    FILE *file = fopen(filename.data(), "rb");
    if(!file) {
        flags_ |= FILE_OPEN_ERROR;
        return;
    }
    FileCloser close_file(file);

    fseek(file, 0, SEEK_END);
    size_t size = ftell(file);
    std::unique_ptr<char[]> storage;
    if(size) {
        rewind(file);
        storage = std::make_unique<char[]>(size);
        size_t r = fread(storage.get(), size, 1, file);
        if(r != 1) {
            flags_ |= FILE_READ_ERROR;
            return;
        }
    }
    xml_ = std::make_unique<XmlDocument>();
    auto e = xml_->Parse(storage.get(), size);
    if(e != tinyxml2::XML_SUCCESS) {
        flags_ |= XML_PARSE_ERROR;
    }
}

XmlLoader::XmlLoader(std::unique_ptr<XmlDocument> &&xml):
    xml_(std::move(xml)) {}

XmlLoader::XmlLoader(const XmlDocument &xml):
    xml_(std::make_unique<XmlDocument>()) {
    xml.DeepCopy(xml_.get());
}

XmlLoader::~XmlLoader() = default;

Result XmlLoader::load(MemoryObjectPtr &objects, FzFileType *file_type) {
    objects.reset();
    if(file_type) {
        *file_type = TYPE_UNKNOWN;
    }
    if(auto r = flag_check(flags_); !result_success(r)) {
        return r;
    }
    if(!xml_) {
        return RESULT_XML_EMPTY;
    }
    auto *root = xml_->RootElement();
    if(!root) {
        return RESULT_XML_MISSING_ROOT;
    }
    if(FZ_ML_ROOT_NAME != root->Name()) {
        return RESULT_XML_UNKNOWN_ROOT_ELEMENT;
    }
    auto *attr = root->FindAttribute("version");
    if(!attr) {
        return RESULT_XML_MISSING_VERSION;
    }
    const char *version = attr->Value();
    if(FZ_ML_VERSION != version) {
        return RESULT_XML_UNKNOWN_VERSION;
    }
    auto *type = root->FindAttribute("file_type");
    if(!type) {
        return RESULT_XML_MISSING_FILE_TYPE;
    }
    if(file_type) {
        FzFileType ft = static_cast<FzFileType>( atoi(type->Value()) );
        if(ft <= TYPE_EFFECT) {
            *file_type = ft;
        }
    }
    auto *element = root->FirstChildElement();
    if(!element) {
        return RESULT_XML_MISSING_CHILDREN;
    }
    MemoryObjectPtr
        current,
        first;
    while(element) {
        if(BANK_TAGNAME == element->Name()) {
            current = MemoryBank::create(*element, current);
        } else if(EFFECT_TAGNAME == element->Name()) {
            current = MemoryEffect::create(*element, current);
        } else if(VOICE_TAGNAME == element->Name()) {
            current = MemoryVoice::create(*element, current);
        } else if(WAVE_TAGNAME == element->Name()) {
            current = MemoryWave::create(*element, current);
        } else {
            return RESULT_XML_UNKNOWN_ELEMENT;
        }
        if(!first) { first = current; }
        element = element->NextSiblingElement();
    }
    objects = first;
    return RESULT_OK;
}


//------------------------------------------------------------------------------
// BlockDumper

Result BlockDumper::dump(const MemoryBlocks &blocks, size_t *write_size) {
    if(destination_) {
        return memory_dump(blocks, write_size);
    } else if(!filename_.empty()) {
        return file_dump(blocks, write_size);
    }
    return RESULT_UNINITIALIZED_DUMPER;
}

Result BlockDumper::memory_dump(const MemoryBlocks &blocks, size_t *write_size) {
    assert(destination_);
    if(write_size) {
        *write_size = 0;
    }
    size_t copy_size = blocks.count() * 1024;
    if(size_ < copy_size) {
        return RESULT_MEMORY_TOO_SMALL;
    }
    memcpy(destination_, blocks.block(0), copy_size);
    if(write_size) {
        *write_size = copy_size;
    }
    return RESULT_OK;
}

Result BlockDumper::file_dump(const MemoryBlocks &blocks, size_t *write_size) {
    assert(!filename_.empty());
    if(write_size) {
        *write_size = 0;
    }
    FILE *file = fopen(filename_.data(), "wb");
    if(!file) {
        return RESULT_FILE_OPEN_ERROR;
    }
    FileCloser close_file(file);

    size_t
        copy_size = blocks.count() * 1024,
        w = fwrite(blocks.block(0), copy_size, 1, file);

    if(w != 1) {
        return RESULT_FILE_WRITE_ERROR;
    }
    if(write_size) {
        *write_size = copy_size;
    }
    return RESULT_OK;
}


//------------------------------------------------------------------------------
// XmlDumper

Result XmlDumper::dump(const MemoryObjectPtr objects, size_t *write_size) {
    if(destination_) {
        return memory_dump(objects, write_size);
    } else if(!filename_.empty()) {
        return file_dump(objects, write_size);
    }
    return RESULT_UNINITIALIZED_DUMPER;
}

Result XmlDumper::memory_dump(const MemoryObjectPtr objects, size_t *write_size) {
    assert(destination_);
    if(write_size) {
        *write_size = 0;
    }
    XmlPrinter printer(nullptr, true);
    print(objects, printer);
    size_t copy_size = printer.CStrSize();
    // Note that in this case we write the copy_size to the output pointer before
    // making the attempt to copy. This is so the caller will know how big of a
    // buffer is needed on the next attempt...
    if(write_size) {
        *write_size = copy_size;
    }
    if(size_ < copy_size) {
        // yielding the truncated output is probably less of a performance hit
        // than re-running the XmlPrinter, so perhaps it's useful to some users...
        memcpy(destination_, printer.CStr(), size_);
        return RESULT_MEMORY_TOO_SMALL;
    }
    memcpy(destination_, printer.CStr(), copy_size);
    return RESULT_OK;
}

Result XmlDumper::file_dump(const MemoryObjectPtr objects, size_t *write_size) {
    assert(!filename_.empty());
    if(write_size) {
        *write_size = 0;
    }
    FILE *file = fopen(filename_.data(), "w");
    if(!file) {
        return RESULT_FILE_OPEN_ERROR;
    }
    FileCloser close_file(file);

    XmlPrinter printer(file);
    print(objects, printer);
    size_t
        copy_size = printer.CStrSize(),
        w = fwrite(printer.CStr(), copy_size, 1, file);

    if(w != 1) {
        return RESULT_FILE_WRITE_ERROR;
    }
    if(write_size) {
        *write_size = copy_size;
    }
    return RESULT_OK;
}


void XmlDumper::print(const MemoryObjectPtr objects, XmlPrinter &p) {
    p.OpenElement(FZ_ML_ROOT_NAME.c_str());
    p.PushAttribute("version", FZ_ML_VERSION.c_str());
    p.PushAttribute("file_type", file_type_);
    auto o = objects;
    while(o) {
        o->print(p);
        o = o->next();
    }
    p.CloseElement();
}

} // Casio::FZ_1::API
