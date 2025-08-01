#ifndef CASIO_FZ_1_API
#define CASIO_FZ_1_API

#include "Casio/FZ-1.h"
#include <memory>

namespace Casio::FZ_1::API {

using MemoryObjectPtr = std::shared_ptr<struct MemoryObject>;

//------------------------------------------------------------------------------
// Result codes

enum Result {
    RESULT_OK,

    RESULT_BAD_BLOCK,
    RESULT_BAD_BLOCK_COUNT,
    RESULT_BAD_BLOCK_INDEX,
    RESULT_BAD_EFFECT_BLOCK_COUNT,
    RESULT_BAD_FILE_SIZE,
    RESULT_BAD_FILE_VERSION,
    RESULT_BAD_HEADER,
    RESULT_FILE_OPEN_ERROR,
    RESULT_FILE_READ_ERROR,
    RESULT_FILE_WRITE_ERROR,
    RESULT_MEMORY_TOO_SMALL,
    RESULT_MISMATCHED_BANK_BLOCK,
    RESULT_MISMATCHED_BLOCK_COUNT,
    RESULT_MISMATCHED_VOICE_BLOCK,
    RESULT_MISMATCHED_WAVE_BLOCK,
    RESULT_MISSING_BANK,
    RESULT_MISSING_VOICE,
    RESULT_MISSING_WAVE,
    RESULT_NO_BLOCKS,
    RESULT_UNINITIALIZED_DUMPER,
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

// Models a contiguous array of Blocks plus some sanity-checking logic.
// Any pointers returned from methods will remain valid until the MemoryBlocks
// object is destroyed.
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

    // unpack block array into a list of MemoryObjects
    Result unpack(MemoryObjectPtr& mo);

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
    friend struct MemoryObject;
};


//------------------------------------------------------------------------------
// MemoryObject

// Models independent Banks, Voices and Waves outside of a Block file array.
// These can be unpacked from MemoryBlocks, manipulated and repacked (or data can
// be saved in .xml or .wav file formats).
struct MemoryObject: std::enable_shared_from_this<MemoryObject> {
    template<typename T, typename U>
    static auto create(U &u, MemoryObjectPtr prev);

    virtual ~MemoryObject() = default;
    virtual BlockType type() { return BT_NONE; }

    MemoryObjectPtr prev() { return prev_.lock(); }
    MemoryObjectPtr next() { return next_; }

    MemoryObjectPtr insert_after(MemoryObjectPtr obj);
    MemoryObjectPtr insert_before(MemoryObjectPtr obj);

    static Result pack(
        MemoryObjectPtr in, MemoryBlocks &out, FzFileType type = TYPE_FULL);

protected:
    struct Lock {};
    MemoryObject(MemoryObjectPtr prev): prev_(prev) {}
    virtual bool pack(Block *block, size_t index) { return false; }

private:
    void link(const MemoryObjectPtr &next) {
        next_ = next;
    }

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

    BlockType type() override { return BT_BANK; }

protected:
    bool pack(Block *block, size_t index) override;

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

    BlockType type() override { return BT_EFFECT; }

protected:
    bool pack(Block *block, size_t index) override;

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

    BlockType type() override { return BT_VOICE; }

protected:
    bool pack(Block *block, size_t index) override;

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

    BlockType type() override { return BT_WAVE; }

protected:
    bool pack(Block *block, size_t index) override;

private:
    Wave wave_;
};


//------------------------------------------------------------------------------
// BlockLoader

struct BlockLoader {
    BlockLoader(std::string_view filename);
    BlockLoader(std::unique_ptr<uint8_t[]>&& storage, size_t size);
    BlockLoader(void *storage, size_t size);
    template<size_t N>BlockLoader(uint8_t (&storage)[N]);

    Result load(MemoryBlocks &blocks);

private:
    enum Flags {
        FILE_OPEN_ERROR = 0x01,
        FILE_READ_ERROR = 0x02,
    };

    std::unique_ptr<uint8_t[]> storage_;
    size_t size_ = 0;
    uint8_t flags_ = 0;
};

template<size_t N>BlockLoader::BlockLoader(uint8_t (&storage)[N]):
    BlockLoader(storage, N) {}


//------------------------------------------------------------------------------
// BlockDumper

struct BlockDumper {
    BlockDumper(std::string_view filename): filename_(filename) {}
    BlockDumper(void *storage, size_t size):
        destination_(storage), size_(size) {}
    template<size_t N>BlockDumper(uint8_t (&storage)[N]);

    Result dump(const MemoryBlocks &blocks, size_t *write_size = nullptr);

private:
    Result memory_dump(const MemoryBlocks &blocks, size_t *write_size);
    Result file_dump(const MemoryBlocks &blocks, size_t *write_size);

    std::string filename_;
    void *destination_ = nullptr;
    size_t size_ = 0;
};

template<size_t N>BlockDumper::BlockDumper(uint8_t (&storage)[N]):
    BlockDumper(storage, N) {}


} //Casio::FZ_1::API

#endif //CASIO_FZ_1_API
