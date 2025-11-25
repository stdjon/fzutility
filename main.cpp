#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <initializer_list>
#include <string>

using namespace Casio::FZ_1;

static const std::string VERSION = "0.1.1";

//------------------------------------------------------------------------------

// ASCII case-insensitive string match
bool string_matches(const std::string &sa, const std::string &sb) {
    return std::equal(sa.begin(), sa.end(), sb.begin(), sb.end(),
        [](unsigned char ca, unsigned char cb) {
            return std::tolower(ca) == std::tolower(cb);
        });
}

bool string_matches(
    const std::string &str, std::initializer_list<std::string> strs) {
    for(auto it = strs.begin(); it != strs.end(); it++) {
        if(string_matches(str, *it)) {
            return true;
        }
    }
    return false;
}

bool string_equals(
    const std::string &str, std::initializer_list<std::string> strs) {
    for(auto it = strs.begin(); it != strs.end(); it++) {
        if(str == *it) {
            return true;
        }
    }
    return false;
}

bool file_extension_matches(
    const std::string &ext, std::initializer_list<std::string> exts) {
    return string_matches(ext, exts);
}

std::string file_extension_find(const std::string filename) {
    std::string ext;
    if(!filename.empty()) {
        if(auto it = filename.rfind('.'); it != std::string::npos) {
            ext = filename.substr(it);
        }
    }
    return ext;
}

void file_extension_replace_or_append(
    std::string &filename, const std::string &ext) {
    if(!filename.empty()) {
        if(auto it = filename.rfind('.'); it != std::string::npos) {
            size_t len = filename.size();
            filename.replace(it, len - it, ext);
        } else {
            filename += ext;
        }
    }
}

//------------------------------------------------------------------------------

FzFileType extension_to_file_type(const std::string &ext) {
    if(file_extension_matches(ext, { ".fzb" })) {
        return TYPE_BANK;
    } else if(file_extension_matches(ext, { ".fze" })) {
        return TYPE_EFFECT;
    } else if(file_extension_matches(ext, { ".fzf" })) {
        return TYPE_FULL;
    } else if(file_extension_matches(ext, { ".fzv" })) {
        return TYPE_VOICE;
    }
    return TYPE_UNKNOWN;
}

std::string file_type_to_extension(FzFileType file_type) {
    std::string ext;
    switch(file_type) {
        default: [[fallthrough]];
        case TYPE_UNKNOWN: assert(false); break;
        case TYPE_BANK: ext = ".fzb"; break;
        case TYPE_EFFECT: ext = ".fze"; break;
        case TYPE_FULL: ext = ".fzf"; break;
        case TYPE_VOICE: ext = ".fzv"; break;
    }
    return ext;
}

//------------------------------------------------------------------------------

struct Args {
    std::string
        option, // optional (for operation argument, has initial '-' stripped)
        first, // first/mandatory argument (usually an input file)
        second, // second argument (usually output file)
        third; // third argument, used by some operations
};

// Print usage info and exit successfully
[[noreturn]] void usage() {
    printf("Usage:\n\n"
        "  fzutility <input> [<output>]\n"
        "    Convert binary files to FZ-ML (or vice versa).\n"
        "  fzutility -i <input>\n"
        "    List objects/blocks contained in input file.\n"
        "  fzutility -v\n"
        "    Display version number.\n"
        "  fzutility -w <input> [<range>] [<output>]\n"
        "    Extract wav data from binary or FZ-ML files.\n");
    exit(EXIT_SUCCESS);
}

// Print an error message and exit
[[noreturn]] void fail(const char *const fmt, ...) {
    va_list va;
    va_start(va, fmt);
    vprintf(fmt, va);
    va_end(va);
    exit(EXIT_FAILURE);
}

void check_result(API::Result result) {
    if(!API::result_success(result)) {
        fail("API error: %s\n  %s",
            API::result_string(result), API::result_description(result));
    }
}

bool parse_range(const std::string &range, size_t &start, int32_t &end) {
    if(range.empty()) {
        start = 0;
        end = 0;
        return true;
    }
    std::string str = range;
    size_t
        it = std::string::npos,
        sep = std::string::npos;
    int32_t invert_end = 1;
    if((it = str.find('-')) != std::string::npos) {
        //normal range start->end
        if(!it || str.rfind('-') != it) { return false; }
        if(str.length() - it == 1) { return false; }
        str[it] = ' ';
        sep = it;
    } else if((it = str.find('~')) != std::string::npos) {
        // inverted end range start->[actual end - end]
        if(!it || str.rfind('~') != it) { return false; }
        if(str.length() - it == 1) { return false; }
        str[it] = ' ';
        sep = it;
        invert_end = -1;
    }
    if(sep == std::string::npos) {
        return false;
    }
    for(auto it = str.begin(); it != str.end(); it++) {
        if((*it < '0' || *it > '9') && (*it != ' ')) {
            return false;
        }
    }
    start = atoi(str.c_str());
    end = atoi(str.c_str() + sep + 1) * invert_end;
    return true;
}

Args parse_args(int argc, const char **argv) {
    Args args;
    if(argc < 2) {
        usage();
    }
    if(argc > 5) {
        fail("Too many arguments given.\n");
    }

    if(argv[1][0] == '-') {
        switch(argc) {
            default:
                [[fallthrough]];
            case 5:
                args.third = argv[4];
                [[fallthrough]];
            case 4:
                args.second = argv[3];
                [[fallthrough]];
            case 3:
                args.first = argv[2];
                [[fallthrough]];
            case 2:
                args.option = argv[1] + 1;
                break;
        }
    } else {
        switch(argc) {
            default:
                [[fallthrough]];
            case 4:
                args.third = argv[3];
                [[fallthrough]];
            case 3:
                args.second = argv[2];
                [[fallthrough]];
            case 2:
                args.first = argv[1];
                break;
        }
    }
    return args;
}

int normal_operation(const Args &args) {
    if(!args.third.empty()) {
        fail("Too many arguments given.\n");
    }
    std::string
        input = args.first,
        output = args.second;
    auto ext = file_extension_find(input);

    if(file_extension_matches(ext, { ".fzml" })) {
        printf("Converting FZ-ML file to binary:\n");
        API::MemoryBlocks blocks;
        API::MemoryObjectPtr obj;
        API::XmlLoader loader(input);
        FzFileType file_type;
        auto result = loader.load(obj, &file_type);
        check_result(result);

        if(output.empty()) {
            if(file_type != TYPE_UNKNOWN) {
                output = input;
                std::string ext = file_type_to_extension(file_type);
                file_extension_replace_or_append(output, ext);
                printf("No filename supplied for output file (using %s).\n",
                    output.c_str());
            }
        }

        API::BlockDumper dumper(output);
        result = API::MemoryObject::pack(obj, blocks);
        check_result(result);
        result = dumper.dump(blocks);
        check_result(result);

        printf("Success!\n");
        return EXIT_SUCCESS;

    } else if(file_extension_matches(ext, { ".fzb", ".fze", ".fzf", ".fzv" })) {
        printf("Converting binary file to FZ-ML:\n");

        if(output.empty()) {
            output = input;
            file_extension_replace_or_append(output, ".fzml");
            printf("No filename supplied for output file (using %s).\n",
                output.c_str());
        }

        API::MemoryBlocks blocks;
        API::MemoryObjectPtr obj;
        API::BlockLoader loader(input);
        API::XmlDumper dumper(output, extension_to_file_type(ext));
        API::Result result = loader.load(blocks);
        check_result(result);
        result = blocks.unpack(obj);
        check_result(result);
        result = dumper.dump(obj);
        check_result(result);

        printf("Success!\n");
        return EXIT_SUCCESS;
    }

    printf("Unknown file extension (filename: %s)\n", input.c_str());
    return EXIT_FAILURE;
}

API::MemoryObjectPtr load_memory_object_list(const std::string &filename) {
    printf("Loading %s...\n", filename.c_str());
    API::MemoryObjectPtr first;
    auto ext = file_extension_find(filename);
    if(file_extension_matches(ext, { ".fzml" })) {
        API::XmlLoader loader(filename);
        API::MemoryObjectPtr obj;
        auto result = loader.load(obj);
        check_result(result);
        first = obj;

    } else if(file_extension_matches(ext, { ".fzb", ".fze", ".fzf", ".fzv" })) {
        API::MemoryBlocks blocks;
        API::MemoryObjectPtr obj;
        API::BlockLoader loader(filename);
        auto result = loader.load(blocks);
        check_result(result);
        result = blocks.unpack(obj);
        check_result(result);
        first = obj;
    } else {
        fail("Unknown file extension (filename: %s)\n", filename.c_str());
    }
    return first;
}

int display_info(const Args &args) {
    if(!args.second.empty()) {
        fail("Too many arguments given.\n");
    }
    std::string input = args.first;
    size_t
        count = 0,
        block_count = 0,
        bank_count = 0,
        effect_count = 0,
        wave_count = 0,
        voice_block_count = 0,
        voice_count = 0;

    if(input.empty()) {
        fail("No input filename specified\n");
    }
    if(API::MemoryObjectPtr first = load_memory_object_list(input)) {
        printf("Object Information:\n");
        API::MemoryObjectPtr obj = first;
        while(obj) {
            count++;
            if(auto *bank = obj->bank()) {
                bank_count++;
                block_count++;
                printf("%3u: \"%s\" (Bank %u)\n",
                    block_count, bank->name, bank_count);
            } else if(obj->effect()) {
                effect_count++;
                block_count++;
                printf("%3u: (Effect %u)\n", block_count, effect_count);
            } else if(auto *voice = obj->voice()) {
                voice_count++;
                if((voice_count % 4) == 1) {
                    block_count++;
                    voice_block_count++;
                    printf("%3u: \"%s\" (Voice %u)\n",
                        block_count, voice->name, voice_count);
                } else {
                    printf("     \"%s\" (Voice %u)\n", voice->name, voice_count);
                }
            } else if(obj->wave()) {
                wave_count++;
                block_count++;
            }
            obj = obj->next();
        }
        if(wave_count > 0) {
            printf("(+%u Wave blocks)\n", wave_count);
        }
        printf("\nSummary:\n"
            "%7u Effect(s)\n"
            "%7u Bank(s)\n"
            "%7u Voice Block(s)\n"
            "%7u Voice(s)\n"
            "%7u Wave(s)\n"
            "%5u Block(s) total\n"
            "%5u Object(s) total\n",
            effect_count, bank_count, voice_block_count, voice_count,
            wave_count, block_count, count);
    }
    return EXIT_SUCCESS;
}

int display_version(const Args&) {
    printf("fzutility v%s\n", VERSION.c_str());
    return EXIT_SUCCESS;
}

int extract_wave(const Args &args) {
    printf("Extracting Wave data...\n");
    std::string
        input = args.first,
        range = args.second,
        output = args.third;
    size_t start = 0;
    int32_t end = 0; // end can be negative to indicate "n samples from end"
    if(input.empty()) {
        fail("No input filename specified\n");
    }
    if(!parse_range(range, start, end)) {
        fail("Couldn't parse range (%s).\n", range.c_str());
    }

    if(API::MemoryObjectPtr first = load_memory_object_list(input)) {
        API::MemoryObjectPtr obj = first;
        while(obj && !obj->wave()) {
            obj = obj->next();
        }
        if(!obj) {
            fail("No wave data!\n");
        }
        if(end <= 0) {
            size_t wave_count = 0;
            API::MemoryObjectPtr wave = obj;
            while(wave) {
                if(wave->wave()) {
                    wave_count++;
                }
                wave = wave->next();
            }
            end = (wave_count * 512) + end;
            if(end < 0) {
                fail("Endpoint would be %d "
                    "samples before start of wave!\n", -end);
            }
        }
        size_t positive_end = end;
        if(start > positive_end) {
            fail("Start (%u) is after end (%u)!\n", start, positive_end);
        }
        if(start && (start == positive_end)) {
            fail("Start (%u) is equal to end, "
                "which would produce empty output.\n", start);
        }

        printf("Wave data from %u-%u...\n", start, positive_end);
        size_t
            offset = start,
            count = end - start;
        if(output.empty()) {
            output = input;
            file_extension_replace_or_append(output, ".wav");
        }
        printf("Dumping wave data to %s\n", output.c_str());
        if(auto* wave = static_cast<API::MemoryWave*>(obj.get())) {
            auto result = wave->dump_wav(output, API::SR_36kHz, offset, count);
            check_result(result);

            printf("Success!\n");
            return EXIT_SUCCESS;
        }
    }
    return EXIT_FAILURE;
}

int special_operation(const Args &args) {
    if(string_equals(args.option, { "?", "h", "help", "-help" })) {
        usage();
    } else if(args.option == "i") {
        return display_info(args);
    } else if(args.option == "v") {
        return display_version(args);
    } else if(args.option == "w") {
        return extract_wave(args);
    }

    printf("Unknown option: use -? or -help for assistance\n");
    return EXIT_FAILURE;
}

int main(int argc, const char **argv) {
    auto args = parse_args(argc, argv);
    return args.option.empty() ?
        normal_operation(args) :
        special_operation(args);
}
