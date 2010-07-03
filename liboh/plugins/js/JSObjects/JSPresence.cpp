#include "JSPresence.hpp"
#include "JSFields.hpp"

using namespace v8;
namespace Sirikata 
{
  namespace JS 
  {
    namespace JSPresence
	{

	 #define CreateInternalFieldAccessor(FieldName, FieldID, FieldType) \
	   template<typename WithHolderType>                                \
	   FieldType* Get##FieldName(const WithHolderType& with_holder)     \
	   {                                                                \
         v8::Local<v8::Object> self = with_holder.Holder();             \
		 v8::Local<v8::External> wrap;                                  \
		 if (self->InternalFieldCount() > 0)                            \
		 {                           \
           wrap = v8::Local<v8::External>::Cast(self->GetInternalField(FieldID)); \
		 } \
		 else \
		 {    \
		   wrap = v8::Local<v8::External>::Cast(v8::Handle<v8::Object>::Cast(self->GetPrototype())->GetInternalField(FieldID)); \
	     }    \
         void* ptr = wrap->Value(); \
         return static_cast<FieldType*>(ptr); \
        } \
	    
     
	  CreateInternalFieldAccessor(TargetJSObjectScript, PRESENCE_JSOBJSCRIPT_FIELD, JSObjectScript);
	  CreateInternalFieldAccessor(TargetSpaceID, PRESENCE_SPACE_ID_FIELD, SpaceID);

	  
	  
	  Handle<v8::Value> setMesh(const v8::Arguments& args)
	  {
	    return v8::Undefined();
	  }

	  Handle<v8::Value> toString(const v8::Arguments& args)
	  {
	    HandleScope handle_scope;
	    v8::Local<v8::Object> v8Object = args.This();
        v8::Local<v8::External> wrapJSObj;
        if (v8Object->InternalFieldCount() > 0)
		{
          wrapJSObj = v8::Local<v8::External>::Cast(
                  v8Object->GetInternalField(PRESENCE_JSOBJSCRIPT_FIELD));
        }
        else
		{
          wrapJSObj = v8::Local<v8::External>::Cast(
              v8::Handle<v8::Object>::Cast(v8Object->GetPrototype())->GetInternalField(PRESENCE_JSOBJSCRIPT_FIELD));
        }			  
        void* ptr = wrapJSObj->Value();
        SpaceID* space_id = static_cast<SpaceID*>(ptr);

        // Look up for the per space data
		//for now just print the space id
        String s = space_id->toString();
		return v8::String::New(s.c_str(), s.length());

	  }
      
	  //Position

	  v8::Handle<v8::Value> ScriptGetPosition(v8::Local<v8::String> property, const v8::AccessorInfo &info)
	  {
	    JSObjectScript* target_script = GetTargetJSObjectScript(info);
	    SpaceID* space = GetTargetSpaceID(info);

		return target_script->getPosition(*space);
	  }

      void ScriptSetPosition(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
      {
	    
        JSObjectScript* target_script = GetTargetJSObjectScript(info);
	    SpaceID* space = GetTargetSpaceID(info);
        target_script->setPosition(*space, value);
		
      }
      // Velocity

      v8::Handle<v8::Value> ScriptGetVelocity(v8::Local<v8::String> property, const v8::AccessorInfo &info)
      {
        JSObjectScript* target_script = GetTargetJSObjectScript(info);
	    SpaceID* space = GetTargetSpaceID(info);
        return target_script->getVelocity(*space);
      }

      void ScriptSetVelocity(v8::Local<v8::String> property, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
      {
        JSObjectScript* target_script = GetTargetJSObjectScript(info);
		SpaceID* space = GetTargetSpaceID(info);
        target_script->setVelocity(*space, value);
      }





 
    }
  }
}  
