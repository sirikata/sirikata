/*     Iridium Utilities -- Iridium Synchronization Utilities
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
 *  * Neither the name of Iridium nor the names of its contributors may
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

/// LockFreeQueue.hpp
namespace Iridium {

/// A queue of any type that has thread-safe push() and pop() functions.
template <typename T> class LockFreeQueue {
private:
    struct Node {
        volatile Node * mNext;
        T mContent;
        Node() {
            mContent=T();
            mNext=NULL;
        }
    };
    static bool compare_and_swap(volatile Node*volatile *target, volatile Node *comperand, volatile Node * exchange){
        return __sync_bool_compare_and_swap (&target, comperand, exchange);
    }
    class FreeNodePool {

        volatile Node *mHead;
    public:
        FreeNodePool() {
            mHead = new Node();
        }

        Node* allocate() {
            volatile Node* node=NULL;
            do {
                node = mHead->mNext;
                if (node == NULL)
                    return new Node();//FIXME should probably be aligned to size(Node) bytes
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
                if (formerTailNext == NULL)
                    successfulAddNode = compare_and_swap(&mTail->mNext, NULL, newNode);
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
};
}
