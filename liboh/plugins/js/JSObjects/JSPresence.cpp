#include "JSPresence.hpp"
#include "JSFields.hpp"
#include "../JSObjectStructs/JSPresenceStruct.hpp"
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"
#include "JSInvokableObject.hpp"
#include <sirikata/core/transfer/URI.hpp>
#include "JSObjectsUtils.hpp"
#include "../JSLogging.hpp"

using namespace v8;

namespace Sirikata{
namespace JS{
namespace JSPresence{

bool isPresence(v8::Handle<v8::Value> v8Val)
{
  if( v8Val->IsNull() || v8Val->IsUndefined() || !v8Val->IsObject())
  {
    return false;
  }

  // This is an object

  v8::Handle<v8::Object>v8Obj = v8Val->ToObject();

  if (TYPEID_FIELD >= v8Obj->InternalFieldCount()) return false;

  v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
  if(typeidVal->IsNull() || typeidVal->IsUndefined())
  {
      return false;
  }

  v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
  void* ptr = wrapped->Value();
  std::string* typeId = static_cast<std::string*>(ptr);
  if(typeId == NULL) return false;

  std::string typeIdString = *typeId;

  if(typeIdString == PRESENCE_TYPEID_STRING)
  {
    return true;
  }

  return false;

}



v8::Handle<v8::Value>  pres_disconnect(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  disconnect for presence requires no arguments.")));

    String errorMessage = "Error in disconnect while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));
    
    return jspres->disconnect();
}


v8::Handle<v8::Value>  getIsConnected(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  getIsConnected for presence requires no arguments.")));

    String errorMessage = "Error in getIsConnected while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    return v8::Boolean::New(jspres->getIsConnected());
}



v8::Handle<v8::Value>  getAllData(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  getAllData for presence requires no arguments.")));

    String errorMessage = "Error in getAllData while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    return jspres->getAllData();
}

  
v8::Handle<v8::Value>  getSpace(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Getting spaceID requires no arguments.")));

    String errorMessage = "Error in getSpaceID while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    SpaceObjectReference sporef = jspres->getSporef();

    if (sporef == SpaceObjectReference::null())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  SpaceID is not defined.")));

    return v8::String::New(sporef.space().toString().c_str());
}

v8::Handle<v8::Value>  getOref(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error.  Getting objectID requires no arguments.")));

    String errorMessage = "Error in getObjectID while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    SpaceObjectReference sporef = jspres->getSporef();

    if (sporef == SpaceObjectReference::null())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error.  ObjectID is not defined.")));

    return v8::String::New(sporef.object().toString().c_str());
}



/**
Takes presence and sets its current velocity and orientation velocity to zero.
Requires no args.
*/
v8::Handle<v8::Value> pres_suspend(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: calling presence's suspend function should not take any args.")) );

    String errorMessage = "Error in suspend while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    return jspres->suspend();
}

/**
Reset's the presence's velocity and orientational velocity to what it was
before suspend was called.  If suspend had not been already called, do
nothing.  Requires no args.
*/
v8::Handle<v8::Value> pres_resume(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: calling presence's resume function should not take any args.")) );

    String errorMessage = "Error in resume while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    return jspres->resume();
}


/**
this function allows the presence to return a visible object version of
itself.  Requires no args
*/
v8::Handle<v8::Value> toVisible(const v8::Arguments& args)
{
    if (args.Length() != 0)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: calling presence's toVisible function should not take any args.")) );

    String errorMessage = "Error in toVisible while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str()) ));

    return jspres->toVisible();
}



/**
   @param String uri of the mesh to set
   Changes the mesh of the associated presence.
 */
Handle<v8::Value> setMesh(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setMesh function.")) );

    String errorMessage = "Error in setMesh while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    //get the uri object from args
    std::string uriLocation;
    bool uriArgValid = getURI(args,uriLocation);
    if (! uriArgValid)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Oops.  You didn't really specify an appropriate uri for your mesh.")) );

    return mStruct->setVisualFunction(uriLocation);
}


/**
   Loads a graphical window associated with this presence.
   @param Single argument corresponds to what type of window to open.  (Should
   probably be 'ogregraphics'.)
 */
v8::Handle<v8::Value>runSimulation(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the runSimulation function. (It should probably be 'ogregraphics'.)\n\n")) );


    String errorMessage = "Error in runSimulation while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    String strDecodeErrorMessage = "Error decoding string as first argument of runSimulation to jspresence.  ";
    String simname; //string to decode to.
    bool decodeStrSuccessful = decodeString(args[0],simname,strDecodeErrorMessage);
    if (! decodeStrSuccessful)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMessage.c_str(), strDecodeErrorMessage.length())) );

    return mStruct->runSimulation(simname);
}


/**
  Calculates the distance between this presence and a specified position vector
  @param Vec3 The position vector of the point to which to calculated distance
  @return distance to the argument
*/


v8::Handle<v8::Value>distance(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: need exactly one argument to distance method of presence")));

    String errorMessage = "Error in distance method of JSPresence.cpp.  Cannot decode presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(args.This(),errorMessage);

    if (jspres == NULL)
        return v8::ThrowException(v8::Exception::Error(v8::String::New(errorMessage.c_str(),errorMessage.length())));


    if (! args[0]->IsObject())
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in dist of JSPresence.cpp.  Argument should be an objet.")));

    v8::Handle<v8::Object> argObj = args[0]->ToObject();

    bool isVec3 = Vec3Validate(argObj);
    if (! isVec3)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid: argument to dist method of Presence needs to be a vec3")));

    Vector3d vec3 = Vec3Extract(argObj);

    return jspres->struct_getDistance(vec3);
}

/**
  Tells whether a presence is connected to the world
  @param No params
  @return returns true if presence is connected else returns false
*/


v8::Handle<v8::Value>isConnectedGetter(v8::Local<v8::String> property, const AccessorInfo& info)
{
    String errorMessage = "Error in isConnectedGetter while decoding presence.  ";
    JSPresenceStruct* jspres = JSPresenceStruct::decodePresenceStruct(info.Holder(),errorMessage);
    if (jspres == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return jspres->getIsConnectedV8();
}

void isConnectedSetter(v8::Local<v8::String> property, v8::Local<v8::Value> toSetTo,const AccessorInfo& info)
{
    String errorMessage = "Error.  Cannot write to isConnected variable.";
    JSLOG(error, errorMessage);
    //return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );
}




        //changine this function to actually do something
        //args should contain a string that can be converted to a uri
        //FIXME: Should maybe also have a callback function inside
/**
   @return A string corresponding to the URI for your current mesh.  Can pass
   this uri to setMesh functions.
 */
        Handle<v8::Value> getMesh(const v8::Arguments& args)
        {

            String errorMessage = "Error in getMesh while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->struct_getMesh();
        }


/**
   @param Vec3.  (To create a Vec3, use new util.Vec3(0,0,0).)

   Teleports this presence to the position specified by argument.
 */
        v8::Handle<v8::Value>  setPosition(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setPosition function.")) );

            String errorMessage = "Error in getMesh while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            if ( ! Vec3ValValidate(args[0]))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPosition function.  Wrong argument: require a vector for new positions.")) );

            Vector3f newPos (Vec3ValExtract(args[0]));
            return mStruct->struct_setPosition(newPos);
        }


/**
   @return Returns a vec3 corresponding to the current position of this presence.
 */
        Handle<v8::Value>      getPosition(const v8::Arguments& args)
        {

            String errorMessage = "Error in getPosition while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            return mStruct->struct_getPosition();
        }


/**
   @param Vec3.  (To create a Vec3, use new util.Vec3(0,0,0).)

   Changes the velocity of this presence to be Vec3.
 */
        v8::Handle<v8::Value>  setVelocity(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setVelocity function.")) );

            String errorMessage = "Error in setVelocity while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            //get first args
            if ( ! Vec3ValValidate(args[0]))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setVelocity function.  Wrong argument: require a vector for new positions.")) );
            Vector3f newVel (Vec3ValExtract(args[0]));

            return mStruct->struct_setVelocity(newVel);
        }

/**
   @return Returns a vec3 corresponding to the current velocity of this presence.
 */
        Handle<v8::Value> getVelocity(const v8::Arguments& args)
        {

            String errorMessage = "Error in getVelocity while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            return mStruct->struct_getVelocity();
        }


/**
   @return Returns a quaternion corresponding to the current orientation of this presence.
 */
        Handle<v8::Value>      getOrientation(const v8::Arguments& args)
        {
            String errorMessage = "Error in getOrientation while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->struct_getOrientation();
        }

/**
   @param Requires a quaternion (new util.Quaternion(0,0,0,1);).

   Changes the orientation of the presence to that associated with quaternion
   passed in.
 */
        v8::Handle<v8::Value>  setOrientation(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientation function.")) );

            String errorMessage = "Error in setOrientation while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            //get first args
            if ( ! QuaternionValValidate(args[0]))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );
            Quaternion newOrientation (QuaternionValExtract(args[0]));

            return mStruct->setOrientationFunction(newOrientation.normal());

        }


/**
   @return a number corresponding to angular velocity of presence (rads/s).
 */
        Handle<v8::Value> getOrientationVel(const v8::Arguments& args)
        {
            String errorMessage = "Error in getOrientationVel while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->struct_getOrientationVel();
        }

/**
   @param a number in rads/s

   Sets the angular velocity of the presence.
 */
        v8::Handle<v8::Value>  setOrientationVel(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientationVel function.")) );


            String errorMessage = "Error in setOrientationVel while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            if ( ! QuaternionValValidate(args[0]))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );
            Quaternion newOrientationVel (QuaternionValExtract(args[0]));

            return mStruct->setOrientationVelFunction(newOrientationVel);
        }


/**
   @param Number in sterradians.

   Sets the solid angle query associated with the presence.  Ie. asks the system
   to return all presences that appear larger than parameter passed in.
   Roughly, the higher this number is, the fewer presences will cause the function associated with
   onProxAdded to be called.  (Reasonable ranges from my experience: .1-4.
 */
        v8::Handle<v8::Value> setQueryAngle(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to setQueryAngle.")) );

            String errorMessage = "Error in setQueryAngle while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            if (!NumericValidate(args[0]))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setQueryAngle function. Wrong argument: require a number for query angle.")) );
            SolidAngle new_qa(NumericExtract(args[0]));

            return mStruct->setQueryAngleFunction(new_qa);
        }


v8::Handle<v8::Value>  getQueryAngle(const v8::Arguments& args)
{
    String errorMessage = "Error in getQueryAngle while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->struct_getQueryAngle();
}

/**
   @return A number indicating the scale of the presence.

   Returns the scale of the object.  1 is unit scale.
 */
Handle<v8::Value> getScale(const v8::Arguments& args)
{
    String errorMessage = "Error in getScale while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->struct_getScale();
}


/**
   @param A number indicating the scale of the presence.

   Change the scale of this presence.  Ie make it larger by putting in a number
   greater than its current scale, or smaller by putting in a number less than
   its current scale.
 */
v8::Handle<v8::Value> setScale(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale of JSPresence.cpp.  You need to specify exactly one argument to setScale.")) );

    String errorMessage = "Error in setScale while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


    if (!NumericValidate(args[0]))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale function. Wrong argument: require a number for query angle.")) );
    float new_scale = NumericExtract(args[0]);

    return mStruct->setVisualScaleFunction(new_scale);
}


//Takes in args, tries to get out the first argument, which should be a
//string that we convert to a TransferURI object.  If anything fails,
//then we just return null
bool getURI(const v8::Arguments& args,std::string& returner)
{
    //assumes that the URI object is in the first 0th arg field
    Handle<Value> newVis = args[0];
    String dummy;
    return decodeString(newVis,returner,dummy);
}

/**
  @return String the string representation of the presence
*/

Handle<v8::Value> toString(const v8::Arguments& args)
{
    String errorMessage = "Error in toString of JSPresence while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    // Look up for the per space data
    //for now just print the space id
    return mStruct->toString();
}


/**
   @return A string corresponding to the URI for your current mesh.  Can pass
   this uri to setMesh functions.
*/
Handle<v8::Value> getPhysics(const v8::Arguments& args)
{
    String errorMessage = "Error in getPhysics while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->getPhysicsFunction();
}

/**
   @param String containing physics settings
   Changes the physical properties of the associated presence.
*/
Handle<v8::Value> setPhysics(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setPhysics function.")) );

    String errorMessage = "Error in setPhysics while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);

    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    if (!StringValidate(args[0]))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPhysics function. Wrong argument: require a string for physical parameters.")) );
    String phy(StringExtract(args[0]));

    return mStruct->setPhysicsFunction(phy);
}


void setNullPresence(const v8::Arguments& args)
{
    v8::Handle<v8::Object> mPres = args.This();

   //grabs the internal pattern
   //(which has been saved as a pointer to JSEventHandler
   if (mPres->InternalFieldCount() > 0)
       mPres->SetInternalField(PRESENCE_FIELD_PRESENCE  ,External::New(NULL));
   else
       v8::Handle<v8::Object>::Cast(mPres->GetPrototype())->SetInternalField(PRESENCE_FIELD_PRESENCE, External::New(NULL));
}



} //end jspresence namespace
} //end js namespace
} //end sirikata namespace
