#ifndef CASIO_FZ_1_API
#define CASIO_FZ_1_API

#include "Casio/FZ-1.h"
#include <memory>

namespace Casio::FZ_1::API {

//------------------------------------------------------------------------------
// Result codes

enum Result {
    RESULT_OK,

    RESULT_BAD_BLOCK_COUNT,
    RESULT_BAD_HEADER,
    RESULT_BAD_FILE_SIZE,
    RESULT_BAD_FILE_VERSION,
    RESULT_NO_BLOCKS,
    RESULT_BANK_BLOCK_MISMATCH,
    RESULT_VOICE_BLOCK_MISMATCH,
    RESULT_WAVE_BLOCK_MISMATCH,
    RESULT_BLOCK_COUNT_MISMATCH,
};


//------------------------------------------------------------------------------
// BlockType

enum BlockType: uint8_t {
    BT_BANK,
    BT_EFFECT,
    BT_VOICE,
    BT_WAVE,
    BT_NONE,
};


//------------------------------------------------------------------------------
// MemoryBlocks

// Models a contiguous array of Blocks plus some sanity-checking logic
struct MemoryBlocks {
    // Low-level access to a specific block by index into the block array.
    // For specific block types, if the block at the given index does not match
    // the requested type, a null pointer is returned.
    Block *block(size_t n) const;
    BankBlock *bank_block(size_t n) const;
    VoiceBlock *voice_block(size_t n) const;
    EffectBlock *effect_block(size_t n) const;
    WaveBlock *wave_block(size_t n) const;

    // Access file header and/or the block that contains it
    // For specific header types, if block zero does not match the requested
    // type, a null pointer is returned.
    FzFileHeader *header() const;
    BankBlockFileHeader *bank_header() const;
    VoiceBlockFileHeader *voice_header() const;
    EffectBlockFileHeader *effect_header() const;

    // Access banks/voices/waves by index: a bank address will usually match its
    // corresponding block but multiple voices can coexist inside a VoiceBlock,
    // and Waves are usually found after a non-zero amount of bank/voice blocks
    Bank *bank(size_t n) const;
    Voice *voice(size_t n) const;
    Wave *wave(size_t n) const;

    size_t count() const { return count_; }
    FzFileType file_type() const { return file_type_; }

    BlockType block_type(size_t n) const;
    char block_type_as_char(size_t n) const;

    bool is_empty() const { return !count_; }
    explicit operator bool() const { return !is_empty(); }

    void reset();

private:
    void *block_data(size_t n) const;
    Result load(std::unique_ptr<uint8_t[]> &&storage, size_t count);
    Result parse();

    std::unique_ptr<uint8_t[]> storage_;
    std::unique_ptr<BlockType[]> block_types_;
    void *data_ = nullptr;
    size_t count_ = 0;
    FzFileType file_type_ = TYPE_UNKNOWN;

    friend struct BlockLoader;
    friend struct BlockDumper;
};


//------------------------------------------------------------------------------

using MemoryObjectPtr = std::shared_ptr<struct MemoryObject>;

// Models independent Banks, Voices and Waves outside of a Block file array.
// These can be unpacked from MemoryBlocks, manipulated and repacked (or data can
// be saved in .xml or .wav file formats).
struct MemoryObject: std::enable_shared_from_this<MemoryObject> {
    MemoryObject(MemoryObjectPtr prev = nullptr): prev_(prev) {}

    MemoryObjectPtr prev() { return prev_.lock(); }
    MemoryObjectPtr next() { return next_; }

    void link(const MemoryObjectPtr &next) {
        next_ = next;
    }

protected:
    struct Lock {};

private:
    std::weak_ptr<MemoryObject> prev_;
    MemoryObjectPtr next_;
};


//------------------------------------------------------------------------------
// MemoryBank

struct MemoryBank: MemoryObject {
    static std::shared_ptr<MemoryBank> create(
        const Bank &bank, MemoryObjectPtr prev = nullptr);

    MemoryBank(Lock, const Bank &bank, MemoryObjectPtr prev = nullptr):
        MemoryObject(prev), bank_(bank_) {}

private:
    Bank bank_;
};


//------------------------------------------------------------------------------
// MemoryEffect

struct MemoryEffect: MemoryObject {
    static std::shared_ptr<MemoryEffect> create(
        const Effect &effect, MemoryObjectPtr prev = nullptr);

    MemoryEffect(Lock, const Effect &effect, MemoryObjectPtr prev = nullptr):
        MemoryObject(prev), effect_(effect) {}

private:
    Effect effect_;
};


//------------------------------------------------------------------------------
// MemoryVoice

struct MemoryVoice: MemoryObject {
    static std::shared_ptr<MemoryVoice> create(
        const Voice &voice, MemoryObjectPtr prev = nullptr);

    MemoryVoice(Lock, const Voice &voice, MemoryObjectPtr prev = nullptr):
        MemoryObject(prev), voice_(voice) {}

private:
    Voice voice_;
};


//------------------------------------------------------------------------------
// MemoryWave

struct MemoryWave: MemoryObject {
    static std::shared_ptr<MemoryWave> create(
        const Wave &wave, MemoryObjectPtr prev = nullptr);

    MemoryWave(Lock, const Wave &wave, MemoryObjectPtr prev = nullptr):
        MemoryObject(prev), wave_(wave) {}

private:
    Wave wave_;
};


//------------------------------------------------------------------------------
// BlockLoader

struct BlockLoader {
    BlockLoader(std::string_view filename);
    BlockLoader(std::unique_ptr<uint8_t[]>&& storage, size_t size);
    BlockLoader(void *storage, size_t size);
    template<size_t N>BlockLoader(uint8_t storage[N]);
    Result load(MemoryBlocks &blocksOut);
private:
    std::unique_ptr<uint8_t[]> storage_;
    size_t size_ = 0;
};

template<size_t N>BlockLoader::BlockLoader(uint8_t storage[N]):
    BlockLoader(storage, N) {}

//------------------------------------------------------------------------------
// BlockDumper

struct BlockDumper {
    BlockDumper(std::string_view filename);
    Result add_block(Block *block);
};

} //Casio::FZ_1::API

#endif //CASIO_FZ_1_API
