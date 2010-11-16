/*  Sirikata
 *  LoadFilter.cpp
 *
 *  Copyright (c) 2010, Ewen Cheslack-Postava
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
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#include "LoadFilter.hpp"
#include <sirikata/mesh/ModelsSystemFactory.hpp>

namespace Sirikata {
namespace Mesh {

LoadFilter::LoadFilter(const String& args) {
    mFilename = args;
}

FilterDataPtr LoadFilter::apply(FilterDataPtr input) {
    using namespace Sirikata::Transfer;

    typedef std::tr1::shared_ptr<SparseData> SparseDataPtr;

    ModelsSystem* parser = ModelsSystemFactory::getSingleton().getConstructor("any")("");

    // Load the file into a DenseData
    DenseDataPtr filedata;
    if (mFilename.empty()) { // use stdin
        SparseDataPtr sparse_data(new SparseData());
        FILE* fp = stdin;
        int offset = 0;
        while(!feof(fp)) {
            char buf[256];
            int nread = fread(&buf, 1, 256, fp);
            if (nread > 0) {
                MutableDenseDataPtr data_seg(new DenseData(Range(offset, nread, Transfer::LENGTH, false)));
                memcpy(data_seg->writableData(), buf, nread);
                offset += nread;
                sparse_data->addValidData(data_seg);
            }
        }
        filedata = sparse_data->flatten();
    }
    else {
        FILE* fp = fopen(mFilename.c_str(), "rb");
        fseek(fp, 0, SEEK_END);
        int fp_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        MutableDenseDataPtr mutable_filedata(new DenseData(Range(0, fp_len, Transfer::LENGTH, true)));
        fread(mutable_filedata->writableData(), 1, fp_len, fp);
        fclose(fp);
        filedata = mutable_filedata;
    }

    URI fileuri(std::string("file://") + mFilename);
    Fingerprint hash = Fingerprint::computeDigest(filedata->data(), filedata->size());
    MeshdataPtr md = parser->load(fileuri, hash, filedata);

    if (!md) {
        std::cout << "Error applying LoadFilter: " << mFilename << std::endl;
        return FilterDataPtr();
    }

    MutableFilterDataPtr output(new FilterData(*input.get()));
    output->push_back(md);
    return output;
}

} // namespace Mesh
} // namespace Sirikata
