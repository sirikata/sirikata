#ifndef _CBR_UNIFORM_OSEG_HPP
#define _CBR_UNIFORM_OSEG_HPP

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

  class UniformObjectSegmentation : public ObjectSegmentation
  {
    private:
      CoordinateSegmentation* mCSeg; //will be used in lookup call
      std::map<UUID,ServerID> mObjectToServerMap;  //initialized with this...from object factory

      std::map<UUID,ServerID> mInTransit;//These are the objects that are in transit.
      std::map<UUID,ServerID> mFinishedMove;


    //      std::vector<Message*> osegMessagesToSend;
    //      std::vector<ServerID> destServersToSend;

      Time mCurrentTime;



    public:
    UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID servID,  Trace* tracer);
    //UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID servID);
      virtual ~UniformObjectSegmentation();

      virtual ServerID lookup(const UUID& obj_id) const;
      virtual void osegMigrateMessage(OSegMigrateMessage*);

      virtual void tick(const Time& t, std::map<UUID,ServerID>& updated);
    //      virtual void migrateMessage(MigrateMessage*);
      virtual void migrateObject(const UUID& obj_id, const ServerID new_server_id);
      virtual void addObject(const UUID& obj_id, const ServerID ourID);


      virtual Message* generateAcknowledgeMessage(const UUID& obj_id, ServerID sID_to);
      virtual ServerID getHostServerID();

  }; //end class

}//namespace CBR

#endif //ifndef
