#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <initializer_list>
#include <string>

using namespace Casio::FZ_1;


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
        "  fzutility -w <range> <input> [<output>]\n"
        "    Extract wav data from binary or FZML files.\n"
        "  ...\n"
        );
    exit(EXIT_SUCCESS);
}

[[noreturn]] void error(API::Result result) {
    printf("Error: %s\n", API::result_str(result));
    exit(EXIT_FAILURE);
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
        if(!API::result_success(result)) {
            error(result);
        }
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
        if(!API::result_success(result)) {
            error(result);
        }
        result = dumper.dump(blocks);
        if(!API::result_success(result)) {
            error(result);
        }
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
        if(!API::result_success(result)) {
            error(result);
        }
        result = blocks.unpack(obj);
        if(!API::result_success(result)) {
            error(result);
        }
        result = dumper.dump(obj);
        if(!API::result_success(result)) {
            error(result);
        }
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
            range = args.first,
            input = args.second,
            output = args.third;
        return EXIT_SUCCESS;
    } else {
        printf("Unknown option.\n");
    }
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
