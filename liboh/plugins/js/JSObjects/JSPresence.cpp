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

//changine this function to actually do something
//args should contain a string that can be converted to a uri
//FIXME: Should maybe also have a callback function inside
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

v8::Handle<v8::Value>runSimulation(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the runSimulation function. (It should probably be 'ogregraphics'.)\n\n")) );


    String errorMessage = "Error in runSimulation while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    String strDecodeErrorMessage = "Error decoding string as first argument of runSimulation to jspresence.  ";
    String simname = ""; //string to decode to.
    bool decodeStrSuccessful = decodeString(args[0],simname,strDecodeErrorMessage);
    if (! decodeStrSuccessful)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(strDecodeErrorMessage.c_str(), strDecodeErrorMessage.length())) );

    return mStruct->runSimulation(simname);
}


v8::Handle<v8::Value> ScriptOnProxAddedEvent(const v8::Arguments& args)
{
    String errorMessage = "Error in ScriptOnProxAddedEvent while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onProxAdded.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onProxAdded().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);

    return mStruct->registerOnProxAddedEventHandler(cb);
}

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
    
    return jspres->distance(&vec3);
    
}

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


v8::Handle<v8::Value> ScriptOnProxRemovedEvent(const v8::Arguments& args)
{

    String errorMessage = "Error in ScriptOnProxRemovedEvent while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onProxRemoved.")) );

    v8::Handle<v8::Value> cb_val = args[0];
    if (!cb_val->IsFunction())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Invalid parameters passed to onProxRemoved().  Must contain callback function.")) );

    v8::Handle<v8::Function> cb = v8::Handle<v8::Function>::Cast(cb_val);

    return mStruct->registerOnProxRemovedEventHandler(cb);
}



        //changine this function to actually do something
        //args should contain a string that can be converted to a uri
        //FIXME: Should maybe also have a callback function inside
        Handle<v8::Value> getMesh(const v8::Arguments& args)
        {

            String errorMessage = "Error in getMesh while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->getVisualFunction();
        }


        v8::Handle<v8::Value>  setPosition(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setPosition function.")) );

            String errorMessage = "Error in getMesh while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            
            //get first args
            Handle<Object> posArg = ObjectCast(args[0]);

            if ( ! Vec3Validate(posArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPosition function.  Wrong argument: require a vector for new positions.")) );


            Vector3f newPos (Vec3Extract(posArg));

            return mStruct->setPositionFunction(newPos);
        }


        Handle<v8::Value>      getPosition(const v8::Arguments& args)
        {

            String errorMessage = "Error in getPosition while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            
            return mStruct->struct_getPosition();
        }



        v8::Handle<v8::Value>  setVelocity(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setVelocity function.")) );

            String errorMessage = "Error in setVelocity while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            //get first args
            Handle<Object> velArg = ObjectCast(args[0]);

            if ( ! Vec3Validate(velArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setVelocity function.  Wrong argument: require a vector for new positions.")) );


            Vector3f newVel (Vec3Extract(velArg));

            return mStruct->struct_setVelocity(newVel);
        }


        Handle<v8::Value> getVelocity(const v8::Arguments& args)
        {

            String errorMessage = "Error in getVelocity while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            return mStruct->getVelocityFunction();
        }



        Handle<v8::Value>      getOrientation(const v8::Arguments& args)
        {
            String errorMessage = "Error in getOrientation while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->getOrientationFunction();
        }

        v8::Handle<v8::Value>  setOrientation(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientation function.")) );

            String errorMessage = "Error in setOrientation while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            //get first args
            Handle<Object> orientationArg = ObjectCast(args[0]);

            if ( ! QuaternionValidate(orientationArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );


            Quaternion newOrientation (QuaternionExtract(orientationArg));

            return mStruct->setOrientationFunction(newOrientation);

        }


        Handle<v8::Value>      getOrientationVel(const v8::Arguments& args)
        {
            String errorMessage = "Error in getOrientationVel while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            return mStruct->getOrientationVelFunction();
        }


        v8::Handle<v8::Value>  setOrientationVel(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientationVel function.")) );


            String errorMessage = "Error in setOrientationVel while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

            
            //get first args
            Handle<Object> orientationVelArg = ObjectCast(args[0]);

            if ( ! QuaternionValidate(orientationVelArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );

            Quaternion newOrientationVel (QuaternionExtract(orientationVelArg));
            return mStruct->setOrientationVelFunction(newOrientationVel);
        }


        v8::Handle<v8::Value> setQueryAngle(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to setQueryAngle.")) );

            String errorMessage = "Error in setQueryAngle while decoding presence.  ";
            JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


            Handle<Object> qa_arg = ObjectCast(args[0]);
            if (!NumericValidate(qa_arg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setQueryAngle function. Wrong argument: require a number for query angle.")) );

            SolidAngle new_qa(NumericExtract(qa_arg));

            return mStruct->setQueryAngleFunction(new_qa);
        }


Handle<v8::Value> getScale(const v8::Arguments& args)
{
    String errorMessage = "Error in getScale while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


    return mStruct->getVisualScaleFunction();
}


v8::Handle<v8::Value> setScale(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale of JSPresence.cpp.  You need to specify exactly one argument to setScale.")) );

    String errorMessage = "Error in setScale while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );


    Handle<Object> scale_arg = ObjectCast(args[0]);
    if (!NumericValidate(scale_arg))
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale function. Wrong argument: require a number for query angle.")) );

    float new_scale = NumericExtract(scale_arg);
    
    return mStruct->setVisualScaleFunction(new_scale);
}


//Takes in args, tries to get out the first argument, which should be a
//string that we convert to a TransferURI object.  If anything fails,
//then we just return null
bool getURI(const v8::Arguments& args,std::string& returner)
{
    //assumes that the URI object is in the first 0th arg field
    Handle<Object> newVis = Handle<Object>::Cast(args[0]);
    
    //means that the argument passed was not a string identifying where
    //we could get the uri
    if (!newVis->IsString())
        return false;
    
    v8::String::Utf8Value newvis_str(newVis);
    if (! *newvis_str)
        return false;
    
    returner= std::string(*newvis_str);
    return true;
}


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



v8::Handle<v8::Value> broadcastVisible(const v8::Arguments& args)
{
    if (args.Length() != 1)
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in broadcastVisible of JSPresence.cpp.  You need to specify exactly one argument to broadcastVisible (an object that is the message you were going to send).")) );


    if (! args[0]->IsObject())
        return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in broadcastVisible of JSPresence.cpp.  You need to pass in an object to broadcast to everyone else.")) );

        
    String errorMessage = "Error in broadcastVisible while decoding presence.  ";
    JSPresenceStruct* mStruct = JSPresenceStruct::decodePresenceStruct(args.This() ,errorMessage);
            
    if (mStruct == NULL)
        return v8::ThrowException( v8::Exception::Error(v8::String::New(errorMessage.c_str(), errorMessage.length())) );

    return mStruct->struct_broadcastVisible(args[0]->ToObject());
}




} //end jspresence namespace
} //end js namespace
} //end sirikata namespace
