#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <stdio.h>
#include <string>

using namespace Casio::FZ_1;


void header_test(API::MemoryBlocks &mb, char filetype) {
    switch(filetype) {
        default: break;
        case 'b': {
            BankBlockFileHeader *h = mb.bank_header();
            printf("BankBlockFileHeader = %p\n", h);
            break;
        }
        case 'e': {
            EffectBlockFileHeader *h = mb.effect_header();
            printf("EffectBlockFileHeader = %p\n", h);
            break;
        }
        case 'f': {
            BankBlockFileHeader *bh = mb.bank_header();
            EffectBlockFileHeader *eh = mb.effect_header();
            VoiceBlockFileHeader *vh = mb.voice_header();
            printf("{Bank|Effect|Voice}BlockFileHeader = {%p|%p|%p}\n", bh, eh, vh);
            break;
        }
        case 'v': {
            VoiceBlockFileHeader *h = mb.voice_header();
            printf("VoiceBlockFileHeader = %p\n", h);
            break;
        }
    }
}


int main(int argc, const char **argv) {
    std::string filename{ "fz_data\\effect.fze" };
    char filetype = 'v';
    if(argc > 1) {
        filename = std::string{ argv[1] };
    }
    if(argc > 2) {
        filetype = argv[2][0];
    }

    //filename = std::string{ "fz_data/" } + filename;

    API::MemoryBlocks mb;
    API::BlockLoader bl(filename);
    auto r = bl.load(mb);
    if(r == API::RESULT_OK) {
        printf("Generic Header:\n");
        printf("file_type = %u\n", mb.file_type());
        if(auto *hdr = mb.header()) {
            printf("- version = %u\n", hdr->version);
            printf("- bank_count = %u\n", hdr->bank_count);
            printf("- voice_count = %u\n", hdr->voice_count);
            printf("- block_count = %u\n", hdr->block_count);
            printf("- wave_block_count = %u\n", hdr->wave_block_count);
        } else {
            printf("- No header?!\n");
        }
        header_test(mb, filetype);
    } else {
        printf("Load error: %u\n", r);
    }
    return 0;
}
