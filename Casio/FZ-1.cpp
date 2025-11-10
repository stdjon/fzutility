#include "Casio/FZ-1.h"
#include <assert.h>
#include <stdio.h>

#define TRACE //printf("@-> %s:%u\n", __FILE__, __LINE__)

namespace Casio::FZ_1 {

template<typename T>struct ReturnVoice { using Type = Voice; };
template<typename T>struct ReturnVoice<const T> { using Type = const Voice; };

template<typename T>typename ReturnVoice<T>::Type &voice_at(T &v, size_t n) {
    assert(n < 4); \
    switch(n) { \
        default: return v.v1; \
        case 1: return v.v2; \
        case 2: return v.v3; \
        case 3: return v.v4; \
    }
}

Voice &VoiceBlock::operator[](size_t n) {
    return voice_at(*this, n);
}

const Voice &VoiceBlock::operator[](size_t n) const {
    return voice_at(*this, n);
}

Voice &VoiceBlockFileHeader::operator[](size_t n) {
    return voice_at(*this, n);
}

const Voice &VoiceBlockFileHeader::operator[](size_t n) const {
    return voice_at(*this, n);
}


#undef RETURN_VOICE

} // Casio::FZ_1
