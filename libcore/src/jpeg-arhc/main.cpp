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
#include <sys/syscall.h>
#include <linux/seccomp.h>

#include <sys/prctl.h>
#include <sys/wait.h>
#include <thread>
#include <atomic>
#include <pthread.h>

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


class FDWriter : public Sirikata::DecoderWriter {
    int fd;
public:
    FDWriter(int f) {
        fd = f;
    }
    void Close() {
        // nothing, for now
    }
    std::pair<Sirikata::uint32, Sirikata::JpegError> Write(const Sirikata::uint8*data, unsigned int size) {
        size_t written = 0;
        while (written < size) {
            ssize_t status = write(fd, data + written, size - written);
            if (status <= 0) {
                if (status < 0 && errno == EINTR) {
                    continue;
                } else {
                    return std::pair<Sirikata::uint32, Sirikata::JpegError>(written, Sirikata::JpegError::errEOF());
                }
            }
            written += status;
        }
        return std::pair<Sirikata::uint32, Sirikata::JpegError>(size, Sirikata::JpegError::nil());
    }
};

void* discard_input(void* vfd) {
    char buffer[16384];
    ssize_t status;
    char *fd = (char*)vfd;
    ssize_t fdd = (ssize_t)(fd - (char*)0);
    while((status = read((int)fdd, buffer, sizeof(buffer))) != 0) {
        if (status < 0 && errno != EINTR) {
            return NULL;
        }
    }
    return NULL;
}
std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > output_file;
void* copy_input(void * vfd) {
    char *fd = (char*)vfd;
    ssize_t fdd = fd - (char*)NULL;
    char buffer[65536];
    ssize_t read_status = 0;
    while((read_status = read((int)fdd, buffer, sizeof(buffer))) != 0) {
        if (read_status < 0) {
            if (errno == EINTR) {
                continue;
            } else {
                assert(false && "Must be able to copy file");
                return NULL;
            }
        }
        output_file.insert(output_file.end(), buffer, buffer + read_status);
    }
    return NULL;
}
int main(int argc, char **argv) {
    using namespace Sirikata;
    assert(BumpAllocatorTest() && "Bump allocator test must pass");
    FILE * input = stdin;
    FILE * output = stdout;
    uint8 level = 9;
    uint8 num_threads = 4;
    bool force7z = false;
    bool uncompressed = false;
    bool seccomp = false;
    int reps = 1;
    for (int i = 1; i < argc; ++i) {
        bool found = false;
        if (argv[i][0] == '-' && tolower(argv[i][1]) == 's' && argv[i][2] == '\0') {
            seccomp = true;
            found = true;
        }

        if (argv[i][0] == '-' && argv[i][1] == 'r' && argv[i][2] >='0' && argv[i][2] <= '9' && argv[i][3] == '\0') {
            reps = argv[i][2] - '0';
            found = true;
        }
 
        if (argv[i][0] == '-' && argv[i][1] == 'r' && argv[i][2] >='0' && argv[i][2] <= '9' && argv[i][3] >='0' && argv[i][3] <= '9' && argv[i][4] == '\0') {
            reps = 10 * (argv[i][2] - '0') + (argv[i][3] - '0');
            found = true;
        }
 

        if (argv[i][0] == '-' && argv[i][1] >='0' && argv[i][1] <= '9' && argv[i][2] == '\0') {
            level = argv[i][1] - '0';
            found = true;
        }
 
        if (argv[i][0] == '-' && argv[i][1] >='j' && argv[i][1] <= 'j'
            && ((argv[i][2] >= '0' && argv[i][2] <= '9')
                || (argv[i][2] >= 'A' && argv[i][2] <= 'F')
                || (argv[i][2] >= 'a' && argv[i][2] <= 'f'))
            && argv[i][3] == '\0') {

            num_threads = argv[i][2];
            if (num_threads >= '0' && num_threads <= '9') {
                num_threads -= '0';
            } else if (num_threads >= 'A' && num_threads <= 'F') {
                num_threads -= 'A';
                num_threads += 10;
            } else if (num_threads >= 'a' && num_threads <= 'f') {
                num_threads -= 'a';
                num_threads += 10;
            } else {
                assert(false && "unreachable");
            }
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
    unsigned char magic[5] = {0};
    size_t magic_size = fread(magic, 1, 4, input);
    bool unknown = true;
    bool isArhc = false;
    bool isXz = false;
    bool isJpeg = false;
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
    if (seccomp || true) {
        alloc.setup_memory_subsystem(2147483647,
                                     &BumpAllocatorInit,
                                     &BumpAllocatorMalloc,
                                     &BumpAllocatorFree);
    }
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
    std::vector<int, JpegAllocator<uint8_t> > pipe_write(alloc);
    std::vector<int, JpegAllocator<uint8_t> > pipe_read(alloc);
    pipe_write.resize(reps);
    pipe_read.resize(reps);
    for (int i = 0; i < reps; ++i) {
        int pipe_fds[2];
        int pipe_ret = pipe2(pipe_fds, 0);
        assert(pipe_ret == 0);
        pipe_read[i] = pipe_fds[0];
        pipe_write[i] = pipe_fds[1];
    }
    pid_t pid;
    int status = 1;
    pthread_t copy_thread = {};
    {
        std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > tmp(alloc);
        tmp.swap(output_file); // get the original allocator in there
    }
    if (!seccomp) {
        std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > tmp;
        tmp.swap(output_file); // get the original allocator in there
        for (int i = 0; i + 1 < reps; ++i) {
            pthread_t tmp_thread = {};
            pthread_create(&tmp_thread, NULL, &discard_input, (char*)0 + pipe_read[i]);
            pthread_detach(tmp_thread);
        }
        pthread_create(&copy_thread, NULL, &copy_input, (char*)0 + pipe_read[reps - 1]);
    }
    if ((!seccomp) || (pid = fork()) == 0) {
        ThreadContext *ctx = TestMakeThreadContext(num_threads, alloc, seccomp, NULL);
        if (seccomp) {
            if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT)) {
                syscall(SYS_exit, 1); // SECCOMP not allowed
            }            
        }
        for (int rep = 0; rep < reps; ++rep) {
            FDWriter memory_output(pipe_write[rep]);
            if (isJpeg && !force7z) {
                if (uncompressed) {
                    err = Decode(memory_input, memory_output, componentCoalescing, alloc);   
                } else {
                    err = CompressJPEGtoARHCMulti(memory_input, memory_output, level, componentCoalescing, ctx);
                }
            } else if (isXz && !force7z){
                err = MultiDecompress7ZtoAny(memory_input, memory_output, ctx);
            } else if (isArhc && !force7z) {
                if (uncompressed) {
                    err = Decode(memory_input, memory_output, componentCoalescing, alloc);
                } else {
                    err = DecompressARHCtoJPEGMulti(memory_input, memory_output, ctx);
                }
            } else {
                assert(unknown || force7z);
                ConstantSizeEstimator cse(memory_input.buffer().size());
                err = MultiCompressAnyto7Z(memory_input, memory_output, level, &cse, ctx);
            }
            memory_input.Close();
            memory_output.Close();
        }
        DestroyThreadContext(ctx);
        if (seccomp) {
            syscall(SYS_exit, 0);
        }
    }
    if (seccomp) {
        std::vector<uint8_t, Sirikata::JpegAllocator<uint8_t> > tmp;
        tmp.swap(output_file); // get the original allocator in there

        for (int i = 0; i + 1 < reps; ++i) {
            close(pipe_write[i]);
            pthread_t tmp_thread = {};
            pthread_create(&tmp_thread, NULL, &discard_input, (char*)0 + pipe_read[i]);
            pthread_detach(tmp_thread);
        }
        close(pipe_write[reps -1]);    
        pthread_create(&copy_thread, NULL, &copy_input, (char*)0 + pipe_read[reps - 1]);
        waitpid(pid, &status, 0);
    } else {
        for (int i = 0; i < reps; ++i) {
            close(pipe_write[i]);
        }
    }
    pthread_join(copy_thread, NULL);
    struct timeval tv_end;
    gettimeofday(&tv_end, NULL);
    double time_taken = tv_end.tv_sec + .000001 * tv_end.tv_usec;
    time_taken -= tv_start.tv_sec + .000001 * tv_start.tv_usec;
    time_taken /= reps;
    fprintf(stderr, "Time Taken, compression level %d time %.3f s size %d / %d %.1f%% status: %d\n",
            (int)level, time_taken, (int)output_file.size(), (int)memory_input.buffer().size(), 
            100. * output_file.size()/(double)memory_input.buffer().size(), status);
    if(!output_file.empty()) {
        if (argc > 2) {
            output = fopen(argv[2], "wb");
        }
        FileWriter writer(output, alloc);
        writer.Write(&output_file[0],
                     output_file.size());
        writer.Close();
    }
    if (err) {
        fprintf(stderr, "Error Encountered %s\n", err.what());
    }
    assert(!err);
}
