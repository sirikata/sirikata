#include "JSPresence.hpp"
#include "JSFields.hpp"
#include "../JSPresenceStruct.hpp"
#include "JSVec3.hpp"
#include "JSQuaternion.hpp"
#include <sirikata/core/transfer/URI.hpp>

using namespace v8;
namespace Sirikata
{
  namespace JS
  {
    namespace JSPresence
	{


        template<typename WithHolderType>
        JSPresenceStruct* GetTargetPresenceStruct(const WithHolderType& with_holder)
        {
            v8::Local<v8::Object> self = with_holder.Holder();
            // NOTE: See v8 bug 162 (http://code.google.com/p/v8/issues/detail?id=162)
            // The template actually generates the root objects prototype, not the root
            // itself.
            v8::Local<v8::External> wrap;
            if (self->InternalFieldCount() > 0)
                wrap = v8::Local<v8::External>::Cast(self->GetInternalField(PRESENCE_FIELD_PRESENCE));
            else
                wrap = v8::Local<v8::External>::Cast(v8::Handle<v8::Object>::Cast(self->GetPrototype())->GetInternalField(PRESENCE_FIELD_PRESENCE));

            void* ptr = wrap->Value();
            return static_cast<JSPresenceStruct*>(ptr);
        }



        //changine this function to actually do something
        //args should contain a string that can be converted to a uri
        //FIXME: Should maybe also have a callback function inside
        Handle<v8::Value> setMesh(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setMesh function.")) );

            //get the jspresencestruct object from arguments
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setMesh function.  Invalid presence struct.")) );

            //get the uri object from args
            std::string uriLocation;
            bool uriArgValid = getURI(args,uriLocation);
            if (! uriArgValid)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Oops.  You didn't really specify an appropriate uri for your mesh.")) );

            mStruct->jsObjScript->setVisualFunction(mStruct->sporef, uriLocation);

	    return v8::Undefined();
        }

        v8::Handle<v8::Value>runSimulation(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the runSimulation function. (It should probably be 'ogregraphics'.)\n\n")) );


            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in runSimulation function.  Invalid presence struct.")) );

            v8::String::Utf8Value str(args[0]);
            const char* cstr = ToCString(str);
            String simname(cstr);

            mStruct->jsObjScript->runSimulation(*(mStruct->sporef),simname);
            return v8::Undefined();
        }


        //changine this function to actually do something
        //args should contain a string that can be converted to a uri
        //FIXME: Should maybe also have a callback function inside
        Handle<v8::Value> getMesh(const v8::Arguments& args)
        {
            //get the jspresencestruct object from arguments
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);

            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getMesh function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getVisualFunction(mStruct->sporef);
        }


        v8::Handle<v8::Value>  setPosition(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setPosition function.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPosition function.  Invalid presence struct.")) );

            //get first args
            Handle<Object> posArg = ObjectCast(args[0]);

            if ( ! Vec3Validate(posArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPosition function.  Wrong argument: require a vector for new positions.")) );


            Vector3f newPos (Vec3Extract(posArg));

            mStruct->jsObjScript->setPositionFunction(mStruct->sporef, newPos);
            return v8::Undefined();

        }


        Handle<v8::Value>      getPosition(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getPosition function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getPositionFunction(mStruct->sporef);
        }



        v8::Handle<v8::Value>  setVelocity(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setVelocity function.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setVelocity function.  Invalid presence struct.")) );

            //get first args
            Handle<Object> velArg = ObjectCast(args[0]);

            if ( ! Vec3Validate(velArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setVelocity function.  Wrong argument: require a vector for new positions.")) );


            Vector3f newVel (Vec3Extract(velArg));

            mStruct->jsObjScript->setVelocityFunction(mStruct->sporef,newVel);
            return v8::Undefined();
        }


        Handle<v8::Value>      getVelocity(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getVelocity function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getVelocityFunction(mStruct->sporef);
        }



        Handle<v8::Value>      getOrientation(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getOrientation function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getOrientationFunction(mStruct->sporef);
        }

        v8::Handle<v8::Value>  setOrientation(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientation function.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Invalid presence struct.")) );

            //get first args
            Handle<Object> orientationArg = ObjectCast(args[0]);

            if ( ! QuaternionValidate(orientationArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );


            Quaternion newOrientation (QuaternionExtract(orientationArg));

            mStruct->jsObjScript->setOrientationFunction(mStruct->sporef,newOrientation);
            return v8::Undefined();
        }



        Handle<v8::Value>      getOrientationVel(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getOrientation function.  Invalid presence struct.")) );


            return mStruct->jsObjScript->getOrientationVelFunction(mStruct->sporef);
        }

        v8::Handle<v8::Value>  setOrientationVel(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to the setOrientationVel function.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Invalid presence struct.")) );

            //get first args
            Handle<Object> orientationVelArg = ObjectCast(args[0]);

            if ( ! QuaternionValidate(orientationVelArg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setOrientation function.  Wrong argument: require a quaternion for new orientation.")) );

            Quaternion newOrientationVel (QuaternionExtract(orientationVelArg));

            mStruct->jsObjScript->setOrientationVelFunction(mStruct->sporef,newOrientationVel);
            return v8::Undefined();
        }

        v8::Handle<v8::Value> setQueryAngle(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to setQueryAngle.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setQueryAngle function. Invalid presence struct.")) );

            Handle<Object> qa_arg = ObjectCast(args[0]);
            if (!NumericValidate(qa_arg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setQueryAngle function. Wrong argument: require a number for query angle.")) );

            SolidAngle new_qa(NumericExtract(qa_arg));

            mStruct->jsObjScript->setQueryAngleFunction(mStruct->sporef, new_qa);
            return v8::Undefined();

        }


        Handle<v8::Value> getScale(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in getScale function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getVisualScaleFunction(mStruct->sporef);
        }

        v8::Handle<v8::Value> setScale(const v8::Arguments& args)
        {
            if (args.Length() != 1)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("ERROR: You need to specify exactly one argument to setScale.")) );

            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale function. Invalid presence struct.")) );

            Handle<Object> scale_arg = ObjectCast(args[0]);
            if (!NumericValidate(scale_arg))
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setScale function. Wrong argument: require a number for query angle.")) );

            float new_scale = NumericExtract(scale_arg);

            mStruct->jsObjScript->setVisualScaleFunction(mStruct->sporef, new_scale);
            return v8::Undefined();
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



        JSPresenceStruct* getPresStructFromArgs(const v8::Arguments& args)
        {
            HandleScope handle_scope;
            v8::Local<v8::Object> v8Object = args.This();
            v8::Local<v8::External> wrapJSPresStructObj;
            if (v8Object->InternalFieldCount() > 0)
            {
                wrapJSPresStructObj = v8::Local<v8::External>::Cast(
                    v8Object->GetInternalField(PRESENCE_FIELD_PRESENCE));
            }
            else
            {
                wrapJSPresStructObj = v8::Local<v8::External>::Cast(
                    v8::Handle<v8::Object>::Cast(v8Object->GetPrototype())->GetInternalField(PRESENCE_FIELD_PRESENCE));
            }
            void* ptr = wrapJSPresStructObj->Value();
            JSPresenceStruct* jspres_struct = static_cast<JSPresenceStruct*>(ptr);

            if (jspres_struct == NULL)
                assert(false);

            return jspres_struct;
        }


        Handle<v8::Value> toString(const v8::Arguments& args)
        {

            JSPresenceStruct* jspres_struct = getPresStructFromArgs(args);

            if (jspres_struct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in toString of presence function.  Invalid presence struct.")) );


            // Look up for the per space data
            //for now just print the space id
            String s = jspres_struct->sporef->toString();
            return v8::String::New(s.c_str(), s.length());

        }


    }
  }
}
