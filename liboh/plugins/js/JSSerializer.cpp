
#include "JS_JSMessage.pbj.hpp"
#include "JSSerializer.hpp"
#include <string>
#include "JSUtil.hpp"
#include "JSObjects/JSFields.hpp"
#include "JSObjects/JSVisible.hpp"
#include "JSObjectStructs/JSVisibleStruct.hpp"
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



/*
void JSSerializer::serializeInternalFields(v8::Local<v8::Object> v8Obj, Sirikata::JS::Protocol::IJSMessage& jsmessage)
{

    v8::Local<v8::Value> typeidVal = v8Obj->GetInternalField(TYPEID_FIELD);
    if(typeidVal->IsNull() || typeidVal->IsUndefined())
    {
      // throw exection here
      return;
    }

    v8::Local<v8::External> wrapped  = v8::Local<v8::External>::Cast(typeidVal);
    void* ptr = wrapped->Value();
    std::string* typeId = static_cast<std::string*>(ptr);
    if(typeId == NULL) return;

    std::string typeIdString = *typeId;


   // v8::String::Utf8Value u(typeidVal);
   // const char* c = ToCString(u);
   // std::string typeIdString(c);

    //Sirikata::JS::Protocol::IJSField jsf = jsmessage.add_fields();
    //jsf.set_name(TYPEID_FIELD_NAME);
    //Sirikata::JS::Protocol::IJSFieldValue jsf_value = jsf.mutable_value();
    //jsf_value.set_s_value(typeIdString);

    // Add additinal fields based on the typeId
    if(typeIdString == VISIBLE_TYPEID_STRING)
    {
      serializeVisible(v8Obj, jsmessage);
    }

}
*/


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
           std::cout << "\n\n Serializing a visible \n\n";
           serializeVisible(v8Obj, jsmessage);
         }

         return;
      }
    }

    v8::Local<v8::Array> properties = v8Obj->GetPropertyNames();

    for( unsigned int i = 0; i < properties->Length(); i++)
    {
        v8::Local<v8::Value> prop_name = properties->Get(i);
        v8::Local<v8::Value> prop_val = v8Obj->Get(properties->Get(i));


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
        v8::String::Utf8Value msgBodyArgs1(prop_name);

        const char* cMsgBody1 = ToCString(msgBodyArgs1);
        std::string cStrMsgBody1(cMsgBody1);

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
        else if(prop_val->IsObject())
        {
          /* If this value is an object , then recursively call the serlialize on this */
          Sirikata::JS::Protocol::IJSMessage ijs_m = jsf_value.mutable_o_value();
          //Sirikata::JS::Protocol::JSMessage js_m = Sirikata::JS::Protocol::JSMessage(ijs_m);
          serializeObjectInternal(prop_val, ijs_m);
        }

        else if(prop_val->IsInt32())
        {
          int32_t i_value = prop_val->Int32Value();
          jsf_value.set_i_value(i_value);
        }
        else if(prop_val->IsString())
        {
          v8::String::Utf8Value msgBodyArgs2(prop_val);
          const char* cMsgBody2 = ToCString(msgBodyArgs2);
          std::string s_value(cMsgBody2);
          jsf_value.set_s_value(s_value);
        }

    }

}

/*
bool JSSerializer::deserializeObject( std::string strDecode,v8::Handle<v8::Object>& deserializeTo)
{
    Sirikata::JS::Protocol::JSMessage jsmessage;
    bool parseWorked = jsmessage.ParseFromString(strDecode);


    if (! parseWorked)
        return false; //means that we couldn't parse out the message


    for(int i = 0; i < jsmessage.fields_size(); i++)
    {

        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        const char* str = jsf.name().c_str();

        v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());
        if(jsvalue.has_s_value())
        {
            const char* str1 = jsvalue.s_value().c_str();
            v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
            deserializeTo->Set(key, val);
        }
        else if(jsvalue.has_i_value())
        {
            v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
            deserializeTo->Set(key, intval);
        }
    }

    return true;
}
*/

/*
bool JSSerializer::deserializeObject( JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
{
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        if(jsf.name() == TYPEID_FIELD_NAME)
        {
          if(jsvalue.s_value() == VISIBLE_TYPEID_STRING)
              return deserializeVisible(jsObjScript,jsmessage,deserializeTo);

        }
    }
    return deserializeRegularObject(jsObjScript,jsmessage,deserializeTo);
}

bool JSSerializer::deserializeVisible(JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
{
    deserializeTo = jsObjScript->manager()->mVisibleTemplate->NewInstance();
    deserializeTo->SetInternalField(TYPEID_FIELD, External::New(new String(VISIBLE_TYPEID_STRING)));
    //deserializeTo->SetInternalField(VISIBLE_JSOBJSCRIPT_FIELD, External::New(jsObjScript));

    SpaceObjectReference visibleObj,visibleTo;

    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

        if(jsf.name() == VISIBLE_SPACEOBJREF_STRING)
            visibleObj = SpaceObjectReference(jsvalue.s_value());
        else if(jsf.name() == VISIBLE_TO_SPACEOBJREF_STRING)
            visibleTo = SpaceObjectReference(jsvalue.s_value());

    }

    lkjs;
    JSVisibleStruct* visStruct = new JSVisibleStruct(jsObjScript,visibleObj,visibleTo,false,Vector3d());
    deserializeTo->SetInternalField(VISIBLE_JSVISIBLESTRUCT_FIELD, External::New(visStruct));

    return deserializeRegularObject(jsObjScript,jsmessage,deserializeTo);
}


bool JSSerializer::deserializeRegularObject(JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
{
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        const char* str = jsf.name().c_str();
        v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());

        if(jsvalue.has_s_value())
        {
            const char* str1 = jsvalue.s_value().c_str();
            v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
            deserializeTo->Set(key, val);
        }
        else if(jsvalue.has_i_value())
        {
            v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
            deserializeTo->Set(key, intval);
        }
    }
    return true;
}

*/



bool JSSerializer::deserializeObject( JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
{
    //check if there is a typeid field and what is the value for it
    bool isAddressable = false;
    bool isVisible = false;
    for(int i = 0; i < jsmessage.fields_size(); i++)
    {
        Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
        Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
        if(jsf.name() == TYPEID_FIELD_NAME)
        {

          if(jsvalue.s_value() == VISIBLE_TYPEID_STRING)
          {
            std::cout << "\n\nGot JSVisible\n\n";
            isVisible = true;
            break;
          }

        }
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


        const char* str = jsf.name().c_str();

        v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());

        if(jsvalue.has_s_value())
        {

          const char* str1 = jsvalue.s_value().c_str();
          v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
          deserializeTo->Set(key, val);
        }
        else if(jsvalue.has_i_value())
        {
          v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
          deserializeTo->Set(key, intval);
        }
        else if(jsvalue.has_o_value())
        {
          v8::Local<v8::Object> intDesObj = v8::Object::New();
          Sirikata::JS::Protocol::JSMessage internal_js_message = jsvalue.o_value();
          JSSerializer::deserializeObject(jsObjScript, internal_js_message, intDesObj);
          deserializeTo->Set(key, v8::Persistent<v8::Object>(intDesObj));
        }
        else if(jsvalue.has_f_value())
        {
          v8::HandleScope handle_scope;
          //const char* str1 = jsvalue.f_value().c_str();
          v8::Handle<v8::Function> func = jsObjScript->functionValue(jsvalue.f_value());
          //v8::Local<v8::String> val = v8::String::New(str1, jsvalue.f_value().size());
          deserializeTo->Set(key, func);
        }

    }

    return true;
}

// bool JSSerializer::deserializeObject( JSObjectScript* jsObjScript, Sirikata::JS::Protocol::JSMessage jsmessage,v8::Local<v8::Object>& deserializeTo)
// {
//     //check if there is a typeid field and what is the value for it
//     bool isAddressable = false;
//     bool isVisible = false;
//     for(int i = 0; i < jsmessage.fields_size(); i++)
//     {
//         Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);
//         Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();
//         if(jsf.name() == TYPEID_FIELD_NAME)
//         {

//           if(jsvalue.s_value() == VISIBLE_TYPEID_STRING)
//           {
//             isVisible = true;
//             break;
//           }

//         }
//     }


//     if(isVisible)
//     {
//       deserializeTo = jsObjScript->manager()->mVisibleTemplate->NewInstance();
//       deserializeTo->SetInternalField(TYPEID_FIELD, External::New(new std::string(VISIBLE_TYPEID_STRING)));
//       deserializeTo->SetInternalField(VISIBLE_JSOBJSCRIPT_FIELD, External::New(jsObjScript));
//     }

//     for(int i = 0; i < jsmessage.fields_size(); i++)
//     {
//         Sirikata::JS::Protocol::JSField jsf = jsmessage.fields(i);

//         Sirikata::JS::Protocol::JSFieldValue jsvalue = jsf.value();

//         if(jsf.name() == TYPEID_FIELD_NAME) { continue; }

//         const char* str = jsf.name().c_str();

//         v8::Local<v8::String> key = v8::String::New(str, jsf.name().size());

//         SpaceObjectReference visibleSpace,visibleTo;

//         if(jsvalue.has_s_value())
//         {
//           if(isVisible)
//           {
//             if(jsf.name() == VISIBLE_SPACEOBJREF_STRING)
//             {

//                 visibleSpace = jsvalue.s_value();
//                 SpaceObjectReference* sporef = new SpaceObjectReference(jsvalue.s_value());
//                 deserializeTo->SetInternalField(VISIBLE_SPACEOBJREF_FIELD, External::New(sporef));
//                 lkjs;
//             }
//             else if(jsf.name() == VISIBLE_TO_SPACEOBJREF_STRING)
//             {
//                 visibleTo = jsvalue.s_value();
//                 SpaceObjectReference* sporefVisTo = new SpaceObjectReference(jsvalue.s_value());
//                 deserializeTo->SetInternalField(VISIBLE_TO_SPACEOBJREF_FIELD, External::New(sporefVisTo));
//             }
//           }
//           else
//           {
//             const char* str1 = jsvalue.s_value().c_str();
//             v8::Local<v8::String> val = v8::String::New(str1, jsvalue.s_value().size());
//             deserializeTo->Set(key, val);
//           }

//         }
//         else if(jsvalue.has_i_value())
//         {
//             v8::Local<v8::Integer> intval = v8::Integer::New(jsvalue.i_value());
//             deserializeTo->Set(key, intval);
//         }
//     }

//     return true;
// }





} //end namespace js
} //end namespace sirikata
