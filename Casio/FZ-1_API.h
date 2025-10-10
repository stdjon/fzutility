#ifndef CASIO_FZ_1_API
#define CASIO_FZ_1_API

#include "Casio/FZ-1.h"
#include <memory>
#include <string>
#include <string_view>

namespace tinyxml2 {
class XMLDocument;
class XMLElement;
class XMLPrinter;
}

namespace Casio::FZ_1::API {

using MemoryObjectPtr = std::shared_ptr<struct MemoryObject>;
using XmlDocument = tinyxml2::XMLDocument;
using XmlElement = tinyxml2::XMLElement;
using XmlPrinter = tinyxml2::XMLPrinter;

//------------------------------------------------------------------------------
// Result codes

#define FZ_EACH_RESULT(_) \
    _(RESULT_OK) \
     \
    _(RESULT_BAD_BLOCK) \
    _(RESULT_BAD_BLOCK_COUNT) \
    _(RESULT_BAD_BLOCK_INDEX) \
    _(RESULT_BAD_EFFECT_BLOCK_COUNT) \
    _(RESULT_BAD_FILE_SIZE) \
    _(RESULT_BAD_FILE_VERSION) \
    _(RESULT_BAD_HEADER) \
    _(RESULT_FILE_OPEN_ERROR) \
    _(RESULT_FILE_READ_ERROR) \
    _(RESULT_FILE_WRITE_ERROR) \
    _(RESULT_MEMORY_TOO_SMALL) \
    _(RESULT_MISMATCHED_BANK_BLOCK) \
    _(RESULT_MISMATCHED_BLOCK_COUNT) \
    _(RESULT_MISMATCHED_VOICE_BLOCK) \
    _(RESULT_MISMATCHED_WAVE_BLOCK) \
    _(RESULT_MISSING_BANK) \
    _(RESULT_MISSING_VOICE) \
    _(RESULT_MISSING_WAVE) \
    _(RESULT_NO_BLOCKS) \
    _(RESULT_UNINITIALIZED_DUMPER) \
    _(RESULT_WAVE_BAD_OFFSET) \
    _(RESULT_WAVE_BAD_SAMPLERATE) \
    _(RESULT_WAVE_OPEN_ERROR) \
    _(RESULT_WAVE_WRITE_ERROR) \
    _(RESULT_XML_EMPTY) \
    _(RESULT_XML_MISSING_CHILDREN) \
    _(RESULT_XML_MISSING_FILE_TYPE) \
    _(RESULT_XML_MISSING_ROOT) \
    _(RESULT_XML_MISSING_VERSION) \
    _(RESULT_XML_PARSE_ERROR) \
    _(RESULT_XML_UNKNOWN_ELEMENT) \
    _(RESULT_XML_UNKNOWN_VERSION) \
    _(RESULT_XML_UNKNOWN_ROOT_ELEMENT) \
     \
    _(RESULT_END_OF_LIST_)

#define FZ_ENUM_RESULT(name_) name_,

enum Result: uint32_t {
    FZ_EACH_RESULT(FZ_ENUM_RESULT)
};

// Return a string representation of a Result code (for convenience when error
// reporting/debugging)
const char *result_str(Result r);

// Reports whether a result indicates success. (Don't just assume that RESULT_OK
// will always be the only ever success value...)
bool result_success(Result r);


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
    // so there will not be 1:1 correspondences in those cases.
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
// NB: a list of MemoryObjects is held strongly from the head of the list, so if
// only one reference exists, this will result in objects being deleted if the
// reference it is advanced (via e.g. o = o->next();).
// This is a safer alternative:
//    MemoryObject first = ...; // permanent link to initial object
//    for(auto o = first; o; o = o->next()) {
//        ... // process each object 'o' leaving 'first' untouched
//    }
struct MemoryObject: std::enable_shared_from_this<MemoryObject> {
    template<typename T, typename U>
    static auto create(const U &u, MemoryObjectPtr prev);

    virtual ~MemoryObject() = default;
    virtual BlockType type() { return BT_NONE; }
    size_t index() { return index_; }

    MemoryObjectPtr prev() { return prev_.lock(); }
    MemoryObjectPtr next() { return next_; }

    MemoryObjectPtr insert_after(MemoryObjectPtr obj);
    MemoryObjectPtr insert_before(MemoryObjectPtr obj);

    // Only one of these will return non-null for any given object
    virtual Bank *bank() { return nullptr; }
    virtual Effect *effect() { return nullptr; }
    virtual Voice *voice() { return nullptr; }
    virtual Wave *wave() { return nullptr; }

    // Pack memory object list back into a contiguous array of blocks
    static Result pack(
        MemoryObjectPtr in, MemoryBlocks &out, FzFileType type = TYPE_FULL);

protected:
    // This restricts access of derived class constructors (which must be public
    // so that std::make_shared<T>() can access them).
    struct Lock {};

    MemoryObject(MemoryObjectPtr prev): prev_(prev) {}
    virtual bool pack(Block *block, size_t index) { return false; }
    virtual void print(XmlPrinter &printer) {}

    size_t index_ = 0;

private:
    void link(const MemoryObjectPtr &next) {
        next_ = next;
        if(next_->type() == type()) {
            next_->index_ = index_ + 1;
        }
    }

    std::weak_ptr<MemoryObject> prev_;
    MemoryObjectPtr next_;

    friend class XmlDumper;
    friend class XmlLoader;
};


//------------------------------------------------------------------------------
// MemoryBank

struct MemoryBank: MemoryObject {
    template<typename U>
    static std::shared_ptr<MemoryBank> create(
        const U &u, MemoryObjectPtr prev = nullptr);

    MemoryBank(Lock, const Bank &bank, MemoryObjectPtr prev):
        MemoryObject(prev), bank_(bank) {}
    MemoryBank(Lock, const XmlElement &element, MemoryObjectPtr prev);

    BlockType type() override { return BT_BANK; }
    Bank *bank() override { return &bank_; }

protected:
    bool pack(Block *block, size_t index) override;
    void print(XmlPrinter &printer) override;

private:
    Bank bank_;
};


//------------------------------------------------------------------------------
// MemoryEffect

struct MemoryEffect: MemoryObject {
    template<typename U>
    static std::shared_ptr<MemoryEffect> create(
        const U &u, MemoryObjectPtr prev = nullptr);

    MemoryEffect(Lock, const Effect &effect, MemoryObjectPtr prev):
        MemoryObject(prev), effect_(effect) {}
    MemoryEffect(Lock, const XmlElement &element, MemoryObjectPtr prev);

    BlockType type() override { return BT_EFFECT; }
    Effect *effect() override { return &effect_; }

protected:
    bool pack(Block *block, size_t index) override;
    void print(XmlPrinter &printer) override;

private:
    Effect effect_;
};


//------------------------------------------------------------------------------
// MemoryVoice

struct MemoryVoice: MemoryObject {
    template<typename U>
    static std::shared_ptr<MemoryVoice> create(
        const U &u, MemoryObjectPtr prev = nullptr);

    MemoryVoice(Lock, const Voice &voice, MemoryObjectPtr prev):
        MemoryObject(prev), voice_(voice) {}
    MemoryVoice(Lock, const XmlElement &element, MemoryObjectPtr prev);

    BlockType type() override { return BT_VOICE; }
    Voice *voice() override { return &voice_; }

protected:
    bool pack(Block *block, size_t index) override;
    void print(XmlPrinter &printer) override;

private:
    Voice voice_;
};


//------------------------------------------------------------------------------
// MemoryWave

struct MemoryWave: MemoryObject {
    template<typename U>
    static std::shared_ptr<MemoryWave> create(
        const U &u, MemoryObjectPtr prev = nullptr);

    MemoryWave(Lock, const Wave &wave, MemoryObjectPtr prev):
        MemoryObject(prev), wave_(wave) {}
    MemoryWave(Lock, const XmlElement &element, MemoryObjectPtr prev);

    BlockType type() override { return BT_WAVE; }
    Wave *wave() override { return &wave_; }

    // ** Interim API: subject to change! **
    // Dump some or all of the wave data in the range specified (in samples):
    //   [offset, offset + count)
    // If the offset and/or count is longer than the current block and more
    // WaveBlocks are available, these will be concatenated as needed.
    // freq = [0, 1, 2] as per definition in Voice::frequency
    Result dump_wav(
        std::string_view filename, uint8_t freq, size_t offset, size_t count);

protected:
    bool pack(Block *block, size_t index) override;
    void print(XmlPrinter &printer) override;

private:
    Wave wave_;
};


//------------------------------------------------------------------------------
// Loader

struct Loader {
protected:
    enum Flags {
        FILE_OPEN_ERROR = 0x01,
        FILE_READ_ERROR = 0x02,
        XML_PARSE_ERROR = 0x04,
    };
    static Result flag_check(uint8_t flags);
};


//------------------------------------------------------------------------------
// BlockLoader

struct BlockLoader: Loader {
    BlockLoader(std::string_view filename);
    BlockLoader(std::unique_ptr<uint8_t[]>&& storage, size_t size);
    BlockLoader(const void *storage, size_t size);
    template<size_t N>BlockLoader(const uint8_t (&storage)[N]);

    Result load(MemoryBlocks &blocks);

private:
    std::unique_ptr<uint8_t[]> storage_;
    size_t size_ = 0;
    uint8_t flags_ = 0;
};

template<size_t N>BlockLoader::BlockLoader(const uint8_t (&storage)[N]):
    BlockLoader(storage, N) {}


//------------------------------------------------------------------------------
// XmlLoader

struct XmlLoader: Loader {
    XmlLoader(std::string_view filename);
    XmlLoader(std::unique_ptr<XmlDocument> &&xml);
    XmlLoader(const XmlDocument &xml);
    ~XmlLoader();

    Result load(MemoryObjectPtr &objects, FzFileType *file_type = nullptr);

private:
    std::unique_ptr<XmlDocument> xml_;
    uint8_t flags_ = 0;
};


//------------------------------------------------------------------------------
// Dumper

struct Dumper {
protected:
    Dumper(std::string_view filename): filename_(filename) {}
    Dumper(void *storage, size_t size): destination_(storage), size_(size) {}

    std::string filename_;
    void *destination_ = nullptr;
    size_t size_ = 0;
};


//------------------------------------------------------------------------------
// BlockDumper

struct BlockDumper: Dumper {
    BlockDumper(std::string_view filename): Dumper(filename) {}
    BlockDumper(void *storage, size_t size): Dumper(storage, size) {}
    template<size_t N>BlockDumper(uint8_t (&storage)[N]);

    Result dump(const MemoryBlocks &blocks, size_t *write_size = nullptr);

private:
    Result memory_dump(const MemoryBlocks &blocks, size_t *write_size);
    Result file_dump(const MemoryBlocks &blocks, size_t *write_size);
};

template<size_t N>BlockDumper::BlockDumper(uint8_t (&storage)[N]):
    BlockDumper(storage, N) {}


//------------------------------------------------------------------------------
// XmlDumper

struct XmlDumper: Dumper {
    XmlDumper(std::string_view filename, FzFileType file_type):
        Dumper(filename), file_type_(file_type) {}
    XmlDumper(void *storage, size_t size, FzFileType file_type):
        Dumper(storage, size), file_type_(file_type) {}
    template<size_t N>XmlDumper(uint8_t (&storage)[N], FzFileType file_type);
    ~XmlDumper() = default;

    Result dump(const MemoryObjectPtr objects, size_t *write_size = nullptr);

private:
    Result memory_dump(const MemoryObjectPtr objects, size_t *write_size);
    Result file_dump(const MemoryObjectPtr objects, size_t *write_size);
    void print(const MemoryObjectPtr objects, XmlPrinter &printer);

    FzFileType file_type_ = TYPE_UNKNOWN;
};

template<size_t N>XmlDumper::XmlDumper(uint8_t (&storage)[N], FzFileType file_type):
    XmlDumper(storage, N, file_type) {}


} //Casio::FZ_1::API

#endif //CASIO_FZ_1_API
