#include "JSPresence.hpp"
#include "JSFields.hpp"
#include "../JSPresenceStruct.hpp"
#include "JSVec3.hpp"
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
                wrap = v8::Local<v8::External>::Cast(self->GetInternalField(PRESENCE_FIELD));
            else
                wrap = v8::Local<v8::External>::Cast(v8::Handle<v8::Object>::Cast(self->GetPrototype())->GetInternalField(PRESENCE_FIELD));

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


            mStruct->jsObjScript->setVisual(mStruct->sID, uriLocation);

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
                    
            return mStruct->jsObjScript->getVisual(mStruct->sID);
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

            mStruct->jsObjScript->setPositionFunction(mStruct->sID, newPos);
            return v8::Undefined();
            
        }
        
        
        Handle<v8::Value>      getPosition(const v8::Arguments& args)
        {
            JSPresenceStruct* mStruct = getPresStructFromArgs(args);
            if (mStruct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in setPosition function.  Invalid presence struct.")) );

            return mStruct->jsObjScript->getPositionFunction(mStruct->sID,mStruct->oref);
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
                    v8Object->GetInternalField(PRESENCE_FIELD));
            }
            else
            {
                wrapJSPresStructObj = v8::Local<v8::External>::Cast(
                    v8::Handle<v8::Object>::Cast(v8Object->GetPrototype())->GetInternalField(PRESENCE_FIELD));
            }			  
            void* ptr = wrapJSPresStructObj->Value();
            JSPresenceStruct* jspres_struct = static_cast<JSPresenceStruct*>(ptr);
            return jspres_struct;
        }

        
        Handle<v8::Value> toString(const v8::Arguments& args)
        {
            
            JSPresenceStruct* jspres_struct = getPresStructFromArgs(args);

            if (jspres_struct == NULL)
                return v8::ThrowException( v8::Exception::Error(v8::String::New("Error in toString of presence function.  Invalid presence struct.")) );

            
            // Look up for the per space data
            //for now just print the space id
            String s = jspres_struct->sID->toString();   
            return v8::String::New(s.c_str(), s.length());
              
        }

        
        //Position
        v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info)
        {
            JSPresenceStruct* pres_struct = GetTargetPresenceStruct(info);
            return pres_struct->jsObjScript->getPosition(* pres_struct->sID);
        }

        void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
        {
            JSPresenceStruct* pres_struct = GetTargetPresenceStruct(info);
            pres_struct->jsObjScript->setPosition(* (pres_struct->sID), value);
        }
        
        // Velocity
        v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info)
        {
            JSPresenceStruct* pres_struct = GetTargetPresenceStruct(info);
            return pres_struct->jsObjScript->getVelocity(* (pres_struct->sID));
        }

        void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
        {
            JSPresenceStruct* pres_struct = GetTargetPresenceStruct(info);
            pres_struct->jsObjScript->setVelocity(* (pres_struct->sID),value);
            
        }

        //decided not to use these because can't take in callback function arguments.
        // v8::Handle<v8::Value> ScriptGetVisual(v8::Local<v8::String> property, const v8::AccessorInfo &info)
        // {
        //     JSPresenceStruct* presStruct = GetTargetPresenceStruct(info);
            
        //     return presStruct->jsObjScript->getVisual(presStruct->sID);
        // }

        // void ScriptSetVisual(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
        // {
        //     JSPresenceStruct* presStruct = GetTargetPresenceStruct(info);

        //     if (!value->IsString())
        //         return;
        //         //return v8::ThrowException( v8::Exception::Error(v8::String::New("Oops.  You didn't really specify an appropriate uri for your mesh.")) );


        //     v8::String::Utf8Value newvis_str(value);
        //     if (! *newvis_str)
        //         return;
        //         //return v8::ThrowException( v8::Exception::Error(v8::String::New("Oops.  You didn't really specify an appropriate uri for your mesh.")) );

        //     Transfer::URI* newURI =  new Transfer::URI(*newvis_str);
        //     presStruct->jsObjScript->setVisual(presStruct->sID, newURI);
        //     delete newURI;
        // }


        
        
    }
  }
}  
