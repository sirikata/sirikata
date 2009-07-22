/*  Sirikata -- Persistence Services
 *  MinitransactionHandlerTest.cpp
 *
 *  Copyright (c) 2008, Stanford University
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
#ifndef _MINITRANSACTION_HANDLER_TEST_HPP_
#define _MINITRANSACTION_HANDLER_TEST_HPP_

#include "ObjectStorageTest.hpp"



typedef std::tr1::function<void()> SetupMinitransactionHandlerFunction;
typedef std::tr1::function<void()> TeardownMinitransactionHandlerFunction;
typedef std::tr1::function<Sirikata::MinitransactionHandler*(const Sirikata::String&)> CreateMinitransactionHandlerFunction;


/** Performs an ordering correctness test on a MinitransactionHandler.
 *  \param _setup function to perform any implementation specific setup
 *  \param create_handler function for creating the handler
 *  \param pl parameters to create the handler with
 *  \param _teardown function to perform any implementation specific teardown
 */
void test_minitransaction_handler_order(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                        Sirikata::String pl, TeardownMinitransactionHandlerFunction _teardown);


/** Performs a stress test on the MinitransactionHandler.  Submits many Minitransactions
 *  to the MinitransactionHandler at once, stressing the parallelism of the handler.
 *  \param _setup function to perform any implementation specific setup
 *  \param create_handler function for creating the handler
 *  \param pl parameters to create the handler with
 *  \param _teardown function to perform any implementation specific teardown
 *  \param num_trans the number of Minitransactions to submit
 *  \param num_ops the number of reads, writes, and compares in each set
 */
void stress_test_minitransaction_handler(SetupMinitransactionHandlerFunction _setup, CreateMinitransactionHandlerFunction create_handler,
                                         Sirikata::String pl, TeardownMinitransactionHandlerFunction _teardown,
                                         Sirikata::uint32 num_trans, Sirikata::uint32 num_ops);

#endif //_READ_WRITE_HANDLER_TEST_HPP_
