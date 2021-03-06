#pragma once

#ifndef __CODEC_H__
#define __CODEC_H__

#include <JuceHeader.h>
#include "Config.h"

typedef juce::Array<uint8_t> DataType;

/* Class forwarding */
namespace schifra {
    namespace reed_solomon {
        template<std::size_t code_length,
                std::size_t fec_length,
                std::size_t data_length = code_length - fec_length,
                std::size_t natural_length = 255, // Needs to be in-sync with field size
                std::size_t padding_length = natural_length - data_length - fec_length>
        class shortened_encoder;

        template<std::size_t code_length,
                std::size_t fec_length,
                std::size_t data_length = code_length - fec_length,
                std::size_t natural_length = 255, // Needs to be in-sync with field size
                std::size_t padding_length = natural_length - data_length - fec_length>
        class shortened_decoder;
    }

    namespace galois {
        class field;

        class field_polynomial;
    }
}

class Codec {
public:
    Codec();

    ~Codec();

    DataType encode(const DataType &in);

    DataType encodeBlock(const DataType &in, int start);

    bool decode(const DataType &in, DataType &out);

    bool decodeBlock(const DataType &in, DataType &out, int start);

    /* Reed Solomon Code Parameters */
    constexpr static std::size_t code_length = Config::BIT_PER_FRAME / 8;
    constexpr static std::size_t fec_length = (Config::BIT_PER_FRAME - Config::DATA_PER_FRAME) / 8;
    constexpr static std::size_t data_length = code_length - fec_length;

private:
    /* Finite Field Parameters */
    constexpr static std::size_t field_descriptor = 8;
    constexpr static std::size_t generator_polynomial_index = 120;
    constexpr static std::size_t generator_polynomial_root_count = fec_length;

    typedef schifra::reed_solomon::shortened_encoder<code_length, fec_length, data_length> encoder_t;
    typedef schifra::reed_solomon::shortened_decoder<code_length, fec_length, data_length> decoder_t;

    const schifra::galois::field *field;
    schifra::galois::field_polynomial *generator_polynomial;

    encoder_t *encoder;
    decoder_t *decoder;
};

#endif