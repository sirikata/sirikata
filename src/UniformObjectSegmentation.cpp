#include "ObjectSegmentation.hpp"
#include "Message.hpp"
#include "UniformObjectSegmentation.hpp"
#include <map>
#include <vector>
#include "Statistics.hpp"

/*
  Querries cseg for numbers of servers.

*/

namespace CBR
{

  /*
    Basic constructor
  */
  //      UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID servID, const Time& timing );


  UniformObjectSegmentation::UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID sID, Trace* tracer)
    //  UniformObjectSegmentation::UniformObjectSegmentation(CoordinateSegmentation* cseg, std::map<UUID,ServerID> objectToServerMap,ServerID sID)
    : mCSeg (cseg),
      mObjectToServerMap (objectToServerMap),
      mCurrentTime(Time::null())
  {
    mID = sID;
    //don't need to initialize held
    mTrace = tracer;
  }

  /*
    Destructor
  */
  UniformObjectSegmentation::~UniformObjectSegmentation()
  {
  }



  /*
    The lookup should look through
  */
  ServerID UniformObjectSegmentation::lookup(const UUID& obj_id) const
  {
    //    std::map<UUID,ServerID>::iterator iter = mInTransit.find(obj_id);
    UUID tmper = obj_id;
    std::map<UUID,ServerID>::const_iterator iter = mInTransit.find(tmper);

    //    iter = mInTransit.find(obj_id);
    if (iter == mInTransit.end())
    {
      //object isn't in transit.
      //      return mInTransit[obj_id];

      //      printf("\n**Debug in UniformObjectSegmentation.cpp's lookup: obj found. \n");

      std::map<UUID,ServerID>::const_iterator iterObjectToServerID = mObjectToServerMap.find(tmper);
      if (iterObjectToServerID == mObjectToServerMap.end())
      {
        //        printf("\n\t\t***In the master list couldn't find object either.\n");
        return -1;
      }

      //      std::cout<<"\t\t****In the master list, could find object.  Located on server:  "<<iterObjectToServerID->second<<std::endl;
      return iterObjectToServerID->second;

    }


    //printf("\n**Debug in UniformObjectSegmentation.cpp's lookup: obj in transit.\n");
    //    std::cout<<"\n**Debug in UniformObjectSegmentation.cpp's lookup: object "<<obj_id.toString()<<" in transit.\n";

    return OBJECT_IN_TRANSIT;
    //    return mObjectToServerMap[obj_id];
    //    return iter->second;
  }

  /*
    Returns the id of the server that the oseg is hosted on.
  */
  ServerID UniformObjectSegmentation::getHostServerID()
  {
    return ObjectSegmentation::mID ;
  }

  /*
    This creates an acknowledge message to be sent out through forwarder.  Acknowledge message says that this oseg now knows that it's in charge of the object obj, acknowledge message recipient is sID_to.
  */
  Message* UniformObjectSegmentation::generateAcknowledgeMessage(Object* obj,ServerID sID_to)
  {
    const UUID& obj_id = obj->uuid();
    OriginID origin;
    origin.id= (uint32)(this->getHostServerID());

    Message* oseg_change_msg = new OSegMigrateMessage(origin,  this->getHostServerID(),  sID_to, sID_to,    this->getHostServerID(),      obj_id, OSegMigrateMessage::ACKNOWLEDGE);
                                                    //origin,id_from, id_to,   messDest  messFrom   obj_id   osegaction
    //returner =  oseg_change_msg;
    return  oseg_change_msg;

  }




  /*
    Means that the server that this oseg is on now is in charge of the object with obj_id.
    Add
   */
  void UniformObjectSegmentation::addObject(const UUID& obj_id, const ServerID ourID)
  {

    if (mObjectToServerMap.find(obj_id) != mObjectToServerMap.end())
    {
      //means that object exists.  we will move it to our id.
      mObjectToServerMap[obj_id] = ourID;
    }
    else
    {
      //means that object doesn't exist.  we will create it in our id.
      mObjectToServerMap[obj_id] = ourID;
    }

  }

  /*
    If we get a message to move an object that our server holds, then we add the object's id to mInTransit.
  */
  void UniformObjectSegmentation::migrateObject(const UUID& obj_id, const ServerID new_server_id)
  {
    //first check that we hold the object that is specified.
    if (mObjectToServerMap[obj_id] == this->getHostServerID())
    {

      //log the message.
      mTrace->objectBeginMigrate(mCurrentTime,obj_id,this->getHostServerID(),new_server_id); //log it.

      //if we do, then say that the object is in transit.
      mInTransit[obj_id] = new_server_id;
    }
  }

  /*
    Behavior:
    If receives a move message, moves object in mObjectToServerMap to another server.  (If object didn't exist before trying to move it, we create it, and put it on assigned server.)
    If receives a create message and object already exists, moves object to new server.
    If receives a kill message, deletes object server pair from map.
    If receives an acknowledged message, means that can start forwarding the messages that we received.
  */
  void UniformObjectSegmentation::osegMigrateMessage(OSegMigrateMessage* msg)
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
        if (mObjectToServerMap.find(obj_id) != mObjectToServerMap.end())
        {
          //means that object exists.  we will move it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        else
        {
          //means that object doesn't exist.  we will create it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        break;

      case OSegMigrateMessage::KILL:    //msg says object has been deleted
        mObjectToServerMap.erase(obj_id);
        break;

      case OSegMigrateMessage::MOVE:     //means that an object moved from one server to another.
        if (mObjectToServerMap.find(obj_id) != mObjectToServerMap.end())
        {
          //means that object exists.  Will move it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        else
        {
          //means that object doesn't exist.  We will create it.
          mObjectToServerMap[obj_id] = serv_to;
        }
        break;

      case OSegMigrateMessage::ACKNOWLEDGE:
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

        break;

    }
  }

  /*
    Fill the messageQueueToPushTo deque by pushing outgoing messages from destServers and osegMessages to it
    Then clear osegMessages and destServers.
  */
  void UniformObjectSegmentation::tick(const Time& t, std::map<UUID,ServerID>& updated)
  {
    mCurrentTime = t;
    updated =  mFinishedMove;
    mFinishedMove.clear();
  }



}//namespace CBR
