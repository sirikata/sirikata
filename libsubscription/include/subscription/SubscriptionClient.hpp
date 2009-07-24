/*  Sirikata Subscription and Broadcasting System -- Subscription Services
 *  SubscriptionClient.hpp
 *
 *  Copyright (c) 2009, Daniel Reiter Horn
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
#ifndef _SIRIKATA_SUBSCRIPTION_SUBSCRIPTIONCLIENT_HPP_
#define _SIRIKATA_SUBSCRIPTION_SUBSCRIPTIONCLIENT_HPP_
#include <network/Stream.hpp>
#include <network/StreamListener.hpp>
#include <util/UUID.hpp>
namespace Sirikata { namespace Subscription {

namespace Protocol {
class Subscribe;
}
class SubscriptionClient {
    class AddressUUID {
        Address mAddress;
        UUID mUUID;
        friend class Hasher;
    public:
        AddressUUID(const Network::Address&a,const UUID&&u):mAddress(a),mUUID(u) {}
        AddressUUID():mAddress(Network::Address::NULL),mUUID(UUID::null()) {}
        bool operator< (const AddressUUID&other) const{
            return mUUID==other.mUUID?mAddress<other.mAddress:mUUID<other.mUUID;
        }
        bool operator==(const AddressUUID&other) const{
            return mAddress==other.mAddress&&mUUID==other.mUUID;
        }
        class Hasher {public:
            bool operator() (const AddressUUID&au) const{
                return Address::Hasher()(au.mAddress)^UUID::Hasher()(au.mUUID);
            }
        };
    };
    Network::IOService*mService;
    class UniqueLock;
    UniqueLock*mMapLock;
    typedef std::unordered_map<Address,std::weak_ptr<Network::Stream> > TopLevelStreamMap;
    typedef std::unordered_map<AddressUUID,std::tr1::weak_ptr<State>,AddressUUID::Hasher> BroadcastMap;

    void upgradeFromIOThread(const std::weak_ptr<State>&source,
                             const std::weak_ptr<State>&dest);

    void addSubscriberFromIOThread(const std::tr1::weak_ptr<IndividualSubscription>&, bool sendIntroMessage);
    void removeSubscriberFromIOThreadHint(const Address mAddress;
                                          const UUID &mUUID);

protected:
    TopLevelStreamMap mTopLevelStreams;
    BroadcastMap mBroadcasts;
    ///schedules a work task to upgrade all items from source to destination
    void upgrade(const std::weak_ptr<State>&source,
                 const std::weak_ptr<State>&dest);
    void addSubscriber(const std::tr1::weak_ptr<IndividualSubscription>&, bool sendIntroMessage);
    void removeSubscriberHint(const Address &mAddress;
                              const UUID &mUUID);

public:
    class IndividualSubscription;
protected:
    class State{
        Network::Chunk mLastDeliveredMessage;
        std::shared_ptr<Network::Stream> mTopLevelStream;
    public:
        ///this function goes through all subscribers of this State and sees if any are dead (probably). Also computes the maximum needed period and potentially downgrades the subscribers if it's too high
        void purgeSubscribersFromIOThread(std::tr1::weak_ptr<State>&weak_thus, SubscriptionClient *parent);
        const Address mAddress;
        const UUID mUUID;
        Duration mPeriod;
        std::tr1::shared_ptr<Network::Stream> mStream;
        std::vector<std::tr1::weak_ptr<IndividualSubscription> > mSubscribers;
        State(const Duration &period,
              const Network::Address&address,
              const UUID&uuid);
        void setStream(const std::shared_ptr<State>thus, const std::tr1::shared_ptr<Network::Stream>topLevelStream, const String&serialized_introduction=String());
        static void bytesReceived(const std::weak_ptr<State>&,
                                  const Network::Chunk&data);
        static void connectionCallback(const std::weak_ptr<State>&thus,
                                       const Network::Stream::ConnectionStatus,
                                       const String&reason);
    };

public:
    SubscriptionClient(Network::IOService*mService);
    ~SubscriptionClient();
    class IndividualSubscription:public {
        IndividualSubscription(const std::tr1::function<void(const Network::Chunk&)>function, const Duration&period);
        friend class Subscriber;
        std::tr1::function<void(const Network::Chunk&)> mFunction;
        std::tr1::function<void()> mDisconFunction;
    public:
        std::tr1::shared_ptr<State> mSubscriptionState;

        Duration mPeriod;
        void call(const Network::Chunk&chunk)const{
            mFunction(chunk);
        }
    };
    std::tr1::shared_ptr<IndividualSubscription> subscribe(const Network::Address& address,
                                     const Protocol::Subscribe&subscription,
                                     const std::tr1::function<void(const Network::Chunk&)>&bytesReceived,
                                     const std::tr1::function<void()>&disconnectionFunction,
                                                           const String&serializedSubscription=String(),
                                                           const std::tr1::shared_ptr<IndividualSubscription>&=std::tr1::shared_ptr<IndividualSubscription>());
    std::tr1::shared_ptr<IndividualSubscription> subscribe(const Network::Address& address,
                                                           const String&subscription,
                                                           const std::tr1::function<void(const Network::Chunk&)>&,
                                                           const std::tr1::function<void()>&disconnectionFunction);
};


} }
#endif
