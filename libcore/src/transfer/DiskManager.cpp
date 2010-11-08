/*  Sirikata Transfer
 *  DiskManager.cpp
 *
 *  Copyright (c) 2010, Jeff Terrace
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

#include <sirikata/core/transfer/DiskManager.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::Transfer::DiskManager);

namespace Sirikata {
namespace Transfer {

DiskManager& DiskManager::getSingleton() {
    return AutoSingleton<DiskManager>::getSingleton();
}
void DiskManager::destroy() {
    AutoSingleton<DiskManager>::destroy();
}

std::string DiskManager::DiskFile::getPath() {
    return "";
}

std::string DiskManager::DiskFile::getFullPath() {
    return "";
}

std::string DiskManager::DiskFile::getLeafName() {
    return "";
}

bool DiskManager::DiskFile::isDirectory() {
    return true;
}

bool DiskManager::DiskFile::isFile() {
    return true;
}

DiskManager::ScanRequest::ScanRequest(DiskFile path, DiskManager::ScanRequest::ScanRequestCallback cb) {
}

void DiskManager::ScanRequest::execute() {
}

DiskManager::DiskManager() {

}

}
}
