
#include "JS_JSMessage.pbj.hpp"
#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSVisible.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
#include "JSObjectStructs/JSSystemStruct.hpp"
#include "JSObjectScript.hpp"
#include "JSLogging.hpp"

/*
  FIXME: If I do not include the JS_Sirikata.pbj.hpp, then just including the
  sirikata/core/util/routablemessagebody.hpp file will cause things to blow up
  (not even using anything in that file, just including it).
 */


#include <v8.h>

namespace Sirikata{
namespace JS{



void JSSerializer::serializeFunction(v8::Local<v8::Function> v8Func, Sirikata::JS::Protocol::JSMessage& jsmessage)
{
//Sirikata::JS::Protocol::JSMessage jsmessage ;
  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();

  v8::HandleScope handle_scope;

  v8::Handle<v8::Value> name = v8Func->GetName();
  v8::String::Utf8Value msgBodyArgs1(name);
  const char* cMsgBody1 = ToCString(msgBodyArgs1);
  std::string cStrMsgBody1(cMsgBody1);

  jsf.set_name(cStrMsgBody1);

  v8::Local<v8::Value> value = v8Func->ToString();
  v8::String::Utf8Value msgBodyArgs2(value);
  const char* cMsgBody2 = ToCString(msgBodyArgs2);
  std::string cStrMsgBody2(cMsgBody2);

  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(cStrMsgBody2);

  v8::Local<v8::String> proto = v8Func->ObjectProtoToString();
  v8::String::Utf8Value msgBodyArgs3(proto);
  const char* cMsgBody3 = ToCString(msgBodyArgs3);
  std::string cStrMsgBody3(cMsgBody3);

  jsf.set_prototype(cStrMsgBody3);

}

namespace {
void serializeSystem(v8::Local<v8::Object> jsSystem, Sirikata::JS::Protocol::IJSMessage& jsmessage)
{
    std::string err_msg;

    JSSystemStruct* sys_struct = JSSystemStruct::decodeSystemStruct(jsSystem, err_msg);
    if(err_msg.size() > 0) {
        SILOG(js, error, "\n\nCould not decode system: "+ err_msg + "\n\n");
        return;
    }

    Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
    jsf.set_name(TYPEID_FIELD_NAME);
    Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    jsf_value.set_s_value(SYSTEM_TYPEID_STRING);
}

}

void JSSerializer::serializeVisible(v8::Local<v8::Object> jsVisible, Sirikata::JS::Protocol::IJSMessage& jsmessage)
{
  std::string err_msg;

  JSVisibleStruct* vstruct = JSVisibleStruct::decodeVisible(jsVisible, err_msg);
  if(err_msg.size() > 0)
  {
    SILOG(js, error, "\n\nCould not decode Visible: "+ err_msg + "\n\n");
    return ;
  }


  JSObjectScript* jsObjectScript     =           vstruct->jsObjScript;
  SpaceObjectReference* sporef       =      vstruct->sporefToListenTo;
  SpaceObjectReference* sporefVisTo  =    vstruct->sporefToListenFrom;


  // we don't want to serialize jsobjectscript for now

  // serialize SpaceObjectReference


  Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
  jsf.set_name(TYPEID_FIELD_NAME);
  Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(VISIBLE_TYPEID_STRING);

  jsf = jsmessage.add_fields();
  jsf.set_name(VISIBLE_SPACEOBJREF_STRING);
  jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(sporef->toString());

  jsf = jsmessage.add_fields();
  jsf.set_name(VISIBLE_TO_SPACEOBJREF_STRING);
  jsf_value = jsf.mutable_value();
  jsf_value.set_s_value(sporefVisTo->toString());

}


std::string JSSerializer::serializeObject(v8::Local<v8::Value> v8Val)
{

  Sirikata::JS::Protocol::JSMessage jsmessage;

  if( v8Val->IsFunction())
  {
    serializeFunction( v8::Local<v8::Function>::Cast(v8Val), jsmessage);
  }
  else if(v8Val->IsObject())
  {
    serializeObjectInternal(v8Val, jsmessage);
  }

  std::string serialized_message;
  jsmessage.SerializeToString(&serialized_message);
  return serialized_message;

}


namespace {

std::vector<String> getPropertyNames(v8::Local<v8::Object> obj) {
    std::vector<String> results;

    v8::Local<v8::Array> properties = obj->GetPropertyNames();
    for(uint32 i = 0; i < properties->Length(); i++) {
        v8::Local<v8::Value> prop_name = properties->Get(i);
          v8::String::Utf8Value prop_name_utf(prop_name);
          results.push_back(String(ToCString(prop_name_utf)));
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

}


void JSSerializer::serializeObjectInternal(v8::Local<v8::Value> v8Val, Sirikata::JS::Protocol::IJSMessage& jsmessage)
{

    v8::HandleScope handle_scope;
    //otherwise assuming it is a v8 object for now
    v8::Local<v8::Object> v8Obj = v8Val->ToObject();

    if(v8Obj->InternalFieldCount() > 0)
    {
      v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
      if(!typeidVal->IsNull() && !typeidVal->IsUndefined())
      {
         v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
         void* ptr = wrapped->Value();
         std::string* typeId = static_cast<std::string*>(ptr);
         if(typeId == NULL) return;

         std::string typeIdString = *typeId;
         if(typeIdString == VISIBLE_TYPEID_STRING)
         {
           serializeVisible(v8Obj, jsmessage);
         }
         else if(typeIdString == SYSTEM_TYPEID_STRING)
         {
           serializeSystem(v8Obj, jsmessage);
         }

         return;
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
            v8::Local<v8::Value> value = v8Func->ToString();
            v8::String::Utf8Value msgBodyArgs2(value);
            const char* cMsgBody2 = ToCString(msgBodyArgs2);
            std::string cStrMsgBody2(cMsgBody2);
            if (cStrMsgBody2.find("{ [native code] }") != String::npos)
                continue;
        }

        Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
        Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();


        // create a JSField out of this
        std::string cStrMsgBody1(prop_name);

        std::string name = cStrMsgBody1;
        /* Check if the value is an object */

        jsf.set_name(cStrMsgBody1);

        if(prop_val->IsFunction())
        {
          v8::Local<v8::Function> v8Func = v8::Local<v8::Function>::Cast(prop_val);
          v8::Local<v8::Value> value = v8Func->ToString();
          v8::String::Utf8Value msgBodyArgs2(value);
          const char* cMsgBody2 = ToCString(msgBodyArgs2);
          std::string cStrMsgBody2(cMsgBody2);

          jsf_value.set_f_value(cStrMsgBody2);
        }
        else if(prop_val->IsArray())
        {
            /* If this value is an object , then recursively call the serlialize on this */
            Sirikata::JS::Protocol::IJSMessage ijs_m = jsf_value.mutable_a_value();
            serializeObjectInternal(prop_val, ijs_m);
        }
        else if(prop_val->IsObject())
        {
            /* If this value is an object , then recursively call the serlialize on this */
            Sirikata::JS::Protocol::IJSMessage ijs_m = jsf_value.mutable_o_value();
            serializeObjectInternal(prop_val, ijs_m);
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
          v8::String::Utf8Value msgBodyArgs2(prop_val);
          const char* cMsgBody2 = ToCString(msgBodyArgs2);
          std::string s_value(cMsgBody2);
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

bool JSSerializer::deserializeObject( JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Handle<v8::Object>& deserializeTo)
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

        }
    }

    if (isSystem) {
        static String fieldname = "builtin";
        static String fieldval = "[object system]";
        v8::Local<v8::String> v8_name = v8::String::New(fieldname.c_str(), fieldname.size());
        v8::Local<v8::String> v8_val = v8::String::New(fieldval.c_str(), fieldval.size());
        deserializeTo->Set(v8_name, v8_val);
        return true;
    }

    if(isVisible)
    {

      SpaceObjectReference visibleObj;
      SpaceObjectReference visibleTo;


      std::cout << "\n\nset internal field for type id \n\n";
      for(int i = 0; i < jsmessage.fields_size(); i++)
      {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        if(jsf.name() == VISIBLE_SPACEOBJREF_STRING)
        {
          visibleObj = SpaceObjectReference(jsvalue.s_value());
          std::cout << "\n\ngot visobj\n\n";
        }
      }

      //visibleTo is always set to null spaceObjectReference
      visibleTo = SpaceObjectReference::null();


      //error if not in context, won't be able to create a new v8 object.
      //should just abort here before seg faulting.
      if (! v8::Context::InContext())
      {
          JSLOG(error, "Error deserializing visible object.  Am not inside a v8 context.  Aborting.");
          return false;
      }
      v8::Handle<v8::Context> ctx = v8::Context::GetCurrent();
      deserializeTo = jsObjScript->createVisibleObject(visibleObj,visibleTo,false, ctx);  //create
                                                                                          //the
                                                                                          //vis
                                                                                          //obj
                                                                                          //through objScript
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
            JSSerializer::deserializeObject(jsObjScript, internal_js_message, intDesObj);
            val = intDesObj;
        }
        else if(jsvalue.has_a_value())
        {
            v8::Handle<v8::Array> intDesArr = v8::Array::New();
            v8::Handle<v8::Object> intDesObj(intDesArr);
            Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.a_value();
            JSSerializer::deserializeObject(jsObjScript, internal_js_message, intDesObj);
            val = intDesObj;
        }
        else if(jsvalue.has_f_value())
        {
            v8::HandleScope handle_scope;
            val = jsObjScript->functionValue(jsvalue.f_value());
        }
        else if(jsvalue.has_d_value())
        {
            val = v8::Number::New(jsvalue.d_value());
        }
        else if(jsvalue.has_b_value())
        {
            val = v8::Boolean::New(jsvalue.b_value());
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
