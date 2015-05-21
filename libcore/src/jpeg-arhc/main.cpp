/*  Sirikata Jpeg Texture Transfer Test Suite -- Texture Transfer management system
 *  main.cpp
 *
 *  Copyright (c) 2015, Daniel Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


// Copyright 2009 The Go Authors. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// Package jpeg implements a JPEG image decoder and encoder.
//
// JPEG is defined in ITU-T T.81: http://www.w3.org/Graphics/JPEG/itu-t81.pdf.
//
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sirikata/core/jpeg-arhc/Compression.hpp>
#include <sirikata/core/jpeg-arhc/BumpAllocator.hpp>
#include <sys/time.h>
class FileReader : public Sirikata::DecoderReader {
    FILE * fp;
    std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > magic;
    unsigned int magicread;
public:
    FileReader(FILE * ff,
               const std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > &magi)
            : magic(magi){
        fp = ff;
        magicread = 0;
    }
    std::pair<Sirikata::uint32, Sirikata::JpegError> Read(Sirikata::uint8*data, unsigned int size) {
        using namespace Sirikata;
        signed long nread = 0;
        if (magicread < magic.size()) {
            unsigned int amnt_to_copy = std::min((unsigned int)magic.size() + magicread, size);
            memcpy(data, magic.data() + magicread, amnt_to_copy);
            magicread += size;
            nread += amnt_to_copy;
        }
        nread += fread(data + nread, 1, size - nread, fp);
        //fprintf(stderr, "%d READ %02x%02x%02x%02x - %02x%02x%02x%02x\n", (int)nread, data[0], data[1],data[2], data[3],
        //        data[nread-4],data[nread-3],data[nread-2],data[nread-1]);
        if (nread <= 0) {
            return std::pair<Sirikata::uint32, JpegError>(0, MakeJpegError("Short read"));
        }
        return std::pair<Sirikata::uint32, JpegError>(nread, JpegError::nil());
    }
    size_t length() {
        size_t where = ftell(fp);
        fseek(fp, 0, SEEK_END);
        size_t retval = ftell(fp);
        fseek(fp, where, SEEK_SET);
        return retval;
    }
};
class FileWriter : public Sirikata::DecoderWriter {
    Sirikata::JpegAllocator<uint8_t> mAllocator;
    FILE * fp;
public:
    FileWriter(FILE * ff,
               const Sirikata::JpegAllocator<uint8_t> &alloc): mAllocator(alloc){
        fp = ff;
    }
    void Close() {
        fclose(fp);
        fp = NULL;
    }
    std::pair<Sirikata::uint32, Sirikata::JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
        using namespace Sirikata;
        signed long nwritten = fwrite(data, size, 1, fp);
        if (nwritten == 0) {
            return std::pair<Sirikata::uint32, JpegError>(0, MakeJpegError("Short write"));
        }
        return std::pair<Sirikata::uint32, JpegError>(size, JpegError::nil());
    }
};

int main(int argc, char **argv) {
    using namespace Sirikata;
    assert(BumpAllocatorTest() && "Bump allocator test must pass");
    FILE * input = stdin;
    FILE * output = stdout;
    uint8 level = 9;
    bool force7z = false;
    bool uncompressed = false;
    for (int i = 1; i < argc; ++i) {
        bool found = false;
        if (argv[i][0] == '-' && argv[i][1] >='0' && argv[i][1] <= '9' && argv[i][2] == '\0') {
            level = argv[i][1] - '0';
            found = true;
        }
        if (argv[i][0] == '-' && argv[i][1] == 'u' && argv[i][2] == '\0') {
            uncompressed = true;
            found = true;
        }
        if (argv[i][0] == '-' && argv[i][1] == 'x' && argv[i][2] == 'z' && argv[i][3] == '\0') {
            force7z = true;
            found = true;
        }
        if (found) {
            for (int j = i; j < argc - 1; ++j) {
                argv[j] = argv[j + 1];
            }
            --argc;
            --i;
        }
    }
    if (argc > 1) {
        input = fopen(argv[1], "rb");
    }
    if (argc > 2) {
        output = fopen(argv[2], "wb");
    }
    unsigned char magic[5] = {0};
    size_t magic_size = fread(magic, 1, 4, input);
    bool unknown = true;
    bool isArhc = false;
    bool isXz = false;
    bool isJpeg = false;
    int reps = 1;
    if (memcmp(magic, "arhc", 4) == 0) {
        isArhc = true;
        uncompressed = true;
    }else if (memcmp(magic, "ARHC", 4) == 0) {
        isArhc = true;
        unknown = false;
    } else if (memcmp(magic + 1, "7zX", 3) == 0) {
        isXz = true;
        unknown = false;
    } else if (magic[0] == 0xff && magic[1] == 0xd8) {
        isJpeg = true;
        unknown = false;
    }
    JpegAllocator<uint8_t> alloc;
/*
    alloc.setup_memory_subsystem(1024 * 1024 * 1024,
                                 &BumpAllocatorInit,
                                 &BumpAllocatorMalloc,
                                 &BumpAllocatorFree);
*/
    FileReader reader(input, std::vector<uint8_t, JpegAllocator<uint8_t> >(magic, magic + magic_size, alloc));

    std::vector<uint8_t, JpegAllocator<uint8_t> >buffered_input(alloc);
    
    buffered_input.resize(reader.length());
    MemReadWriter memory_input(alloc);
    if (!buffered_input.empty()) {
        std::pair<uint32, JpegError> xerr = reader.Read(&buffered_input[0], buffered_input.size());
        assert(xerr.second == JpegError());
        assert(xerr.first == buffered_input.size());
        memory_input.Write(&buffered_input[0], buffered_input.size());
    }

    //FileReader&memory_input = reader;
	FileWriter writer(output, alloc);
    bool coalesceYCr = false;
    bool coalesceYCb = false;
    bool coalesceCbCr = true;
	JpegError err;
	uint8 componentCoalescing = 0;
	if (coalesceYCb) {
		componentCoalescing |= Decoder::comp01coalesce;
	}
    if (coalesceYCr) {
        componentCoalescing |= Decoder::comp02coalesce;
	}
    if (coalesceCbCr) {
        componentCoalescing |= Decoder::comp12coalesce;
	}

    struct timeval tv_start;
    gettimeofday(&tv_start, NULL);
    MemReadWriter memory_output(alloc);
    for (int rep = 0; rep < reps; ++rep) {
        memory_output.buffer().clear();
        if (isJpeg && !force7z) {
            if (uncompressed) {
                err = Decode(memory_input, memory_output, componentCoalescing, alloc);   
            } else {
                err = CompressJPEGtoARHC(memory_input, memory_output, level, componentCoalescing, alloc);   
            }
        } else if (isXz && !force7z){
            err = Decompress7ZtoAny(memory_input, memory_output, alloc);
        } else if (isArhc && !force7z) {
            if (uncompressed) {
                err = Decode(memory_input, memory_output, componentCoalescing, alloc);   
            } else {
                err = DecompressARHCtoJPEG(memory_input, memory_output, alloc);
            }
        } else {
            assert(unknown || force7z);
            err = CompressAnyto7Z(memory_input, memory_output, level, alloc);
        }
        memory_input.Close();
        memory_output.Close();
    }
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);
    double time_taken = tv_end.tv_sec + .000001 * tv_end.tv_usec;
    time_taken -= tv_start.tv_sec + .000001 * tv_start.tv_usec;
    time_taken /= reps;
    fprintf(stderr, "Time Taken, compression level %d time %.3f s size %d / %d %.1f%%\n",
            (int)level, time_taken, (int)memory_output.buffer().size(), (int)memory_input.buffer().size(), 
            100. * memory_output.buffer().size()/(double)memory_input.buffer().size());
    if(!memory_output.buffer().empty()) {
        writer.Write(&memory_output.buffer()[0],
                     memory_output.buffer().size());
    }
    if (err) {
        fprintf(stderr, "Error Encountered %s\n", err.what());
    }
    assert(!err);
}
