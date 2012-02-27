
#include <v8.h>
#include "JSPresenceStruct.hpp"
#include "../EmersonScript.hpp"
#include "../JSSerializer.hpp"
#include "../JSLogging.hpp"
#include "../JSObjects/JSVec3.hpp"
#include "../JSObjects/JSQuaternion.hpp"
#include "JSPresenceStruct.hpp"
#include <boost/lexical_cast.hpp>

#include <sirikata/mesh/Bounds.hpp>
#include <sirikata/mesh/Raytrace.hpp>

namespace Sirikata {
namespace JS {

JSPositionListener::JSPositionListener(EmersonScript* parent, JSAggregateVisibleDataPtr _jpp, JSCtx* ctx)
 : mParentScript(parent),
   jpp(_jpp),
   mCtx(ctx)
{
}

JSPositionListener::~JSPositionListener()
{
}

v8::Handle<v8::Value> JSPositionListener::struct_getAllData()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getAllData);

    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = v8::Object::New();
    returner->Set(v8::String::New("sporef"), struct_getSporef());
    returner->Set(v8::String::New("pos"), struct_getPosition());
    returner->Set(v8::String::New("vel"), struct_getVelocity());
    returner->Set(v8::String::New("orient"), struct_getOrientation());
    returner->Set(v8::String::New("orientVel"), struct_getOrientationVel());
    returner->Set(v8::String::New("scale"), struct_getScale());
    returner->Set(v8::String::New("mesh"), struct_getMesh());
    returner->Set(v8::String::New("physics"), struct_getPhysics());

    returner->Set(v8::String::New("posTime"),struct_getTransTime());
    returner->Set(v8::String::New("orientTime"), struct_getOrientTime());

    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> JSPositionListener::struct_getSporef()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getSporef);
    return v8::String::New(getSporef().toString().c_str());
}

SpaceObjectReference JSPositionListener::getSporef()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getSporef,SpaceObjectReference::null());
    return jpp->id();
}

String JSPositionListener::getMesh()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getMesh,"");
    return jpp->mesh().toString();
}

v8::Handle<v8::Value> JSPositionListener::struct_getMesh()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getMesh);
    String mesh_str = getMesh();
    return v8::String::New(mesh_str.c_str(), mesh_str.size());
}

bool JSPositionListener::getStillVisible()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(funcIn,false);
    return mParentScript->jsVisMan.isVisible(jpp->id());
}

v8::Handle<v8::Value> JSPositionListener::struct_getStillVisible()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getStillVisible);
    return v8::Boolean::New(getStillVisible());
}

String JSPositionListener::getPhysics()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getPhysics,"");
    return jpp->physics();
}

v8::Handle<v8::Value> JSPositionListener::struct_getPhysics()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getVisible);
    return v8::String::New(jpp->physics().c_str(),jpp->physics().size());
}


Vector3f JSPositionListener::getPosition()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getPosition,Vector3f::zero());
    return jpp->location().position(mParentScript->getHostedTime());
}
Vector3f JSPositionListener::getVelocity()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getVelocity,Vector3f::zero());
    return jpp->location().velocity();
}

Quaternion JSPositionListener::getOrientationVelocity()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getOrientationVelocity,Quaternion::identity());
    return jpp->orientation().velocity();
}

Quaternion JSPositionListener::getOrientation()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getOrientation,Quaternion::identity());
    return jpp->orientation().position(mParentScript->getHostedTime());
}


BoundingSphere3f JSPositionListener::getBounds()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getBounds,BoundingSphere3f());
    return jpp->bounds();
}

v8::Handle<v8::Value> JSPositionListener::struct_getPosition()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getPosition,curContext);
    return CreateJSResult(curContext,getPosition());
}

v8::Handle<v8::Value> JSPositionListener::struct_getTransTime()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getTranstime);
    uint64 transTime = jpp->location().updateTime().raw();
    String convertedString;

    try
    {
        convertedString = boost::lexical_cast<String>(transTime);
    }
    catch(boost::bad_lexical_cast & blc)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in getTransTime.  Cound not convert uint64 to string.")));
    }

    return v8::String::New(convertedString.c_str(), convertedString.size());
}


v8::Handle<v8::Value> JSPositionListener::struct_getOrientTime()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getOrientTime);

    uint64 orientTime = jpp->orientation().updateTime().raw();
    String convertedString;

    try
    {
        convertedString = boost::lexical_cast<String>(orientTime);
    }
    catch(boost::bad_lexical_cast & blc)
    {
        return v8::ThrowException(v8::Exception::Error(v8::String::New("Error in getOrientTime.  Cound not convert uint64 to string.")));
    }

    return v8::String::New(convertedString.c_str(),convertedString.size());
}

v8::Handle<v8::Value> JSPositionListener::struct_getAnimationList()
{
    if (!mVisual) {
      return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call getAnimationList before loading the mesh.")));
    }

    std::vector<String> animationList;
    std::set<String> animationSet;

    Mesh::MeshdataPtr md( std::tr1::dynamic_pointer_cast<Mesh::Meshdata>(mVisual) );
    if (md) {
        for (uint32 i=0;  i < md->nodes.size(); i++) {
            Sirikata::Mesh::Node& node = md->nodes[i];

            for (std::map<String, Sirikata::Mesh::TransformationKeyFrames>::iterator it = node.animations.begin();
                 it != node.animations.end(); it++)
            {
                animationSet.insert(it->first);
            }
        }

        for (std::set<String>::iterator it = animationSet.begin(); it != animationSet.end(); it++) {
            animationList.push_back(*it);
        }
    }

    v8::HandleScope handle_scope;
    v8::Handle<v8::Object> returner = v8::Array::New(animationList.size());

    for ( uint32  i = 0; i < animationList.size(); i++) {
      returner->Set(i, v8::String::New(animationList[i].c_str(), animationList[i].size() ));
    }

    return handle_scope.Close(returner);
}

v8::Handle<v8::Value> JSPositionListener::loadMesh(
    JSContextStruct* ctx, v8::Handle<v8::Function> cb)
{
    CHECK_JPP_INIT_THROW_V8_ERROR(loadMesh);

    mCtx->mainStrand->post(
        std::tr1::bind(&JSPositionListener::eLoadMesh,this,
            ctx,v8::Persistent<v8::Function>::New(cb)),
        "JSPositionListener::eLoadMesh"
    );

    return v8::Undefined();
}

void JSPositionListener::eLoadMesh(
    JSContextStruct* ctx,v8::Persistent<v8::Function>cb)
{
    JSObjectScriptManager::MeshLoadCallback wrapped_cb =
        std::tr1::bind(&JSPositionListener::finishLoadMesh, this,
            this->livenessToken(), ctx->livenessToken(), ctx,cb, _1);

    mParentScript->manager()->loadMesh(Transfer::URI(getMesh()), wrapped_cb);
}




void JSPositionListener::finishLoadMesh(
    Liveness::Token alive, Liveness::Token ctx_alive, JSContextStruct* ctx,
    v8::Persistent<v8::Function> cb, Mesh::VisualPtr data)
{
    if (!alive || !ctx_alive) return;

    mCtx->objStrand->post(
        std::tr1::bind(&JSPositionListener::iFinishLoadMesh,this,
            alive,ctx_alive,ctx,cb,data),
        "JSPositionListener::iFinishLoadMesh"
    );
}



void JSPositionListener::iFinishLoadMesh(
    Liveness::Token alive, Liveness::Token ctx_alive, JSContextStruct* ctx,
    v8::Persistent<v8::Function> cb, Mesh::VisualPtr data)
{
    if (!alive || !ctx_alive) return;

    if (mParentScript->isStopped()) {
        JSLOG(warn, "Ignoring load mesh callback after shutdown request.");
        return;
    }

    while (!mCtx->initialized())
    {}

    mVisual = data;

    v8::Locker locker(mCtx->mIsolate);
    v8::Isolate::Scope iscope(mCtx->mIsolate);
    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx->mContext);
    TryCatch try_catch;
    mParentScript->invokeCallback(ctx, cb);
    mParentScript->postCallbackChecks();
}

namespace {
v8::Handle<v8::Value> CreateJSBBoxResult(v8::Handle<v8::Context>& ctx, const BoundingBox3f3f& bbox) {
    // Returns as a pair of Vec3s
    v8::Handle<v8::Array> js_bb = v8::Array::New();
    js_bb->Set(0, CreateJSResult(ctx, bbox.min()));
    js_bb->Set(1, CreateJSResult(ctx, bbox.max()));
    return js_bb;
}
}
v8::Handle<v8::Value> JSPositionListener::meshBounds() {
    if (!mVisual) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call meshBounds before loading the mesh.")));

    // Unfortunately, the radius part of the bounds are computed against the
    // origin. We need to get the bounds of:
    //   xform * make_uniform_size * mesh
    // The second term requires the radius of the mesh, but when it is centered
    // on the origin. Currently we need two passes, one untransformed to get the
    // scale factor, the second using the transformed data.
    double rad;
    Mesh::ComputeBounds(mVisual, NULL, &rad);

    Matrix4x4f xform = Matrix4x4f::translate(getPosition()) * Matrix4x4f::rotate(getOrientation()) * Matrix4x4f::scale(getBounds().radius()/rad);

    BoundingBox3f3f bbox;
    Mesh::ComputeBounds(mVisual, xform, &bbox);
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(meshBounds,curContext);
    return CreateJSBBoxResult(curContext, bbox);
}

v8::Handle<v8::Value> JSPositionListener::untransformedMeshBounds() {
    if (!mVisual) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call untransformedMeshBounds before loading the mesh.")));

    BoundingBox3f3f bbox;
    double rad;
    Mesh::ComputeBounds(mVisual, &bbox, &rad);
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(untransformedMeshBounds,curContext);
    // The only 'transformation' we apply is to get the initial scale of the
    // model in the correct range, i.e. we divide by rad. Here, we.
    bbox = BoundingBox3f3f(bbox.min()/rad, bbox.max()/rad);
    return CreateJSBBoxResult(curContext, bbox);
}

/* NOTE that this ONLY RAYTRACES THE MESH with NO TRANSFORMATIONS APPLIED. It is
 * in mesh space, not even object space. Wrappers in Emerson take care of
 * providing more convenient versions.
 */
v8::Handle<v8::Value> JSPositionListener::raytrace(const Vector3f& mesh_ray_start, const Vector3f& mesh_ray_dir) {
    if (!mVisual) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call raytrace before loading the mesh.")));

    // We don't have the scaled-to-unit version, so we need to compute the
    // scaling factor and pass that through to the raytrace function.
    double rad;
    Mesh::ComputeBounds(mVisual, NULL, &rad);
    Matrix4x4f to_unit = Matrix4x4f::scale(1.f/rad);

    float32 t_out; Vector3f hit_out;
    bool did_hit = Mesh::Raytrace(mVisual, to_unit, mesh_ray_start, mesh_ray_dir.normal(), &t_out, &hit_out);
    if (!did_hit) return v8::Undefined();

    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(raytrace,curContext);
    return CreateJSResult(curContext, hit_out);
}

v8::Handle<v8::Value> JSPositionListener::unloadMesh() {
    CHECK_JPP_INIT_THROW_V8_ERROR(unloadMesh);
    mVisual.reset();
    return v8::Undefined();
}



v8::Handle<v8::Value> JSPositionListener::wrapSporef(SpaceObjectReference sporef)
{
    return v8::String::New(sporef.toString().c_str());
}


v8::Handle<v8::Value> JSPositionListener::struct_checkEqual(JSPositionListener* jpl)
{
    CHECK_JPP_INIT_THROW_V8_ERROR(checkEqual);
    bool returner = (getSporef() == jpl->getSporef());
    return v8::Boolean::New(returner);
}


v8::Handle<v8::Value>JSPositionListener::struct_getVelocity()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getVelocity,curContext);
    return CreateJSResult(curContext,getVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientationVel()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getOrientationVel,curContext);
    return CreateJSResult(curContext,getOrientationVelocity());
}

v8::Handle<v8::Value> JSPositionListener::struct_getOrientation()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getOrientationVel,curContext);
    return CreateJSResult(curContext,getOrientation());
}

v8::Handle<v8::Value> JSPositionListener::struct_getScale()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getScale);
    return v8::Number::New(getBounds().radius());
}

v8::Handle<v8::Value> JSPositionListener::struct_getDistance(const Vector3d& distTo)
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getDistance);
    Vector3d curPos = Vector3d(getPosition());
    double distVal = (distTo - curPos).length();

    return v8::Number::New(distVal);
}


} //namespace JS
} //namespace Sirikata
