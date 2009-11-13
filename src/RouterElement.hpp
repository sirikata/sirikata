/*  cbr
 *  ForwarderElement.hpp
 *
 *  Copyright (c) 2009, Ewen Cheslack-Postava
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
 *  * Neither the name of cbr nor the names of its contributors may
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

#ifndef _CBR_ROUTER_ELEMENT_
#define _CBR_ROUTER_ELEMENT_

#include "Utility.hpp"
#include <sirikata/network/IOStrand.hpp>

namespace CBR {

/** Interface for schedulable routing elements.  These elements can request to
 *  be scheduled for events and have a single method which is called when their
 *  task is handled.
 */
class SchedulableRouterElement {
public:
    /** Create a SchedulableRouterElement which will run its tasks in the specified strand.
     *  \param strand the IOStrand to run this elements tasks in
     */
    SchedulableRouterElement(Sirikata::Network::IOStrand& strand);

    virtual ~SchedulableRouterElement();

    /** Request that this element be scheduled as a task on the event queue.  Note that there
     *  may only ever be a single outstanding request per element.  This means that it is
     *  safe to call this multiple times, but that run() should handle any outstanding events
     *  within the element.
     */
    void requestRun();

    /** Run this elements task.  This should be overridden by subclasses to create element
     *  specific behavior.
     */
    virtual void run();
private:
    /** Callback which actually handles the event and ensures the element can be rescheduled. */
    void handleRun();

    Sirikata::Network::IOStrand& mStrand;
    bool mOutstandingRequest;
}; // class IRouterElement



// Forward declarations of classes below since they need to refer to
// each other
template<typename PacketType>
class DownstreamElementBase;
template<typename PacketType>
class UpstreamElementBase;


/** Base class for elements that accept input.  Downstream refers to
 *  the fact that it receives input from other elements.
 */
template<typename PacketType>
class DownstreamElementBase {
public:
    typedef UpstreamElementBase<PacketType> InputElement;

    /** Connect the specified input port of this element to another router element,
     *  on the specified output port.
     *  \param inport the input port to connect to the other element
     *  \param elmt the element to connect the port to
     *  \param outport the output port on the providing element to connect to
     *  \returns true if the connection was successful, false otherwise
     */
    virtual bool connectInput(uint32 inport, InputElement* elmt, uint32 outport) {
        InputPort& i = input(inport);
        i.set(elmt, outport);
        return true;
    }

    /** Push a packet to this element on the given port.
     *  \param port the port to push the packet to
     *  \param pkt the packet to push
     */
    virtual bool push(uint32 port, PacketType* pkt) = 0;

protected:
    /** An input port for this element. A convenience for inherited
     *  which performs some error checking.
     */
    class InputPort {
    public:
        InputPort()
         : element(NULL),
           port()
        {}

        bool set(InputElement* elmt, uint32 _port) {
            element = elmt;
            port = _port;
            return true;
        }

        PacketType* pull() {
            assert(element != NULL);
            return element->pull(port);
        }

        InputElement* element;
        uint32 port;
    };

    virtual InputPort& input(uint32 k) = 0;
}; // class DownstreamElementBase

/** Base class for downstream elements with a fixed number of input ports. */
template<typename PacketType, uint32 NumInputs>
class DownstreamElementFixed : public DownstreamElementBase<PacketType> {
public:
    typedef typename DownstreamElementBase<PacketType>::InputElement InputElement;

    DownstreamElementFixed() {
        memset(mInputPorts, 0, sizeof(InputPort)*NumInputs);
    };

protected:
    typedef typename DownstreamElementBase<PacketType>::InputPort InputPort;

    virtual InputPort& input(uint32 k) {
        assert(k < NumInputs);
        return mInputPorts[k];
    }

    InputPort mInputPorts[NumInputs];
}; // class DownstreamElementFixed

/** Base class for downstream elements, using a default implementation for port lookups. */
template<typename PacketType>
class DownstreamElement : public DownstreamElementBase<PacketType> {
public:
    typedef typename DownstreamElementBase<PacketType>::InputElement InputElement;

    DownstreamElement(uint32 nports) {
        mInputPorts.resize(nports);
        for(uint32 i = 0;i < nports; i++)
            mInputPorts[i].element = NULL;
    };

protected:
    typedef typename DownstreamElementBase<PacketType>::InputPort InputPort;

    virtual InputPort& input(uint32 k) {
        assert(k < mInputPorts.size());
        return mInputPorts[k];
    }

    std::vector<InputPort> mInputPorts;
}; // class DownstreamElement


/** Base class for elements that generate output.  Upstream refers to
 *  the fact that packets flow from it to other elements.
 */
template<typename PacketType>
class UpstreamElementBase {
public:
    typedef DownstreamElementBase<PacketType> OutputElement;

    /** Connect the specified output port of this element to another router element,
     *  on the specified input port.
     *  \param outport the output port to connect to the other element
     *  \param elmt the element to connect the port to
     *  \param inport the input port on the receiving element to connect to
     *  \returns true if the connection was successful, false otherwise
     */
    virtual bool connect(uint32 outport, OutputElement* elmt, uint32 inport) {
        OutputPort& o = output(outport);
        o.set(elmt, inport);
        elmt->connectInput(inport, this, outport);
        return true;
    }

    /** Pull a packet from the element on the specified port.
     *  \param port the port to request the packet from
     *  \returns a packet, or NULL if one cannot be pulled currently
     */
    virtual PacketType* pull(uint32 port) = 0;

protected:
    /** An output port for this element. A convenience for inherited
     *  which performs some error checking.
     */
    class OutputPort {
    public:
        OutputPort()
         : element(NULL),
           port()
        {}

        bool set(OutputElement* elmt, uint32 _port) {
            element = elmt;
            port = _port;
            return true;
        }

        bool push(PacketType* pkt) {
            assert(element != NULL);
            return element->push(port, pkt);
        }

        OutputElement* element;
        uint32 port;
    };

    virtual OutputPort& output(uint32 k) = 0;
}; // class UpstreamElementBase

/** Base class for upstream elements with a fixed number of input ports. */
template<typename PacketType, uint32 NumOutputs>
class UpstreamElementFixed : public UpstreamElementBase<PacketType> {
public:
    typedef typename UpstreamElementBase<PacketType>::OutputElement OutputElement;

    UpstreamElementFixed() {
        memset(mOutputPorts, 0, sizeof(OutputPort)*NumOutputs);
    };

protected:
    typedef typename UpstreamElementBase<PacketType>::OutputPort OutputPort;

    virtual OutputPort& output(uint32 k) {
        assert(k < NumOutputs);
        return mOutputPorts[k];
    }

    OutputPort mOutputPorts[NumOutputs];
}; // class UpstreamElementFixed

/** Base class for downstream elements, using a default implementation for port lookups. */
template<typename PacketType>
class UpstreamElement : public UpstreamElementBase<PacketType> {
public:
    typedef typename UpstreamElementBase<PacketType>::OutputElement OutputElement;

    UpstreamElement(uint32 nports) {
        mOutputPorts.resize(nports);
        for(uint32 i = 0;i < nports; i++)
            mOutputPorts[i].element = NULL;
    };

protected:
    typedef typename UpstreamElementBase<PacketType>::OutputPort OutputPort;

    virtual OutputPort& input(uint32 k) {
        assert(k < mOutputPorts.size());
        return mOutputPorts[k];
    }

    std::vector<OutputPort> mOutputPorts;
}; // class UpstreamElement


/** Convenience base class for router elements that handle input and output. */
template<typename PacketType>
class RouterElement : public DownstreamElement<PacketType>, public UpstreamElement<PacketType> {
public:
    typedef typename UpstreamElement<PacketType>::OutputElement OutputElement;
    typedef typename DownstreamElement<PacketType>::InputElement InputElement;
protected:
    typedef typename UpstreamElement<PacketType>::InputPort InputPort;
    typedef typename DownstreamElement<PacketType>::OutputPort OutputPort;
}; // class RouterElement


} // namespace CBR

#endif //_CBR_ROUTER_ELEMENT_
