#ifndef CASIO_FZ_1_API
#define CASIO_FZ_1_API

#include "./FZ-1.h"
#include <memory>

namespace Casio::FZ_1::API {

enum Result {
    RESULT_OK,

    RESULT_BAD_BLOCK_COUNT,
    RESULT_BAD_HEADER,
    RESULT_BAD_FILE_SIZE,
    RESULT_NO_BLOCKS,
};

//------------------------------------------------------------------------------
// MemoryBlocks

// Models a contiguous array of Blocks plus some sanity-checking logic
struct MemoryBlocks {
    Block *block(size_t n) const;
    BankBlock *bank_block(size_t n) const;
    VoiceBlock *voice_block(size_t n) const;
    EffectBlock *effect_block(size_t n) const;
    WaveBlock *wave_block(size_t n) const;

    FzFileHeader *header() const;
    BankBlockFileHeader *bank_header() const;
    VoiceBlockFileHeader *voice_header() const;
    EffectBlockFileHeader *effect_header() const;

    size_t count() const { return count_; }
    FzFileType file_type() const { return file_type_; }

private:
    void *block_data(size_t n) const;
    void load(std::unique_ptr<uint8_t[]> &&storage, size_t count);

    std::unique_ptr<uint8_t[]> storage_;
    void *data_ = nullptr;
    size_t count_ = 0;
    FzFileType file_type_ = TYPE_UNKNOWN;

    friend struct BlockLoader;
    friend struct BlockDumper;
};

//------------------------------------------------------------------------------
// BlockLoader

struct BlockLoader {
    BlockLoader(std::string_view filename);
    Result load(MemoryBlocks &blocksOut);
private:
    std::unique_ptr<uint8_t[]> storage_;
    size_t size_ = 0;
};


//------------------------------------------------------------------------------
// BlockDumper

struct BlockDumper {
    BlockDumper(std::string_view filename);
    Result add_block(Block *block);

};


} //Casio::FZ_1::API

#endif //CASIO_FZ_1_API
