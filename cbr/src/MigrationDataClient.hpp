/*  Sirikata
 *  MigrationDataClient.hpp
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

#ifndef _SIRIKATA_MIGRATION_DATA_CLIENT_HPP_
#define _SIRIKATA_MIGRATION_DATA_CLIENT_HPP_

#include <sirikata/cbrcore/Utility.hpp>

namespace Sirikata {

/** MigrationDataClients produce and accept chunks of data during migration.
 *  MigrationDataClient is a generic interface to allow any component of the
 *  space to participate in the migration process.
 */
class MigrationDataClient {
public:
    virtual ~MigrationDataClient() {}

    /** The tag used to uniquely identify this component. */
    virtual std::string migrationClientTag() = 0;

    /** Produce data for the migration of obj from source_server to
     *  dest_server.
     */
    virtual std::string generateMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server) = 0;

    /** Receive data for the migration of obj from source_server to
     *  dest_server.
     */
    virtual void receiveMigrationData(const UUID& obj, ServerID source_server, ServerID dest_server, const std::string& data) = 0;
};

} // namespace Sirikata

#endif //_SIRIKATA_MIGRATION_DATA_CLIENT_HPP_
