


#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include <map>
#include <vector>
#include "Statistics.hpp"
#include "Time.hpp"

#include "oseg_dht/Bamboo.hpp"
#include "oseg_dht/gateway_prot.h"
#include "DhtObjectSegmentation.hpp"
#include "CoordinateSegmentation.hpp"
#include <sstream>
#include <string.h>
#include <stdlib.h>


namespace CBR
{

  /*
    Basic constructor
  */
  DhtObjectSegmentation::DhtObjectSegmentation (CoordinateSegmentation* cseg, std::vector<UUID*> vectorOfObjectsInitializedOnThisServer, ServerID servID,  Trace* tracer, char* dht_host, int dht_port)
    : mCSeg (cseg),
      mCurrentTime(0),
      bambooDht(dht_host, dht_port)
  {
    
    mID = servID;
    mTrace = tracer;

    Bamboo::Bamboo_val serv_id_as_dht_val;
    convert_serv_id_to_dht_val(mID, serv_id_as_dht_val);
    Bamboo::Bamboo_key obj_id_as_dht_key;
    
    //start loading the objects that are in vectorOfObjectsInitializedOnThisServer into the dht.
    for (int s=0;s < (int)vectorOfObjectsInitializedOnThisServer.size(); ++s)
    {
      convert_obj_id_to_dht_key((*vectorOfObjectsInitializedOnThisServer[s]),obj_id_as_dht_key);

      //   pushes this server id onto the dht associated with the object_id (as key)  (even if it doesn't already exist on the
      bambooDht.put(obj_id_as_dht_key, serv_id_as_dht_val);
    }
  }
    
  /*
    Destructor
  */
  DhtObjectSegmentation::~DhtObjectSegmentation()
  {
  }


  /*
    After insuring that the object isn't in transit, the lookup should querry the dht.
  */
  ServerID DhtObjectSegmentation::lookup(const UUID& obj_id) const
  {
    UUID tmper = obj_id;
    std::map<UUID,ServerID>::const_iterator iter = mInTransit.find(tmper);

    if (iter == mInTransit.end())
    {
      //if object is not in transit, lookup its location in the dht.  returns -1 if object doesn't exist.

      bamboo_get_result*  get_result;
      Bamboo::Bamboo_key dht_key;

      convert_obj_id_to_dht_key(obj_id,dht_key);
      
      get_result = bambooDht.get(dht_key);

      int numResponses = get_result->values.values_len; //number of servers that the object id is tied to.
      
      if(numResponses == 0)
      {
        return -1;  //means that we couldn't locate the object in the dht.
      }
            
      return convert_dht_val_to_server_id(get_result->values.values_val[numResponses -1].value.bamboo_value_val );
    }

    return OBJECT_IN_TRANSIT;
  }

  /*
    Returns the id of the server that this oseg is hosted on.
  */
  ServerID DhtObjectSegmentation::getHostServerID()
  {
    return ObjectSegmentation::mID ;
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  Message* DhtObjectSegmentation::generateAcknowledgeMessage(Object* obj,ServerID sID_to)
  {
    const UUID& obj_id = obj->uuid();
    OriginID origin;
    origin.id= (uint32)(this->getHostServerID());

    Message* oseg_change_msg = new OSegMigrateMessage(origin,  this->getHostServerID(),  sID_to, sID_to,    this->getHostServerID(),      obj_id, OSegMigrateMessage::ACKNOWLEDGE);
                                                    //origin,id_from, id_to,   messDest  messFrom   obj_id   osegaction
    return  oseg_change_msg;
  }


  

  /*
    Means that the server that this oseg is on now is in charge of the object with obj_id.
    Add
    1. remove all instances of this server id associated with the object_id (as key) in the dht.
    2. pushes this server id onto the dht associated with the object_id (as key)  (even if it doesn't already exist on the 
    3. perform a get on the dht getting all serverid's associated with object_id (as key)
    4. remove all of the prior values.
    5. send a message to other server saying that we have taken in the object.  (Well, this last step is actually done in the forwarder when processing migrate messages.  Not implemented here.)

   */
  void DhtObjectSegmentation::addObject(const UUID& obj_id, const ServerID ourID)
  {

    Bamboo::Bamboo_key obj_id_as_dht_key;
    convert_obj_id_to_dht_key(obj_id, obj_id_as_dht_key);

    
    Bamboo::Bamboo_val serv_id_as_dht_val;
    convert_serv_id_to_dht_val(ourID, serv_id_as_dht_val);

    
    //    1. remove all instances of this server id associated with the object_id (as key) in the dht.
    bambooDht.remove(obj_id_as_dht_key, serv_id_as_dht_val);

    //    2. pushes this server id onto the dht associated with the object_id (as key)  (even if it doesn't already exist on the
    bambooDht.put(obj_id_as_dht_key, serv_id_as_dht_val);

    //    3. perform a get on the dht getting all serverid's associated with object_id (as key)
    bamboo_get_result*  get_result;
    get_result = bambooDht.get(obj_id_as_dht_key);
    int numResponses = get_result->values.values_len; //number of servers that the object id is tied to.

    //    4. remove all of the prior values.
    for (int s=0; s < numResponses-1; ++s)
    {
      bambooDht.remove(obj_id_as_dht_key,get_result->values.values_val[s].value.bamboo_value_val );
    }
  }

  
  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
    Whatever calls this must verify that the object is on this server.
    I can do the check in the function by querrying bambooDht as well
  */
  void DhtObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {

    //log the message.
    mTrace->objectBeginMigrate(mCurrentTime,obj_id,this->getHostServerID(),new_server_id); //log it.
      
    //if we do, then say that the object is in transit.
    mInTransit[obj_id] = new_server_id;

  }


  
  /*
    Behavior:

    If receives a create message:
      1) Checks to make sure that object does not already exist in dht
      2) If it doesn't, adds the object and my server name to dht.
      3) If it does, throws an error and kills program.
      
    If receives a kill message:
      1) Gets all records of object from dht.
      2) Systematically deletes every object/server pair from dht.
      3) ????Also deletes object from inTransit (not implemented yet)???
      4) Note: if object doesn't exist, does nothing.
      
    Should never receive a move message.
      Kills program if I do.
    
    If receives an acknowledged message, it means that we can start forwarding messages that we received:
      1) Checks to see if object id was stored in inTransit.
      2) If it is, then we remove it from inTransit.
      3) Put it in mFinishedMove.
      4) Log the acknowledged message.
    
  */
  void DhtObjectSegmentation::osegMigrateMessage(OSegMigrateMessage* msg)
  {
    ServerID serv_from, serv_to;
    UUID obj_id;
    OSegMigrateMessage::OSegMigrateAction oaction;

    serv_from = msg->getServFrom();
    serv_to   = msg->getServTo();
    obj_id    = msg->getObjID();
    oaction   = msg->getAction();
    
    switch (oaction)
    {
      case OSegMigrateMessage::CREATE:      //msg says something has been added
        {
          //      1) Checks to make sure that object does not already exist in dht
          Bamboo::Bamboo_key obj_id_as_dht_key;
          convert_obj_id_to_dht_key(obj_id, obj_id_as_dht_key);

        
          Bamboo::Bamboo_val serv_id_as_dht_val;
          convert_serv_id_to_dht_val(serv_to,serv_id_as_dht_val);
        
          bamboo_get_result*  get_result;
          get_result = bambooDht.get(obj_id_as_dht_key);
          int numResponses = get_result->values.values_len; //number of servers that the object id is tied to.
        
          if (numResponses != 0)
          {
            printf("\n\nIn DhtObjectSegmentation.cpp.  Object already exists.  Performing no action.\n\n");
            assert(false); //return
            return;
          }
        
          //      2) If it doesn't, adds the object and my server name to dht.
          bambooDht.put(obj_id_as_dht_key, serv_id_as_dht_val);

        }
        break;

      case OSegMigrateMessage::KILL:
        {
          //msg says object has been deleted
          //      1) Gets all records of object from dht.
          Bamboo::Bamboo_key obj_id_as_dht_key;
          convert_obj_id_to_dht_key(obj_id, obj_id_as_dht_key);

        
          bamboo_get_result*  get_result;
          get_result = bambooDht.get(obj_id_as_dht_key);
          int numResponses = get_result->values.values_len; //number of servers that the object id is tied to.


          //      2) Systematically deletes every object/server pair from dht.
          for (int s= 0;s < numResponses; ++s)
          {
            bambooDht.remove(obj_id_as_dht_key,get_result->values.values_val[s].value.bamboo_value_val );
          }
        }
        
        break;

      case OSegMigrateMessage::MOVE:     //means that an object moved from one server to another.
        {
          //should never receive this message.
          printf("\n\nIn DhtObjectSegmentation.cpp under osegMigrateMessage.  Got a move message that I should not have.\n\n");
          assert(false);
        }
        break;

      case OSegMigrateMessage::ACKNOWLEDGE:
        {
          //When receive an acknowledge, it means that we can free the messages that we were holding for the node that's being moved.
          //remove object id from in transit.
          //place it in mFinishedMove

          std::map<UUID,ServerID>::iterator inTransIt;
        
          inTransIt = mInTransit.find(obj_id);
          if (inTransIt != mInTransit.end())
          {
            //means that we now remove the obj_id from mInTransit
            mInTransit.erase(inTransIt);
            //add it to mFinishedMove.  serv_from
            mFinishedMove[obj_id] = serv_from;

            //log reception of acknowled message
            mTrace->objectAcknowledgeMigrate(mCurrentTime, obj_id,serv_from,this->getHostServerID());

          }
        }        
        break;
    }
  }

  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.
  */
  void DhtObjectSegmentation::tick(const Time& t, std::map<UUID,ServerID>& updated)
  {
    mCurrentTime = t;
    updated =  mFinishedMove;
    mFinishedMove.clear();
  }


  /*
    //need to convert sID to a string (I believe)
  */
  void DhtObjectSegmentation::convert_serv_id_to_dht_val(ServerID sID, Bamboo::Bamboo_val& returner) const
  {
    std::stringstream ss;
    ss << sID;
    std::string tmper = ss.str();

    if (tmper.size() + 1 > BAMBOO_KEY_SIZE)
    {
      printf("\n\nToo many servers.\n\n");
      printf("\n\nPreventing overflow in DhtObjectSegmentation.cpp\n\n");
      assert(false);
    }
    
    strncpy(returner,tmper.c_str(),tmper.size()+1); //+1 so that can also encode the null character.
  }
    

  /*

  */
  void DhtObjectSegmentation::convert_obj_id_to_dht_key(const UUID& obj_id, Bamboo::Bamboo_key& returner) const
  {
    strncpy(returner,obj_id.toString().c_str(),sizeof(returner));
  }

  /*
    Takes in a bamboo value and returns a corresponding server id.  
  */
  ServerID DhtObjectSegmentation::convert_dht_val_to_server_id(Bamboo::Bamboo_val bVal) const
  {
    return (ServerID) atoi(bVal);
  }
    
  

}//namespace CBR
