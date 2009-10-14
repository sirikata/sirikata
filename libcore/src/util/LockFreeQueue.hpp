/*  Sirikata Utilities -- Sirikata Synchronization Utilities
 *  LockFreeQueue.hpp
 *
 *  Copyright (c) 2008, Daniel Reiter Horn
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


#ifndef _SIRIKATA_LOCK_FREE_QUEUE_HPP_
#define _SIRIKATA_LOCK_FREE_QUEUE_HPP_

#include "AtomicTypes.hpp"

/// LockFreeQueue.hpp
namespace Sirikata {

/// A queue of any type that has thread-safe push() and pop() functions.
template <typename T> class LockFreeQueue {
private:
    struct Node {
        volatile Node * mNext;
        T mContent;
        void *operator new(size_t num_bytes) {
            return Sirikata::aligned_malloc<Node>(num_bytes,16);
        }
        void operator delete(void *data) {
            Sirikata::aligned_free(data);
        }
        Node() :mNext(NULL), mContent() {
        }
    };
    class FreeNodePool {

        volatile Node *mHead;
    public:
        FreeNodePool() {
            mHead = new Node();
        }

        ~FreeNodePool() {
            Node *cur = const_cast<Node*>(mHead);
            Node *next = NULL;
            do {
                next = const_cast<Node*>(cur->mNext);
                delete cur;
            } while ((cur=next)!=NULL);
        }

        Node* allocate() {
            volatile Node* node=0;
            do {
                node = mHead->mNext;
                if (node == 0) {
                    return new Node();//FIXME should probably be aligned to size(Node) bytes
                }
            } while (!compare_and_swap(&mHead->mNext, node, node->mNext));
            Node * return_node=(Node*)node;//FIXME volatile cast only allowed if mContent is primitive type of pointer size or less
            return_node->mNext = NULL;
            return_node->mContent=T();
            return return_node;
        }

        void release(Node *node) {
            node->mContent = T();
            do {
                node->mNext = mHead->mNext;
            } while (!compare_and_swap(&mHead->mNext, node->mNext, node));
        }
    } mFreeNodePool;
    volatile Node *mHead;
    volatile Node *mTail;
public:
    LockFreeQueue() {
        mHead = mFreeNodePool.allocate();
        mTail = mHead;
    }
    class NodeIterator {
    private:
    	// Noncopyable
    	NodeIterator(const NodeIterator &other);
    	void operator=(const NodeIterator &other);

    	Node *mLastReturned;
    	Node *mCurrent;

    	FreeNodePool *mFreePool;

    public:
    	NodeIterator(LockFreeQueue<T> &queue)
    		: mLastReturned(queue.fork()),
    		  mCurrent(const_cast<Node*>(mLastReturned->mNext)),
    		  mFreePool(&queue.mFreeNodePool) {
    	}

    	~NodeIterator() {
    		if (mLastReturned) {
    			mFreePool->release(mLastReturned);
    		}
    		while (mCurrent) {
    			mFreePool->release(mCurrent);
    			mCurrent = const_cast<Node*>(mCurrent->mNext);
    		}
    	}

    	T *next() {
    		if (mLastReturned) {
    			mFreePool->release(mLastReturned);
    		}
    		mLastReturned = mCurrent;
    		if (mCurrent) {
    			mCurrent = const_cast<Node*>(mCurrent->mNext);
        		return &mLastReturned->mContent;
    		} else {
    			return NULL;
    		}
    	}

    };

    ~LockFreeQueue() {
        NodeIterator junk(*this);
        // release everything in the queue in ~NodeIterator
    }

private:
    friend class NodeIterator;

    Node *fork() {
    	volatile Node *newHead = mFreeNodePool.allocate();
		volatile Node *oldHead = mHead;

    	// Acquire "lock" on head, for multiple people fork()ing at once.
    	{
    		while (oldHead == 0 ||
						!compare_and_swap(&mHead, oldHead, (volatile Node*)0)) {
    			oldHead = mHead;
    		}
    	}

    	{
			volatile Node *oldTail = mTail;
			while (!compare_and_swap(&mTail, oldTail, newHead)) {
				oldTail = mTail;
			}
    	}

    	mHead = newHead;
    	return const_cast<Node*>(oldHead);
    }
public:

    /**
     * Pushes value onto the queue
     *
     * @param value  Will be copied and placed onto the end of the queue.
     */
    void push(const T &value) {
        volatile Node* formerTail = NULL;
        volatile Node* formerTailNext=NULL;

        Node* newerNode = mFreeNodePool.allocate();
        newerNode->mContent = value;
        volatile Node*newNode=newerNode;
        bool successfulAddNode = false;
        while (!successfulAddNode) {
            formerTail = mTail;
            formerTailNext = formerTail->mNext;

            if (mTail == formerTail) {
                if (formerTailNext == 0)
                    successfulAddNode = compare_and_swap(&mTail->mNext, (volatile Node*)0, newNode);
                else
                    compare_and_swap(&mTail, formerTail, formerTailNext);
            }
        }

        compare_and_swap(&mTail, formerTail, newNode);
    }

    /**
     * Pops the front value from the queue and places it in value.
     *
     * @param value  Will have the T at the front of the queue copied into it.
     * @returns      whether value was changed (if the queue had at least one item).
     */
    bool pop(T &value) {
        volatile Node* formerHead = NULL;

        bool headAlreadyAdvanced = false;
        while (!headAlreadyAdvanced) {

            formerHead = mHead;
            if (formerHead == NULL) {
            	// fork() function is operating on mTail.
            	continue;
            }
            volatile Node* formerHeadNext = formerHead->mNext;
            volatile Node*formerTail = mTail;

            if (formerHead == mHead) {
                if (formerHead == formerTail) {
                    if (formerHeadNext == NULL) {
                        value=T();
                        return false;
                    }
                    compare_and_swap(&mTail, formerTail, formerHeadNext);
                }

                else {
                    value = ((Node*)formerHeadNext)->mContent;//FIXME volatile cast only allowed if mContent is primitive type of pointer size or less
                    headAlreadyAdvanced = compare_and_swap(&mHead, formerHead, formerHeadNext);
                }
            }
        }
        mFreeNodePool.release((Node*)formerHead);//FIXME volatile cast only allowed if mContent is primitive type of pointer size or less
        return true;
    }

    void blockingPop(T &item) {
    	throw std::runtime_error(std::string("Blocking Pop not implemented!!!"));
    }
    void swap(std::deque<T>&swapWith){
        if (!swapWith.empty())
            throw std::runtime_error(std::string("Trying to swap with a nonempty queue"));            
        popAll(&swapWith);
    }
    void popAll(std::deque<T>*toPop) {
        assert (toPop->empty());
        T value;
        while (pop(value)){
            toPop->push(value);
        }
    }
    bool probablyEmpty() {
        volatile Node* formerHead = mHead;
        if (formerHead) {
            if (formerHead->mNext) {
                // fork() function is operating on mTail.
                return false;
            }
        }
        return true;
    }
};
}

#endif //_SIRIKATA_LOCK_FREE_QUEUE_HPP_
