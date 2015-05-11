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
#include "reader.h"
class FileReader : public Sirikata::Reader {
    FILE * fp;
public:
    FileReader(FILE * ff){
        fp = ff;
    }
    std::pair<Sirikata::uint32, Sirikata::JpegError> Read(Sirikata::uint8*data, unsigned int size) {
        using namespace Sirikata;
        signed long nread = fread(data, 1, size, fp);
        if (nread <= 0) {
            return std::pair<Sirikata::uint32, JpegError>(0, JpegError("Short read"));
        }
        return std::pair<Sirikata::uint32, JpegError>(nread, JpegError::nil);
    }
};
class FileWriter : public Sirikata::Writer {
    FILE * fp;
public:
    FileWriter(FILE * ff){
        fp = ff;
    }
    std::pair<Sirikata::uint32, Sirikata::JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
        using namespace Sirikata;
        signed long nwritten = fwrite(data, size, 1, fp);
        if (nwritten == 0) {
            return std::pair<Sirikata::uint32, JpegError>(0, JpegError("Short write"));
        }
        return std::pair<Sirikata::uint32, JpegError>(size, JpegError::nil);
    }
};

int main(int argc, char **argv) {
    using namespace Sirikata;
    FILE * input = stdin;
    FILE * output = stdout;
    if (argc > 1) {
        input = fopen(argv[1], "rb");
    }
    if (argc > 2) {
        output = fopen(argv[2], "wb");
    }
    FileReader reader(input);
	FileWriter writer(output);

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
	err = Decode(reader, writer, componentCoalescing);
    if (err) {
        fprintf(stderr, "Error Encountered %s\n", err.what());
    }
    assert(!err);
}
