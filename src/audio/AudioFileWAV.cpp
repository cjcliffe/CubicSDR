// Copyright (c) Charles J. Cliffe
// SPDX-License-Identifier: GPL-2.0+

#include "AudioFileWAV.h"


// Simple endian io read/write handling from 
// http://www.cplusplus.com/forum/beginner/31584/#msg171056
namespace little_endian_io
{
    template <typename Word>
    std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word)) {
        for (; size; --size, value >>= 8) {
            outs.put(static_cast <char> (value & 0xFF));
        }
        return outs;
    }

    template <typename Word>
    std::istream& read_word(std::istream& ins, Word& value, unsigned size = sizeof(Word)) {
        for (unsigned n = 0, value = 0; n < size; ++n) {
            value |= ins.get() << (8 * n);
        }
        return ins;
    }
}

namespace big_endian_io
{
    template <typename Word>
    std::ostream& write_word(std::ostream& outs, Word value, unsigned size = sizeof(Word)) {
        while (size) {
            outs.put(static_cast <char> ((value >> (8 * --size)) & 0xFF));
        }
        return outs;
    }

    template <typename Word>
    std::istream& read_word(std::istream& ins, Word& value, unsigned size = sizeof(Word)) {
        for (value = 0; size; --size) {
            value = (value << 8) | ins.get();
        }
        return ins;
    }
}

using namespace little_endian_io;

AudioFileWAV::AudioFileWAV() : AudioFile() {
}

AudioFileWAV::~AudioFileWAV() {
}


std::string AudioFileWAV::getExtension()
{
    return "wav";
}

bool AudioFileWAV::writeToFile(AudioThreadInputPtr input)
{
    if (!outputFileStream.is_open()) {
        std::string ofName = getOutputFileName();
                
        outputFileStream.open(ofName.c_str(), std::ios::binary);

        // Based on simple wav file output code from
        // http://www.cplusplus.com/forum/beginner/166954/

        // Write the wav file headers
        outputFileStream << "RIFF----WAVEfmt "; // (chunk size to be filled in later)
        write_word(outputFileStream, 16, 4); // no extension data
        write_word(outputFileStream, 1, 2); // PCM - integer samples
        write_word(outputFileStream, input->channels, 2); // channels
        write_word(outputFileStream, input->sampleRate, 4); // samples per second (Hz)
        write_word(outputFileStream, (input->sampleRate * 16 * input->channels) / 8, 4); // (Sample Rate * BitsPerSample * Channels) / 8
        write_word(outputFileStream, input->channels * 2, 2); // data block size (size of integer samples, one for each channel, in bytes)
        write_word(outputFileStream, 16, 2); // number of bits per sample (use a multiple of 8)

        // Write the data chunk header
        dataChunkPos = outputFileStream.tellp();
        outputFileStream << "data----";  // (chunk size to be filled in later)
    }

    // Prevent clipping
    float intScale = (input->peak < 1.0) ? 32767.0f : (32767.0f / input->peak);

    if (input->channels == 1) {
        for (size_t i = 0, iMax = input->data.size(); i < iMax; i++) {
            write_word(outputFileStream, int(input->data[i] * intScale), 2);
        }
    }
    else if (input->channels == 2) {
        for (size_t i = 0, iMax = input->data.size() / 2; i < iMax; i++) {
            write_word(outputFileStream, int(input->data[i * 2] * intScale), 2);
            write_word(outputFileStream, int(input->data[i * 2 + 1] * intScale), 2);
        }
    }

    // TODO: Periodically update the RIFF/data chunk size in case of crash?

    return true;
}

bool AudioFileWAV::closeFile()
{
    if (outputFileStream.is_open()) {
        size_t file_length = outputFileStream.tellp();

        // Fix the data chunk header to contain the data size
        outputFileStream.seekp(dataChunkPos + 4);
        write_word(outputFileStream, file_length - dataChunkPos + 8);

        // Fix the file header to contain the proper RIFF chunk size, which is (file size - 8) bytes
        outputFileStream.seekp(0 + 4);
        write_word(outputFileStream, file_length - 8, 4);

        outputFileStream.close();
    }

    return true;
}
