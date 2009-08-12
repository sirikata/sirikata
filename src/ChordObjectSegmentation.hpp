#ifndef _CBR_CHORD_OSEG_HPP
#define _CBR_CHORD_OSEG_HPP

#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include "ServerNetwork.hpp"
#include "Utility.hpp"
#include "CoordinateSegmentation.hpp"
#include <map>
#include <vector>
/*
  Querries cseg for numbers of servers.

*/

namespace CBR
{

  class ChordObjectSegmentation : public ObjectSegmentation
  {
    private:
      CoordinateSegmentation* mCSeg; //will be used in lookup call
      std::map<UUID,ServerID> mObjectToServerMap;  //initialized with this...from object factory

      std::map<UUID,ServerID> mInTransitOrLookup;//These are the objects that are in transit or that we are looking up the server location of.
      std::map<UUID,ServerID> mFinishedMoveOrLookup; //These are the objects that just finished moving or we just found the server id for.



      std::vector<Message*> osegMessagesToSend;  //these are either lookups or migrate messages.
      std::vector<ServerID> destServersToSend; //these are the servers that they are destined for.

      Time mCurrentTime;

      void lookupMessage_lookup(OSegLookupMessage* msg);
      void lookupMessage_objectFound(OSegLookupMessage* msg);

    public:
      ChordObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID servID,  Trace* tracer);
    //UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID servID);
      virtual ~ChordObjectSegmentation();

      virtual ServerID lookup(const UUID& obj_id) const;
      virtual void osegMigrateMessage(OSegMigrateMessage*);

      virtual void tick(const Time& t, std::map<UUID,ServerID> updated);
    //      virtual void migrateMessage(MigrateMessage*);
      virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
      virtual void addObject(const UUID& obj_id, const ServerID ourID);

      virtual void getMessages(std::vector<Message*> &messToSendFromOSegToForwarder, std::vector<ServerID> &destServers );

      virtual void processLookupMessage(OSegLookupMessage* msg);

      virtual Message* generateAcknowledgeMessage(Object* obj, ServerID sID_to);
      virtual ServerID getHostServerID();

  }; //end class

}//namespace CBR

#endif //ifndef
