#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
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

//------------------------------------------------------------------------------

struct Args {
    std::string
        option,
        first,
        second;
};

[[noreturn]] void usage() {
    printf("Usage:\n\n"
        "  fzutility ‹input› ‹output›\n"
        "    Convert binary files to FZML (or vice versa).\n"
        "  fzutility -w ‹range› ‹input›\n"
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
    auto ext = file_extension_find(args.first);
    if(file_extension_matches(ext, { ".fzml" })) {
        printf("XML -> BIN\n");
        API::MemoryBlocks blocks;
        API::MemoryObjectPtr obj;
        API::XmlLoader loader(args.first);
        API::BlockDumper dumper(args.second);
        auto result = loader.load(obj);
        if(!API::result_success(result)) {
            error(result);
        }
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
        printf("BIN -> XML\n");
        API::MemoryBlocks blocks;
        API::MemoryObjectPtr obj;
        API::BlockLoader loader(args.first);
        API::XmlDumper dumper(args.second, extension_to_file_type(ext));
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
#if 0
    printf(
        "ok = %u\n"
        "args.option = %s\n"
        "args.first = %s\n"
        "args.second = %s\n",
        ok, args.option.c_str(), args.first.c_str(), args.second.c_str());
#endif

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
