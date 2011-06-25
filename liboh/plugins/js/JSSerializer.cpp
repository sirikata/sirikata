
#include "JS_JSMessage.pbj.hpp"
#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSVisible.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSSystemStruct.hpp"
#include "JSObjectStructs/JSPresenceStruct.hpp"
#include "JSObjectScript.hpp"
#include "EmersonScript.hpp"
#include "JSLogging.hpp"
#include "JSObjects/JSObjectsUtils.hpp"

/*
  FIXME: If I do not include the JS_Sirikata.pbj.hpp, then just including the
  sirikata/core/util/routablemessagebody.hpp file will cause things to blow up
  (not even using anything in that file, just including it).
 */


#include <v8.h>

namespace Sirikata{
namespace JS{


void annotateMessage(Sirikata::JS::Protocol::JSMessage& toAnnotate,int32 &toStampWith)
{
    toAnnotate.set_msg_id(toStampWith);
    ++toStampWith;
}
void annotateMessage(Sirikata::JS::Protocol::IJSMessage& toAnnotate, int32&toStampWith)
{
    toAnnotate.set_msg_id(toStampWith);
    ++toStampWith;
}


//Runs through all the objects in the object vector toUnmark.  For each of these
//objects, calls DeleteHiddenValue on each of them.
void JSSerializer::unmarkSerialized(ObjectVec& toUnmark)
{
    v8::HandleScope handle_scope;
    for (ObjectVecIter iter = toUnmark.begin();  iter != toUnmark.end(); ++iter)
    {
        if ((*iter).IsEmpty())
        {
            JSLOG(error, "Error in unmarkSerialized.  Got an empty value to unmark.");
            continue;
        }


        v8::Local<v8::Value> hiddenVal = (*iter)->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));
        if (hiddenVal.IsEmpty())
            JSLOG(error, "Error in unmarkSerialized.  All values in vector should have a hidden value.");
        else
            (*iter)->DeleteHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));
    }

}

//Points the jsf_value to int32ToPointTo
void JSSerializer::pointOtherObject(int32 int32ToPointTo,Sirikata::JS::Protocol::IJSFieldValue& jsf_value)
{
    jsf_value.set_loop_pointer(int32ToPointTo);
}


void JSSerializer::serializeFunction(v8::Local<v8::Function> v8Func, Sirikata::JS::Protocol::JSMessage& jsmessage,int32& toStampWith, ObjectVec& objVec)
{
    v8::HandleScope handle_scope;

    annotateMessage(jsmessage,toStampWith);

    Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();


    v8::Handle<v8::Value> name = v8Func->GetName();
    INLINE_STR_CONV(name,cStrMsgBody1,"error decoding string in serializeFunction");


    
    jsf.set_name(cStrMsgBody1);

    v8::Local<v8::Value> value = v8Func->ToString();
    INLINE_STR_CONV(value,cStrMsgBody2, "error decoding string in serializeFunction");


    Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    jsf_value.set_s_value(cStrMsgBody2);

    v8::Local<v8::String> proto = v8Func->ObjectProtoToString();
    INLINE_STR_CONV(proto,cStrMsgBody3, "error decoding string in serializeFunction");

    jsf.set_prototype(cStrMsgBody3);

}

//namespace {
//void serializeSystem(v8::Local<v8::Object> jsSystem, Sirikata::JS::Protocol::IJSMessage& jsmessage)
void JSSerializer::serializeSystem(v8::Local<v8::Object> jsSystem, Sirikata::JS::Protocol::IJSMessage& jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
    std::string err_msg;

    annotateMessage(jsmessage,toStampWith);

    JSSystemStruct* sys_struct = JSSystemStruct::decodeSystemStruct(jsSystem, err_msg);
    if(err_msg.size() > 0) {
        SILOG(js, error, "Could not decode system in JSSerializer::serializeSystem: "+ err_msg );
        return;
    }

    Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
    jsf.set_name(TYPEID_FIELD_NAME);
    Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    jsf_value.set_s_value(SYSTEM_TYPEID_STRING);
}


void JSSerializer::serializeVisible(v8::Local<v8::Object> jsVisible, Sirikata::JS::Protocol::IJSMessage&jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
  std::string err_msg;
  annotateMessage(jsmessage,toStampWith);

  JSVisibleStruct* vstruct = JSVisibleStruct::decodeVisible(jsVisible, err_msg);
  if(err_msg.size() > 0)
  {
    SILOG(js, error, "Could not decode Visible in JSSerializer::serializeVisible: "+ err_msg );
    return ;
  }

  EmersonScript* emerScript     =           vstruct->jpp->emerScript;
  SpaceObjectReference sporef       =      vstruct->getSporef();

  fillVisible(jsmessage, sporef);
}

// void JSSerializer::fillVisible(Sirikata::JS::Protocol::IJSMessage& jsmessage, const SpaceObjectReference& listenTo, const SpaceObjectReference& listenFrom) {

void JSSerializer::fillVisible(Sirikata::JS::Protocol::IJSMessage& jsmessage, const SpaceObjectReference& listenTo)
{
  // serialize SpaceObjectReference
  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
  jsf.set_name(TYPEID_FIELD_NAME);
  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(VISIBLE_TYPEID_STRING);

  jsf = jsmessage.add_fields();
  jsf.set_name(VISIBLE_SPACEOBJREF_STRING);
  jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(listenTo.toString());

}

void JSSerializer::serializePresence(v8::Local<v8::Object> jsPresence, Sirikata::JS::Protocol::IJSMessage&jsmessage,int32& toStampWith,ObjectVec& allObjs)
{
  std::string err_msg;
  annotateMessage(jsmessage,toStampWith);

  JSPresenceStruct* presStruct = JSPresenceStruct::decodePresenceStruct(jsPresence, err_msg);
  if(err_msg.size() > 0)
  {
    SILOG(js, error, "Could not decode Presence in JSSerializer::serializePresence: "+ err_msg );
    return;
  }
  fillVisible(jsmessage, presStruct->getSporef());
}

std::string JSSerializer::serializeObject(v8::Local<v8::Value> v8Val,int32 toStampWith)
{
  ObjectVec allObjs;
  Sirikata::JS::Protocol::JSMessage jsmessage;

  v8::HandleScope handleScope;

  annotateMessage(jsmessage,toStampWith);

  if( v8Val->IsFunction())
  {
      serializeFunction( v8::Local<v8::Function>::Cast(v8Val), jsmessage,toStampWith,allObjs);
  }
  else if(v8Val->IsObject())
  {
      serializeObjectInternal(v8Val, jsmessage,toStampWith,allObjs);
  }
  
  std::string serialized_message;
  jsmessage.SerializeToString(&serialized_message);

  unmarkSerialized(allObjs);
  return serialized_message;
}


void debug_printSerialized(Sirikata::JS::Protocol::JSMessage jm, String prepend)
{
    //printing breadth first:
    for (int s = 0; s < jm.fields_size(); ++s)
    {
        std::cout<<prepend<<":"<<jm.fields(s).name();
        std::cout<<"\n";
        if (jm.fields(s).has_value())
        {
            if (jm.fields(s).value().has_o_value())
                debug_printSerialized(jm.fields(s).value().o_value(), prepend+ ":"+ jm.fields(s).name()  );
            if (jm.fields(s).value().has_a_value())
                debug_printSerialized(jm.fields(s).value().a_value(), prepend + ":" + jm.fields(s).name());
            if (jm.fields(s).value().has_s_value())
                std::cout<<" s_val: "<<jm.fields(s).value().s_value()<<"\n";
        }
    }
}




std::vector<String> getPropertyNames(v8::Local<v8::Object> obj) {
    std::vector<String> results;

    v8::Local<v8::Array> properties = obj->GetPropertyNames();
    for(uint32 i = 0; i < properties->Length(); i++) {
        v8::Local<v8::Value> prop_name = properties->Get(i);
        INLINE_STR_CONV(prop_name,tmpStr, "error decoding string in getPropertyNames");
        results.push_back(tmpStr);
    }

    return results;
}

// For some reason, V8 isn't providing this, so we have to be able to figure out
// the correct list ourselves.
std::vector<String> getOwnPropertyNames(v8::Local<v8::Object> obj) {
    std::vector<String> all_props = getPropertyNames(obj);

    if (obj->GetPrototype()->IsUndefined() || obj->GetPrototype()->IsNull())
        return all_props;

    std::vector<String> results;
    std::vector<String> prototype_props = getPropertyNames(v8::Local<v8::Object>::Cast(obj->GetPrototype()));


    for(std::vector<String>::size_type i = 0; i < all_props.size(); i++) {
        if (std::find(prototype_props.begin(), prototype_props.end(), all_props[i]) == prototype_props.end())
            results.push_back(all_props[i]);
    }

    results.push_back("prototype");
    return results;
}




void JSSerializer::serializeFunctionInternal(v8::Local<v8::Function> funcToSerialize, Sirikata::JS::Protocol::IJSFieldValue& field_to_put_in, int32& toStampWith)
{
    v8::Local<v8::Value> value = funcToSerialize->ToString();
    INLINE_STR_CONV(value,cStrMsgBody2, "error decoding string in serializeFunctionInternal");
    
    Sirikata::JS::Protocol::IJSFunctionObject jsfuncObj = field_to_put_in.mutable_f_value();
    jsfuncObj.set_f_value(cStrMsgBody2);
    jsfuncObj.set_func_id(toStampWith);
    ++toStampWith;
}

void JSSerializer::annotateObject(ObjectVec& objVec, v8::Handle<v8::Object> v8Obj,int32 toStampWith)
{
    //don't annotate any of the special objects that we have.  Can't form loops
    v8Obj->SetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME), v8::Int32::New(toStampWith));
    objVec.push_back(v8Obj);
}


void JSSerializer::serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage& jsmessage,int32 & toStampWith,ObjectVec& objVec)
{
    //otherwise assuming it is a v8 object for now
    v8::Local<v8::Object> v8Obj = v8Val->ToObject();

    //stamps message
    annotateObject(objVec,v8Obj,toStampWith);
    annotateMessage(jsmessage,toStampWith);

    if(v8Obj->InternalFieldCount() > 0)
    {
        v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
        if (!typeidVal.IsEmpty())
        {
            if(!typeidVal->IsNull() && !typeidVal->IsUndefined())
            {
                v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
                void* ptr = wrapped->Value();
                std::string* typeId = static_cast<std::string*>(ptr);
                if(typeId == NULL) return;

                std::string typeIdString = *typeId;
                if (typeIdString == VISIBLE_TYPEID_STRING)
                {
                    serializeVisible(v8Obj, jsmessage,toStampWith,objVec);
                }
                else if (typeIdString == SYSTEM_TYPEID_STRING)
                {
                    serializeSystem(v8Obj, jsmessage,toStampWith,objVec);
                }
                else if (typeIdString == PRESENCE_TYPEID_STRING)
                {
                    serializePresence(v8Obj, jsmessage,toStampWith,objVec);
                }
                return;
            }
        }
    }

    std::vector<String> properties = getOwnPropertyNames(v8Obj);

    for( unsigned int i = 0; i < properties.size(); i++)
    {
        String prop_name = properties[i];
        v8::Local<v8::Value> prop_val;
        if (properties[i] == "prototype")
            prop_val = v8Obj->GetPrototype();
        else
            prop_val = v8Obj->Get( v8::String::New(properties[i].c_str(), properties[i].size()) );

        /* This is a little gross, but currently necessary. If something is
         * referring to native code, we shouldn't be shipping it. This means we
         * need to detect native code and drop the field. However, v8 doesn't
         * provide a way to check for native code. Instead, we need to check for
         * the { [native code] } definition.
         *
         * This is considered bad because of the way we are filtering instead of
         * detecting native types and . A better approach would work on the
         * whole-object level, detecting special types and converting them to an
         * appropriate form for restoration on the remote side. In some cases
         * this would be seamless (e.g. vec3) and sometimes would require
         * 'simplification' (e.g. Presence).
         */
        if(prop_val->IsFunction())
        {
            v8::Local<v8::Function> v8Func = v8::Local<v8::Function>::Cast(prop_val);
            INLINE_STR_CONV(v8Func->ToString(),cStrMsgBody2, "error decoding string in serializeObjectInternal");
            
            if (cStrMsgBody2.find("{ [native code] }") != String::npos)
                continue;
        }

        Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
        Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();

        // create a JSField out of this
        jsf.set_name(prop_name);
        
        
        /* Check if the value is a function, object, bool, etc. */
        if(prop_val->IsFunction())
        {
            v8::Local<v8::Function> v8Func = v8::Local<v8::Function>::Cast(prop_val);
            v8::Local<v8::Value> hiddenValue = v8Func->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));

            if (hiddenValue.IsEmpty())
            {
                //means that we have not already stamped this function object
                //as having been serialized.  need to serialize it now.
                //annotateObject(objVec,v8Func,toStampWith);
                serializeFunctionInternal(v8Func, jsf_value,toStampWith);
            }
            else
            {
                //we have already stamped this function object as having been
                //serialized.  Instead of serializing it again, point to
                //the old version.

                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }
        }
        else if(prop_val->IsArray())
        {
            /* If this value is an object , then recursively call the serlialize on this */
            v8::Local<v8::Array> v8Array = v8::Local<v8::Array>::Cast(prop_val);
            v8::Local<v8::Value> hiddenValue = v8Array->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));
            if (hiddenValue.IsEmpty())
            {
                Sirikata::JS::Protocol::IJSMessage ijs_m =jsf_value.mutable_a_value();
                serializeObjectInternal(v8Array, ijs_m, toStampWith,objVec);
            }
            else
            {
                //we have already stamped this function object as having been
                //serialized.  Instead of serializing it again, point to
                //the old version.
                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }
        }
        else if(prop_val->IsObject())
        {
            v8::Local<v8::Object> v8obj = v8::Local<v8::Object>::Cast(prop_val);
            v8::Local<v8::Value> hiddenValue = v8obj->GetHiddenValue(v8::String::New(JSSERIALIZER_TOKEN_FIELD_NAME));
            
            if (hiddenValue.IsEmpty()) 
            {
                //means that we have not already stamped this object, and should
                //now
                Sirikata::JS::Protocol::IJSMessage ijs_o = jsf_value.mutable_o_value();
                //recursively call serialize on this
                serializeObjectInternal(v8obj, ijs_o, toStampWith,objVec);
            }
            else
            {
                if (hiddenValue->IsInt32())
                    pointOtherObject(hiddenValue->ToInt32()->Value(),jsf_value);
                else
                    JSLOG(error,"Error in serialization.  Hidden value was not an int32");
            }
        }
        else if(prop_val->IsInt32())
        {
            int32_t i_value = prop_val->Int32Value();
            jsf_value.set_i_value(i_value);
        }
        else if(prop_val->IsUint32())
        {
            uint32 ui_value = prop_val->Uint32Value();
            jsf_value.set_ui_value(ui_value);
        }
        else if(prop_val->IsString())
        {
            INLINE_STR_CONV(prop_val,s_value, "error decoding string in serializeObjectInternal");
            jsf_value.set_s_value(s_value);
        }
        else if(prop_val->IsNumber())
        {
            float64 d_value = prop_val->NumberValue();
            jsf_value.set_d_value(d_value);
        }
        else if(prop_val->IsBoolean())
        {
            bool b_value = prop_val->BooleanValue();
            jsf_value.set_b_value(b_value);
        }
        else if(prop_val->IsDate())
        {
        }
        else if(prop_val->IsRegExp())
        {
        }
    }

}

/**
   This function runs through the map of values toFixUp, pointing them to the
   an object in labeledObjs instead.

   If a key for the toFixUp map does not exist as a key in labeledObjs, returns
   false.

   Should be run at the very end of deserializeObjectInternal
 */
bool JSSerializer::deserializePerformFixups(ObjectMap& labeledObjs, FixupMap& toFixUp)
{
    for (FixupMapIter iter = toFixUp.begin(); iter != toFixUp.end(); ++iter)
    {
        ObjectMapIter finder = labeledObjs.find(iter->first);
        if (finder == labeledObjs.end())
        {
            JSLOG(error, "error deserializing object pointing to "<< iter->first<< ". No record of that label.");
            return false;
        }
        if (iter->second.name != "prototype")
            iter->second.parent->Set(v8::String::New(iter->second.name.c_str(), iter->second.name.size()), finder->second);
        else
        {
            if (!finder->second.IsEmpty() && !finder->second->IsUndefined() && !finder->second->IsNull())
            {
                iter->second.parent->SetPrototype(finder->second);
            }
        }
//        iter->second.parent->Set(v8::String::New(iter->second.name.c_str(), iter->second.name.size()), finder->second);
    }
    return true;
}



bool JSSerializer::deserializeObject( EmersonScript* emerScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo)
{
        
    v8::HandleScope handle_scope;
    ObjectMap labeledObjs;
    FixupMap  toFixUp;
    v8::Handle<v8::Object> tmpParent;

    //if can't handle first stage of deserialization, don't even worry about
    //fixups
    labeledObjs[jsmessage.msg_id()] = deserializeTo;
    if (! deserializeObjectInternal(emerScript, jsmessage,deserializeTo, labeledObjs,toFixUp))
        return false;

    //return whether fixups worked or not.
    bool returner = deserializePerformFixups(labeledObjs,toFixUp);
    return returner;
}



bool JSSerializer::deserializeObjectInternal( EmersonScript* emerScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo, ObjectMap& labeledObjs,FixupMap& toFixUp)
{

    //check if there is a typeid field and what is the value for it
    bool isVisible = false;
    bool isSystem = false;
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        if(jsf.name() == TYPEID_FIELD_NAME) {
            if(jsvalue.s_value() == SYSTEM_TYPEID_STRING) {
                isSystem = true;
                break;
            }
            else if(jsvalue.s_value() == VISIBLE_TYPEID_STRING) {
                isVisible = true;
                break;
            }
            else if(jsvalue.s_value() == PRESENCE_TYPEID_STRING) {
                SILOG(js,fatal,"Received presence for deserialization, but they cannot be serialized! This object will likely be restored as an empty, useless object.");
            }
        }
    }

    if (isSystem) {
        static String sysFieldname = "builtin";
        static String sysFieldval = "[object system]";
        v8::Local<v8::String> v8_name = v8::String::New(sysFieldname.c_str(), sysFieldval.size());
        v8::Local<v8::String> v8_val = v8::String::New(sysFieldval.c_str(), sysFieldval.size());
        deserializeTo->Set(v8_name, v8_val);
        return true;
    }

    if(isVisible)
    {
      SpaceObjectReference visibleObj;

      for(int i = 0; i < jsmessage.fields_size(); i++)
      {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        if(jsf.name() == VISIBLE_SPACEOBJREF_STRING)
          visibleObj = SpaceObjectReference(jsvalue.s_value());

      }


      //error if not in context, won't be able to create a new v8 object.
      //should just abort here before seg faulting.
      if (! v8::Context::InContext())
      {
          JSLOG(error, "Error deserializing visible object.  Am not inside a v8 context.  Aborting.");
          return false;
      }
      v8::Handle<v8::Context> ctx = v8::Context::GetCurrent();

      //create the vis obj through objScript
      deserializeTo = emerScript->createVisiblePersistent(visibleObj,NULL,ctx);  

      return true;
    }


    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        String fieldname = jsf.name();

        
        v8::Local<v8::String> fieldkey = v8::String::New(fieldname.c_str(), fieldname.size());
        v8::Handle<v8::Value> val;

        if(jsvalue.has_s_value())
        {
            String str1 = jsvalue.s_value();
            val = v8::String::New(str1.c_str(), str1.size());
        }
        else if(jsvalue.has_i_value())
        {
            val = v8::Integer::New(jsvalue.i_value());
        }
        else if(jsvalue.has_ui_value())
        {
            val = v8::Integer::NewFromUnsigned(jsvalue.ui_value());
        }
        else if(jsvalue.has_o_value())
        {
            v8::Handle<v8::Object> intDesObj = v8::Object::New();
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.o_value();
            JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, intDesObj,labeledObjs,toFixUp);
            val = intDesObj;
            labeledObjs[jsvalue.o_value().msg_id()] = intDesObj;
        }
        else if(jsvalue.has_a_value())
        {
            v8::Handle<v8::Array> intDesArr = v8::Array::New();
            v8::Handle<v8::Object> intDesObj(intDesArr);
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.a_value();
            JSSerializer::deserializeObjectInternal(emerScript, internal_js_message, intDesObj,labeledObjs,toFixUp);
            val = intDesObj;
            labeledObjs[jsvalue.a_value().msg_id()] = intDesObj;
        }
        else if(jsvalue.has_f_value())
        {
            v8::HandleScope handle_scope;
            val = emerScript->functionValue(jsvalue.f_value().f_value());
            //add the function to already-labeled objects
            labeledObjs[jsvalue.f_value().func_id()] = v8::Handle<v8::Object>::Cast(val);
        }
        else if(jsvalue.has_d_value())
        {
            val = v8::Number::New(jsvalue.d_value());
        }
        else if(jsvalue.has_b_value())
        {
            val = v8::Boolean::New(jsvalue.b_value());
        }
        else if (jsvalue.has_loop_pointer())
        {
            toFixUp[jsvalue.loop_pointer()] =(LoopedObjPointer(deserializeTo,fieldname,jsvalue.loop_pointer()));
            continue;
        }


        if (fieldname == "prototype") {
            if (!val.IsEmpty() && !val->IsUndefined() && !val->IsNull())
                deserializeTo->SetPrototype(val);
        }
        else {
            if (!val.IsEmpty())
                deserializeTo->Set(fieldkey, val);
        }
    }

    return true;
}


} //end namespace js
} //end namespace sirikata
