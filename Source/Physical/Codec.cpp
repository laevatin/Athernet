#include <cstddef>
#include <iostream>
#include <string>

#include "Physical/Codec.h"

#include "lib/schifra_galois_field.hpp"
#include "lib/schifra_galois_field_polynomial.hpp"
#include "lib/schifra_sequential_root_generator_polynomial_creator.hpp"
#include "lib/schifra_reed_solomon_encoder.hpp"
#include "lib/schifra_reed_solomon_decoder.hpp"
#include "lib/schifra_reed_solomon_block.hpp"
#include "lib/schifra_error_processes.hpp"

Codec::Codec() {
    /* Instantiate Finite Field and Generator Polynomials */
    field = new schifra::galois::field(field_descriptor,
                                       schifra::galois::primitive_polynomial_size06,
                                       schifra::galois::primitive_polynomial06);

    generator_polynomial = new schifra::galois::field_polynomial(*field);

    if (
            !schifra::make_sequential_root_generator_polynomial(*field,
                                                                generator_polynomial_index,
                                                                generator_polynomial_root_count,
                                                                *generator_polynomial)
            ) {
        std::cout << "Error - Failed to create sequential root generator!" << std::endl;
    }

    encoder = new encoder_t(*field, *generator_polynomial);
    decoder = new decoder_t(*field, generator_polynomial_index);
}

DataType Codec::encode(const DataType &in) {
    DataType out;
    auto *pointer = in.getRawDataPointer();

    for (int i = 0; i < in.size(); i += data_length) {
        schifra::reed_solomon::block<code_length, fec_length> block;

        for (int j = i; j < std::min(i + (int) data_length, in.size()); j++) {
            block[j - i] = *(pointer + j);
        }

        if (!encoder->encode(block)) {
            std::cout << "Error - Critical encoding failure! "
                      << "Msg: " << block.error_as_string() << std::endl;
        }

        for (int k: block.data)
            out.add((uint8_t) k);
    }

    return std::move(out);
}

DataType Codec::encodeBlock(const DataType &in, int start) {
    DataType out;
    auto *pointer = in.getRawDataPointer();

    schifra::reed_solomon::block<code_length, fec_length> block;

    for (int j = start; j < std::min(start + (int) data_length, in.size()); j++)
        block[j - start] = *(pointer + j);

    if (!encoder->encode(block)) {
        std::cout << "Error - Critical encoding failure! "
                  << "Msg: " << block.error_as_string() << std::endl;
    }

    for (int k: block.data)
        out.add((uint8_t) k);

    return std::move(out);
}

bool Codec::decode(const DataType &in, DataType &out) {
    bool success = false;

    for (int i = 0; i < in.size(); i += code_length) {
        schifra::reed_solomon::block<code_length, fec_length> block;

        for (int j = 0; j < code_length; j++)
            block[j] = in[i + j];

        if (!(success = decoder->decode(block))) {
            std::cout << "Error - Critical decoding failure! "
                      << "Msg: " << block.error_as_string() << std::endl;
        }

        for (int k = 0; k < data_length; k++)
            out.add((uint8_t) block.data[k]);
    }

    return success;
}

bool Codec::decodeBlock(const DataType &in, DataType &out, int start) {
    bool success = false;

    schifra::reed_solomon::block<code_length, fec_length> block;

    for (int j = 0; j < code_length; j++)
        block[j] = in[start + j];

    if (!(success = decoder->decode(block))) {
        std::cout << "Error - Critical decoding failure! "
                  << "Msg: " << block.error_as_string() << std::endl;
    }

    for (int k = 0; k < data_length; k++)
        out.add((uint8_t) block.data[k]);

    return success;
}

Codec::~Codec() {
    delete encoder;
    delete decoder;
}