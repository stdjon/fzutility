#include "Casio/FZ-1.h"
#include "Casio/FZ-1_API.h"
#include <stdio.h>
#include <string>

using namespace Casio::FZ_1;


void block_test(API::MemoryBlocks &mb) {
    size_t n = mb.count();
    printf("%u blocks:\n", n);
    size_t
        bank = 0,
        voice = 0,
        wave = 0,
        voice_count = mb.header()->voice_count;
    for(size_t i = 0; i < n; i++) {
        if(auto *bank_block = mb.bank_block(i)) {
            printf("= bank %u: \"%s\"\n", bank++, bank_block->name);
        }
        if(auto *voice_block = mb.voice_block(i)) {
            size_t q = voice_count > 4 ? 4 : voice_count;
            printf("= voice %u: (%u)\n", voice++, q);
            voice_count -= q;
        }
        if(auto *effect_block = mb.effect_block(i))  {
            printf("= (effect_block)\n");
        }
        if(auto *wave_block = mb.wave_block(i)) {
            if(!wave++) {
                printf("= wave(s): .");
            } else {
                putchar('.');
            }
        }
    }
    if(wave) {
        putchar('\n');
    }
}

void bank_test(API::MemoryBlocks &mb) {
    size_t n = mb.header()->bank_count;
    printf("%u banks:\n", n);
    for(size_t i = 0; i < n; i++) {
        if(auto *bank = mb.bank(i)) {
            printf("+ bank %u: \"%s\"\n", i, bank->name);
        }
    }
}

void voice_test(API::MemoryBlocks &mb) {
    size_t n = mb.header()->voice_count;
    printf("%u voices:\n", n);
    for(size_t i = 0; i < n; i++) {
        if(auto *voice = mb.voice(i)) {
            printf("_ voice %u: \"%s\"\n", i, voice->name);
        }
    }
}

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
    char filetype = 'b';
    if(argc > 1) {
        filename = std::string{ argv[1] };
    }
    if(argc > 2) {
        filetype = argv[2][0];
    }

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
        block_test(mb);
        bank_test(mb);
        voice_test(mb);
//        header_test(mb, filetype);
    } else {
        printf("Load error: %u\n", r);
    }
    return 0;
}
