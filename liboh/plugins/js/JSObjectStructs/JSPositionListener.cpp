
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





JSPositionListener::JSPositionListener(JSProxyPtr _jpp)
 : jpp(_jpp)
{
}


JSPositionListener::~JSPositionListener()
{
}

void JSPositionListener::setSharedProxyDataPtr( JSProxyPtr _jpp)
{
    jpp = _jpp;
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
    return jpp->sporefToListenTo;
}

String JSPositionListener::getMesh()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getMesh,"");
    return jpp->mMesh;
}

v8::Handle<v8::Value> JSPositionListener::struct_getMesh()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getMesh);
    return v8::String::New(jpp->mMesh.c_str(),jpp->mMesh.size());
}

bool JSPositionListener::getStillVisible()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(funcIn,false);
    return jpp->emerScript->isVisible(jpp->sporefToListenTo);
}

v8::Handle<v8::Value> JSPositionListener::struct_getStillVisible()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getStillVisible);
    return v8::Boolean::New(getStillVisible());
}

String JSPositionListener::getPhysics()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getPhysics,"");
    return jpp->mPhysics;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPhysics()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getVisible);
    return v8::String::New(jpp->mPhysics.c_str(),jpp->mPhysics.size());
}


Vector3f JSPositionListener::getPosition()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getPosition,Vector3f::nil());
    return jpp->mLocation.position(jpp->emerScript->getHostedTime());
}
Vector3f JSPositionListener::getVelocity()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getVelocity,Vector3f::nil());
    return jpp->mLocation.velocity();
}

Quaternion JSPositionListener::getOrientationVelocity()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getOrientationVelocity,Quaternion::identity());
    return jpp->mOrientation.velocity();
}

Quaternion JSPositionListener::getOrientation()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getOrientation,Quaternion::identity());
    return jpp->mOrientation.position(jpp->emerScript->getHostedTime());
}


BoundingSphere3f JSPositionListener::getBounds()
{
    CHECK_JPP_INIT_THROW_LOG_CPP_ERROR(getBounds,BoundingSphere3f());
    return jpp->mBounds;
}

v8::Handle<v8::Value> JSPositionListener::struct_getPosition()
{
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(getPosition,curContext);
    return CreateJSResult(curContext,getPosition());
}

v8::Handle<v8::Value> JSPositionListener::struct_getTransTime()
{
    CHECK_JPP_INIT_THROW_V8_ERROR(getTranstime);
    uint64 transTime = jpp->mLocation.updateTime().raw();
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

    uint64 orientTime = jpp->mOrientation.updateTime().raw();
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

v8::Handle<v8::Value> JSPositionListener::loadMesh(JSContextStruct* ctx, v8::Handle<v8::Function> cb) {
    CHECK_JPP_INIT_THROW_V8_ERROR(loadMesh);

    JSObjectScriptManager::MeshLoadCallback wrapped_cb =
        std::tr1::bind(&JSPositionListener::finishLoadMesh, this,
            this->livenessToken(), ctx->livenessToken(), ctx, v8::Persistent<v8::Function>::New(cb), _1);
    jpp->emerScript->manager()->loadMesh(Transfer::URI(getMesh()), wrapped_cb);
    return v8::Undefined();
}

void JSPositionListener::finishLoadMesh(Liveness::Token alive, Liveness::Token ctx_alive, JSContextStruct* ctx, v8::Persistent<v8::Function> cb, Mesh::MeshdataPtr data) {
    if (!alive || !ctx_alive) return;

    mMeshdata = data;

    v8::HandleScope handle_scope;
    v8::Context::Scope context_scope(ctx->mContext);
    TryCatch try_catch;
    jpp->emerScript->invokeCallback(ctx, cb);
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
    if (!mMeshdata) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call meshBounds before loading the mesh.")));

    // Unfortunately, the radius part of the bounds are computed against the
    // origin. We need to get the bounds of:
    //   xform * make_uniform_size * mesh
    // The second term requires the radius of the mesh, but when it is centered
    // on the origin. Currently we need two passes, one untransformed to get the
    // scale factor, the second using the transformed data.
    double rad;
    Mesh::ComputeBounds(mMeshdata, NULL, &rad);

    Matrix4x4f xform = Matrix4x4f::translate(getPosition()) * Matrix4x4f::rotate(getOrientation()) * Matrix4x4f::scale(getBounds().radius()/rad);

    BoundingBox3f3f bbox;
    Mesh::ComputeBounds(mMeshdata, xform, &bbox);
    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(meshBounds,curContext);
    return CreateJSBBoxResult(curContext, bbox);
}

v8::Handle<v8::Value> JSPositionListener::untransformedMeshBounds() {
    if (!mMeshdata) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call untransformedMeshBounds before loading the mesh.")));

    BoundingBox3f3f bbox;
    double rad;
    Mesh::ComputeBounds(mMeshdata, &bbox, &rad);
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
    if (!mMeshdata) return v8::ThrowException(v8::Exception::Error(v8::String::New("Cannot call raytrace before loading the mesh.")));

    float32 t_out; Vector3f hit_out;
    bool did_hit = Mesh::Raytrace(mMeshdata, mesh_ray_start, mesh_ray_dir, &t_out, &hit_out);
    if (!did_hit) return v8::Undefined();

    JSPOSITION_CHECK_IN_CONTEXT_THROW_EXCEP(raytrace,curContext);
    return CreateJSResult(curContext, hit_out);

}

v8::Handle<v8::Value> JSPositionListener::unloadMesh() {
    CHECK_JPP_INIT_THROW_V8_ERROR(unloadMesh);
    mMeshdata.reset();
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
