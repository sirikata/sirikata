/*  Sirikata
 *  ObjectSegmentation.cpp
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

#include <sirikata/space/ObjectSegmentation.hpp>
#include <sirikata/core/network/IOStrandImpl.hpp>

AUTO_SINGLETON_INSTANCE(Sirikata::OSegFactory);

namespace Sirikata {

OSegFactory& OSegFactory::getSingleton() {
    return AutoSingleton<OSegFactory>::getSingleton();
}

void OSegFactory::destroy() {
    AutoSingleton<OSegFactory>::destroy();
}

ObjectSegmentation::ObjectSegmentation(SpaceContext* ctx, Network::IOStrand* o_strand)
 : mContext(ctx),
   mStopping(false),
   mLookupListener(NULL),
   mWriteListener(NULL),
   oStrand(o_strand),
   mMigAckMessages( ctx->mainStrand->wrap(std::tr1::bind(&ObjectSegmentation::handleNewMigAckMessages, this)) ),
   mFrontMigAck(NULL)
{
    // Register a ServerMessage service for oseg
    mOSegServerMessageService = mContext->serverRouter()->createServerMessageService("oseg");

    //registering with the dispatcher.  can now receive messages addressed to it.
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE, this);
    mContext->serverDispatcher()->registerMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);
}

ObjectSegmentation::~ObjectSegmentation() {
    //delete retries
    while( !mMigAckMessages.empty() ) {
        Message* msg = NULL;
        mMigAckMessages.pop(msg);
        delete msg;
    }

    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE, this);
    mContext->serverDispatcher()->unregisterMessageRecipient(SERVER_PORT_OSEG_UPDATE, this);

    delete mOSegServerMessageService;
}

void ObjectSegmentation::receiveMessage(Message* msg)
{
    if (msg->dest_port() == SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE) {
        Sirikata::Protocol::OSeg::MigrateMessageAcknowledge oseg_ack_msg;
        bool parsed = parsePBJMessage(&oseg_ack_msg, msg->payload());
        if (parsed)
            this->handleMigrateMessageAck(oseg_ack_msg);
    }
    else if (msg->dest_port() == SERVER_PORT_OSEG_UPDATE) {
        Sirikata::Protocol::OSeg::UpdateOSegMessage update_oseg_msg;
        bool parsed = parsePBJMessage(&update_oseg_msg, msg->payload());
        if (parsed)
            this->handleUpdateOSegMessage(update_oseg_msg);
    }
    else {
        SILOG(oseg,error,"OSeg received unknown type of server message: " << msg->dest_port());
    }

    delete msg;
}

void ObjectSegmentation::queueMigAck(const Sirikata::Protocol::OSeg::MigrateMessageAcknowledge& msg) {
    Message* to_send = new Message(
        mContext->id(),
        SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
        msg.m_message_destination(),
        SERVER_PORT_OSEG_MIGRATE_ACKNOWLEDGE,
        serializePBJMessage(msg)
    );
    mMigAckMessages.push(to_send); // sending handled in main strand
}

void ObjectSegmentation::trySendMigAcks() {
    if (mStopping) return;

    while(true) {
        if (mFrontMigAck == NULL) {
            if (mMigAckMessages.empty())
                return;
            mMigAckMessages.pop(mFrontMigAck);
        }

        bool sent = mOSegServerMessageService->route( mFrontMigAck );
        if (!sent)
            break;

        mFrontMigAck = NULL;
    }

    if (mFrontMigAck != NULL || !mMigAckMessages.empty()) {
        // We've still got work to do, setup a retry
        mContext->mainStrand->post(
            Duration::microseconds(100),
            std::tr1::bind(&ObjectSegmentation::trySendMigAcks, this)
                                   );
    }
}

void ObjectSegmentation::handleNewMigAckMessages() {
    trySendMigAcks();
}

} // namespace Sirikata
