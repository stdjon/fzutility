#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <initializer_list>
#include <string>

using namespace Casio::FZ_1;


#define FAIL(...) do { \
        printf(__VA_ARGS__); \
        exit(EXIT_FAILURE); \
    } while(false)

//------------------------------------------------------------------------------

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

[[noreturn]] void usage() {
    printf("Usage:\n\n"
        "  fzutility <input> [<output>]\n"
        "    Convert binary files to FZML (or vice versa).\n"
        "  fzutility -w <input> [<range>] [<output>]\n"
        "    Extract wav data from binary or FZML files.\n"
        "  ...\n"
        );
    exit(EXIT_SUCCESS);
}

[[noreturn]] void error(API::Result result) {
    FAIL("Error: %s\n", API::result_str(result));
}

void check_result(API::Result result_) {
    if(!API::result_success(result_)) {
        error(result_);
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

bool parse_args(int argc, const char **argv, Args &args) {
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
                args.third = argv[2];
                [[fallthrough]];
            case 3:
                args.second = argv[2];
                [[fallthrough]];
            case 2:
                args.first = argv[1];
                break;
        }
    }
    return true;
}

int normal_operation(const Args &args) {
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

    printf("Unknown file extension.\n");
    return EXIT_FAILURE;
}

int special_operation(const Args &args) {
    if(string_matches(args.option, { "?", "h", "help", "-help" })) {
        usage();
    } else if(string_matches(args.option, { "w" })) {
        printf("Extracting Wave data...\n");
        std::string
            input = args.first,
            range = args.second,
            output = args.third;
        size_t start = 0;
        int32_t end = 0; // end can be negative to indicate "n samples from end"
        if(!parse_range(range, start, end)) {
            FAIL("Couldn't parse range (%s).\n", range.c_str());
        }
        API::MemoryObjectPtr first;
        auto ext = file_extension_find(input);
        if(file_extension_matches(ext, { ".fzml" })) {
            API::XmlLoader loader(input);
            API::MemoryObjectPtr obj;
            auto result = loader.load(obj);
            check_result(result);
            first = obj;

        } else if(file_extension_matches(ext, { ".fzb", ".fze", ".fzf", ".fzv" })) {
            API::MemoryBlocks blocks;
            API::MemoryObjectPtr obj;
            API::BlockLoader loader(input);
            auto result = loader.load(blocks);
            check_result(result);
            result = blocks.unpack(obj);
            check_result(result);
            first = obj;
        }

        if(first) {
            API::MemoryObjectPtr obj = first;
            while(obj && !obj->wave()) {
                obj = obj->next();
            }
            if(!obj) {
                FAIL("No wave data!\n");
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
                    FAIL("Endpoint would be %d "
                        "samples before start of wave!\n", -end);
                }
            }
            size_t pos_end = end;
            if(start > pos_end) {
                FAIL("Start (%u) is after end (%u)!\n", start, pos_end);
            }
            if(start && (start == pos_end)) {
                FAIL("Start (%u) is equal to end, "
                    "which would produce empty output.\n", start);
            }

            printf("Wave data from %u-%u...\n", start, pos_end);
            size_t
                offset = start,
                count = end - start;
            if(output.empty()) {
                output = input;
                file_extension_replace_or_append(output, ".wav");
            }
            printf("Dumping wave data to %s\n", output.c_str());
            if(auto* wave = static_cast<API::MemoryWave*>(obj.get())) {
                auto result = wave->dump_wav(output, 0, offset, count);
                check_result(result);

                printf("Success!\n");
                return EXIT_SUCCESS;
            }
        }
    }

    printf("Unknown option.\n");
    return EXIT_FAILURE;
}

int main(int argc, const char **argv) {
    Args args;
    int result = EXIT_SUCCESS;

    if(argc < 2) {
        usage();
    }

    auto ok = parse_args(argc, argv, args);

    if(!ok) {
        printf("Unknown option: use -? or -help for assistance");
        return EXIT_FAILURE;
    }

    if(args.option.empty()) {
        result = normal_operation(args);
    } else {
        result = special_operation(args);
    }

    return result;
}
