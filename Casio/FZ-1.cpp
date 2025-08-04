#include "Casio/FZ-1.h"
#include <assert.h>
#include <stdio.h>

#define TRACE //printf("@-> %s:%u\n", __FILE__, __LINE__)

namespace Casio::FZ_1 {

// yes this is gross, but so is code repetition...
// (but there probably is a way to do this with templates...)
#define RETURN_VOICE(N_) \
    assert(N_ < 4); \
    switch(N_) { \
        default: return v1; \
        case 1: return v2; \
        case 2: return v3; \
        case 3: return v4; \
    }

Voice &VoiceBlock::operator[](size_t n) {
    RETURN_VOICE(n);
}

const Voice &VoiceBlock::operator[](size_t n) const {
    RETURN_VOICE(n);
}

Voice &VoiceBlockFileHeader::operator[](size_t n) {
    RETURN_VOICE(n);
}

const Voice &VoiceBlockFileHeader::operator[](size_t n) const {
    RETURN_VOICE(n);
}


#undef RETURN_VOICE

} // Casio::FZ_1
