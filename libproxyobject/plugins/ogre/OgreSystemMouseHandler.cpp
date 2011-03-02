/*  Sirikata libproxyobject -- Ogre Graphics Plugin
 *  OgreSystemMouseHandler.cpp
 *
 *  Copyright (c) 2009, Patrick Reiter Horn
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  * Neither the name of Sirikata nor the names of its contributors may
 *    be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
 * OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "OgreSystemMouseHandler.hpp"
#include "OgreSystem.hpp"
#include "Camera.hpp"
#include "Lights.hpp"
#include "Entity.hpp"
#include <sirikata/proxyobject/ProxyManager.hpp>
#include <sirikata/proxyobject/ProxyObject.hpp>
#include "input/InputEvents.hpp"
#include "input/SDLInputDevice.hpp"
#include "input/SDLInputManager.hpp"
#include "input/InputManager.hpp"
#include "task/Event.hpp"
#include "task/EventManager.hpp"
#include <sirikata/core/task/Time.hpp>
#include <set>

#include <sirikata/core/util/KnownServices.hpp>
#include <sirikata/core/util/KnownMessages.hpp>
#include <sirikata/core/util/KnownScriptTypes.hpp>
#include "Protocol_JSMessage.pbj.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

namespace Sirikata {
namespace Graphics {
using namespace Input;
using namespace Task;
using namespace std;

// FIXME this needs to be documented. this used to rely on whether entities were
// cameras or not (using position instead of names). it is not clear at all how
// or why this method is supposed to be the right way to compare entities.
bool compareEntity (const Entity* one, const Entity* two)
{
    ProxyObject *pp = one->getProxyPtr().get();

    Time now = one->getScene()->simTime();
    Location loc1 = pp->globalLocation(now);

    ProxyObject* pp2;
    pp2 = two->getProxyPtr().get();
    Location loc2 = pp->globalLocation(now);

    return pp->getPhysical().name < pp2->getPhysical().name;

    return one<two;
}

void OgreSystemMouseHandler::mouseOverWebView(Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, bool mouseup) {
    Location location(cam->following()->getProxy().globalLocation(time));
    Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
    Ogre::Ray traceFrom(toOgre(location.getPosition(), mParent->getOffset()), toOgre(dir));
    ProxyObjectPtr obj(mMouseDownObject.lock());
    Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
    if (mMouseDownTri.intersected && ent) {
        Entity *me = ent;
        IntersectResult res = mMouseDownTri;
        res.distance = 1.0e38;
/* fixme */
        Ogre::Node *node = ent->getSceneNode();
        const Ogre::Vector3 &position = node->_getDerivedPosition();
        const Ogre::Quaternion &orient = node->_getDerivedOrientation();
        const Ogre::Vector3 &scale = node->_getDerivedScale();
        Triangle newt = res.tri;
        newt.v1.coord = (orient * (newt.v1.coord * scale)) + position;
        newt.v2.coord = (orient * (newt.v2.coord * scale)) + position;
        newt.v3.coord = (orient * (newt.v3.coord * scale)) + position;
/* */
        OgreMesh::intersectTri(OgreMesh::transformRay(ent->getSceneNode(), traceFrom), res, &newt, true); // &res.tri
    }
}

Entity* OgreSystemMouseHandler::hoverEntity (Camera *cam, Time time, float xPixel, float yPixel, bool mousedown, int *hitCount,int which) {
    Location location(cam->following()->getProxy().globalLocation(time));
    Vector3f dir (pixelToDirection(cam, location.getOrientation(), xPixel, yPixel));
    SILOG(input,detailed,"OgreSystemMouseHandler::hoverEntity: X is "<<xPixel<<"; Y is "<<yPixel<<"; pos = "<<location.getPosition()<<"; dir = "<<dir);

    double dist;
    Vector3f normal;
    IntersectResult res;
    int subent=-1;
    Ogre::Ray traceFrom(toOgre(location.getPosition(), mParent->getOffset()), toOgre(dir));
    Entity *mouseOverEntity = mParent->internalRayTrace(traceFrom, false, *hitCount, dist, normal, subent, &res, mousedown, which);
    if (mousedown && mouseOverEntity) {
        Entity *me = mouseOverEntity;
        if (me) {
            mMouseDownTri = res;
            mMouseDownObject = me->getProxyPtr();
            mMouseDownSubEntity = subent;
        }
    }
    if (mouseOverEntity) {
        while (
            mouseOverEntity->getProxy().getParentProxy() &&
            !(mouseOverEntity->getProxy().getParentProxy()->getObjectReference() == mCurrentGroup)) {
            mouseOverEntity = mParent->getEntity(mouseOverEntity->getProxy().getParentProxy()->getObjectReference());
            if (mouseOverEntity == NULL) {
                return NULL; // FIXME: should try again.
            }
        }
        return mouseOverEntity;
    }
    return NULL;
}

void OgreSystemMouseHandler::clearSelection() {
    for (SelectedObjectSet::const_iterator selectIter = mSelectedObjects.begin();
         selectIter != mSelectedObjects.end(); ++selectIter) {
        ProxyObjectPtr obj(selectIter->lock());
        Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (ent) {
            ent->setSelected(false);
        }
        // Fire deselected event.
    }
    mSelectedObjects.clear();
}


bool OgreSystemMouseHandler::recentMouseInRange(float x, float y, float *lastX, float *lastY) {
    float delx = x-*lastX;
    float dely = y-*lastY;

    if (delx<0) delx=-delx;
    if (dely<0) dely=-dely;
    if (delx>.03125||dely>.03125) {
        *lastX=x;
        *lastY=y;

        return false;
    }
    return true;
}

void OgreSystemMouseHandler::selectObjectAction(Vector2f p, int direction) {
    if (!mParent||!mParent->mPrimaryCamera) return;
    Camera *camera = mParent->mPrimaryCamera;
    Time time = mParent->simTime();

    if (!camera) {
        return;
    }
    if (mParent->mInputManager->isModifierDown(Input::MOD_SHIFT)) {
        // add object.
        int numObjectsUnderCursor=0;
        Entity *mouseOver = hoverEntity(camera, time, p.x, p.y, false, &numObjectsUnderCursor, mWhichRayObject);
        if (!mouseOver) {
            return;
        }
        if (mouseOver->id() == mLastShiftSelected && numObjectsUnderCursor==mLastHitCount ) {
            SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
            if (selectIter != mSelectedObjects.end()) {
                ProxyObjectPtr obj(selectIter->lock());
                Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
                if (ent) {
                    ent->setSelected(false);
                }
                mSelectedObjects.erase(selectIter);
            }
            mWhichRayObject+=direction;
            mLastShiftSelected = SpaceObjectReference::null();
        }else {
            mWhichRayObject=0;
        }
        mouseOver = hoverEntity(camera, time, p.x, p.y, false, &mLastHitCount, mWhichRayObject);
        if (!mouseOver) {
            return;
        }

        SelectedObjectSet::iterator selectIter = mSelectedObjects.find(mouseOver->getProxyPtr());
        if (selectIter == mSelectedObjects.end()) {
            SILOG(input,info,"Added selection " << mouseOver->id());
            mSelectedObjects.insert(mouseOver->getProxyPtr());
            mouseOver->setSelected(true);
            mLastShiftSelected = mouseOver->id();
            // Fire selected event.
        }
        else {
            ProxyObjectPtr obj(selectIter->lock());
            Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
            if (ent) {
                SILOG(input,info,"Deselected " << ent->id());
                ent->setSelected(false);
            }
            mSelectedObjects.erase(selectIter);
            // Fire deselected event.
        }
    }
    else if (mParent->mInputManager->isModifierDown(Input::MOD_CTRL)) {
        SILOG(input,info,"Cleared selection");
        clearSelection();
        mLastShiftSelected = SpaceObjectReference::null();
    }
    else {
        // reset selection.
        clearSelection();
        //mWhichRayObject+=direction;
        int numObjectsUnderCursor=0;
        Entity *mouseOver = hoverEntity(camera, time, p.x, p.y, true, &numObjectsUnderCursor, mWhichRayObject);
        if (recentMouseInRange(p.x, p.y, &mLastHitX, &mLastHitY)==false||numObjectsUnderCursor!=mLastHitCount){
            mouseOver = hoverEntity(camera, time, p.x, p.y, true, &mLastHitCount, mWhichRayObject=0);
        }
        if (mouseOver) {
            mSelectedObjects.insert(mouseOver->getProxyPtr());
            mouseOver->setSelected(true);
            SILOG(input,info,"Replaced selection with " << mouseOver->id() << " : " << mouseOver->getOgrePosition() );
            // Fire selected event.
        }
        mLastShiftSelected = SpaceObjectReference::null();
    }
    return;
}

///////////////// KEYBOARD HANDLERS /////////////////

void OgreSystemMouseHandler::groupObjectsAction() {
    if (mSelectedObjects.size()<2) {
        return;
    }
    if (!mParent->mPrimaryCamera) {
        return;
    }
    SpaceObjectReference parentId = mCurrentGroup;

    ProxyManager *proxyMgr = mParent->mPrimaryCamera->following()->getProxy().getProxyManager();
    Time now = mParent->simTime();
    for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
         iter != mSelectedObjects.end(); ++iter) {
        ProxyObjectPtr obj(iter->lock());
        Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (!ent) continue;
        if (ent->getProxy().getProxyManager() != proxyMgr) {
            SILOG(input,error,"Attempting to group objects owned by different proxy manager!");
            return;
        }
        if (!(ent->getProxy().getParentProxy()->getObjectReference() == parentId)) {
            SILOG(input,error,"Multiple select "<< ent->id() <<
                " has parent  "<<ent->getProxy().getParentProxy()->getObjectReference() << " instead of " << mCurrentGroup);
            return;
        }
    }
    Vector3d totalPosition (averageSelectedPosition(now, mSelectedObjects.begin(), mSelectedObjects.end()));
    Location totalLocation (totalPosition,Quaternion::identity(),Vector3f(0,0,0),Vector3f(0,0,0),0);
    Entity *parentEntity = mParent->getEntity(parentId);
    if (parentEntity) {
        totalLocation = parentEntity->getProxy().globalLocation(now);
        totalLocation.setPosition(totalPosition);
    }

    SpaceObjectReference newParentId = SpaceObjectReference(mCurrentGroup.space(), ObjectReference(UUID::random()));
    //proxyMgr->createObject(ProxyObjectPtr(new ProxyMeshObject(proxyMgr, newParentId, mParent->getPrimaryCamera()->getProxy().odp())),mParent->getPrimaryCamera()->getProxy().getQueryTracker());
    Entity *newParentEntity = mParent->getEntity(newParentId);
    //newParentEntity->getProxy().resetLocation(now, totalLocation);

    if (parentEntity) {
        //newParentEntity->getProxy().setParent(parentEntity->getProxyPtr(), now);
    }
    for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
         iter != mSelectedObjects.end(); ++iter) {
        ProxyObjectPtr obj(iter->lock());
        Entity *ent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (!ent) continue;
        //ent->getProxy().setParent(newParentEntity->getProxyPtr(), now);
        ent->setSelected(false);
    }
    mSelectedObjects.clear();
    mSelectedObjects.insert(newParentEntity->getProxyPtr());
    newParentEntity->setSelected(true);
}

bool OgreSystemMouseHandler::doUngroupObjects(Time now) {
    int numUngrouped = 0;
    SelectedObjectSet newSelectedObjects;
    for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
         iter != mSelectedObjects.end(); ++iter) {
        ProxyObjectPtr obj(iter->lock());
        Entity *parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (!parentEnt) {
            continue;
        }
        ProxyManager *proxyMgr = parentEnt->getProxy().getProxyManager();
        ProxyObjectPtr parentParent (parentEnt->getProxy().getParentProxy());
        mCurrentGroup = parentEnt->getProxy().getParentProxy()->getObjectReference(); // parentParent may be NULL.
        bool hasSubObjects = false;
        for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
            hasSubObjects = true;
            Entity *ent = *subIter;
            //ent->getProxy().setParent(parentParent, now);
            newSelectedObjects.insert(ent->getProxyPtr());
            ent->setSelected(true);
        }
        if (hasSubObjects) {
            parentEnt->setSelected(false);
            proxyMgr->destroyObject(parentEnt->getProxyPtr());
            parentEnt = NULL; // dies.
            numUngrouped++;
        }
        else {
            newSelectedObjects.insert(parentEnt->getProxyPtr());
        }
    }
    mSelectedObjects.swap(newSelectedObjects);
    return (numUngrouped>0);
}

void OgreSystemMouseHandler::ungroupObjectsAction() {
    Time now = mParent->simTime();
    doUngroupObjects(now);
}

void OgreSystemMouseHandler::enterObjectAction() {
    Time now = mParent->simTime();
    if (mSelectedObjects.size() != 1) {
        return;
    }
    Entity *parentEnt = NULL;
    for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
         iter != mSelectedObjects.end(); ++iter) {
        ProxyObjectPtr obj(iter->lock());
        parentEnt = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
    }
    if (parentEnt) {
        SelectedObjectSet newSelectedObjects;
        bool hasSubObjects = false;
        for (SubObjectIterator subIter (parentEnt); !subIter.end(); ++subIter) {
            hasSubObjects = true;
            Entity *ent = *subIter;
            newSelectedObjects.insert(ent->getProxyPtr());
            ent->setSelected(true);
        }
        if (hasSubObjects) {
            mSelectedObjects.swap(newSelectedObjects);
            mCurrentGroup = parentEnt->id();
        }
    }
}

void OgreSystemMouseHandler::leaveObjectAction() {
    Time now = mParent->simTime();
    for (SelectedObjectSet::iterator iter = mSelectedObjects.begin();
         iter != mSelectedObjects.end(); ++iter) {
        ProxyObjectPtr obj(iter->lock());
        Entity *selent = obj ? mParent->getEntity(obj->getObjectReference()) : NULL;
        if (selent) {
            selent->setSelected(false);
        }
    }
    mSelectedObjects.clear();
    Entity *ent = mParent->getEntity(mCurrentGroup);
    if (ent) {
        mCurrentGroup = ent->getProxy().getParentProxy()->getObjectReference();
        Entity *parentEnt = mParent->getEntity(mCurrentGroup);
        if (parentEnt) {
            mSelectedObjects.insert(parentEnt->getProxyPtr());
        }
    }
    else {
        mCurrentGroup = SpaceObjectReference::null();
    }
}

/** Create a UI element using a web view. */
void OgreSystemMouseHandler::createUIAction(const String& ui_page) {
    WebView* ui_wv = WebViewManager::getSingleton().createWebView("__object", "__object", 300, 300, OverlayPosition(RP_BOTTOMCENTER));
    ui_wv->loadFile(ui_page);

}

/** Create a UI element for interactively scripting an object.
    Sends a message on KnownServices port LISTEN_FOR_SCRIPT_BEGIN to the
    HostedObject.
*/
void OgreSystemMouseHandler::createScriptingUIAction() {
    static bool onceInitialized = false;

    // Ask all the objects to initialize scripting
    initScriptOnSelectedObjects();

    // Then bring up windows for each of them
    for(SelectedObjectSet::iterator sel_it = mSelectedObjects.begin(); sel_it != mSelectedObjects.end(); sel_it++) {
        ProxyObjectPtr obj(sel_it->lock());
        if (!obj) continue;

        SpaceObjectReference objid = obj->getObjectReference();

        ObjectUIMap::iterator ui_it = mObjectUIs.find(objid);
        if (ui_it == mObjectUIs.end()) {
            mObjectUIs.insert( ObjectUIMap::value_type(objid, UIInfo()) );
            ui_it = mObjectUIs.find(objid);
        }
        UIInfo& ui_info = ui_it->second;

        if (ui_info.scripting != NULL) {
            // Already there, just make sure its showing
            ui_info.scripting->show();
        }
        else {
            OverlayPosition op = onceInitialized ? RP_BOTTOMCENTER : RP_TOPLEFT;
            onceInitialized = true;

            WebView* new_scripting_ui =
                WebViewManager::getSingleton().createWebView(
                    String("__scripting") + objid.toString(), "__scripting", 300, 300,
                    OverlayPosition(RP_BOTTOMCENTER)
                );
            new_scripting_ui->loadFile("scripting/prompt.html");

            ui_info.scripting = new_scripting_ui;
            mScriptingUIObjects[new_scripting_ui] = obj;
            mScriptingUIWebViews[obj->getObjectReference()] = new_scripting_ui;
            new_scripting_ui->bind("event", std::tr1::bind(&OgreSystemMouseHandler::executeScript, this, _1, _2));
        }
    }
}


void OgreSystemMouseHandler::onUploadObjectEvent(WebView* webview, const JSArguments& args) {
    printf("upload object event fired arg length = %d\n", (int)args.size());
    if (args.size() != 3) {
        printf("expected 3 arguments, returning.\n");
        return;
    }

    String file_path(args[0].data());
    String title(args[1].data());
    String description(args[2].data());

    printf("Upload request. path = '%s' , title = '%s' , desc = '%s' .\n", file_path.c_str(), title.c_str(), description.c_str());
    WebViewManager::getSingleton().destroyWebView(mUploadWebView);
    mUploadWebView = NULL;
}

void OgreSystemMouseHandler::startUploadObject() {
    if(mUploadWebView) {
        printf("startUploadObject called. Focusing existing.\n");
        mUploadWebView->focus();
    } else {
        printf("startUploadObject called. Opening upload UI.\n");
        mUploadWebView = WebViewManager::getSingleton().createWebView("upload_tool", "upload_tool",404, 227,
            OverlayPosition(RP_CENTER), false, 70, TIER_FRONT);
        mUploadWebView->bind("event", std::tr1::bind(&OgreSystemMouseHandler::onUploadObjectEvent, this, _1, _2));
        mUploadWebView->loadFile("chrome/upload.html");
    }
}

void OgreSystemMouseHandler::handleQueryAngleWidget() {
    if(mQueryAngleWidgetView) {
        WebViewManager::getSingleton().destroyWebView(mQueryAngleWidgetView);
        mQueryAngleWidgetView = NULL;
    } else {
        mQueryAngleWidgetView = WebViewManager::getSingleton().createWebView("query_angle_widget", "query_angle_widget",300, 100,
            OverlayPosition(RP_BOTTOMRIGHT), false, 70, TIER_FRONT);
        mQueryAngleWidgetView->bind("set_query_angle", std::tr1::bind(&OgreSystemMouseHandler::handleSetQueryAngle, this, _1, _2));
        mQueryAngleWidgetView->loadFile("debug/query_angle.html");
    }
}

void OgreSystemMouseHandler::handleSetQueryAngle(WebView* webview, const JSArguments& args) {
    assert(args.size() == 1);
    mNewQueryAngle = boost::lexical_cast<float>(args[0].data());
    mQueryAngleTimer->cancel();
    mQueryAngleTimer->wait(Duration::seconds(1.f));
}

void OgreSystemMouseHandler::handleSetQueryAngleTimeout() {
    mParent->mViewer->requestQueryUpdate(mParent->mPresenceID.space(), mParent->mPresenceID.object(), SolidAngle(mNewQueryAngle));
}

void OgreSystemMouseHandler::executeScript(WebView* wv, const JSArguments& args)
{
    ScriptingUIObjectMap::iterator objit = mScriptingUIObjects.find(wv);
    if (objit == mScriptingUIObjects.end())
        return;
    ProxyObjectPtr target_obj(objit->second.lock());

    if (!target_obj) return;


    JSIter command_it;
    for (command_it = args.begin(); command_it != args.end(); ++command_it)
    {
        std::string strcmp (command_it->begin());



        if (strcmp == "Command")
        {
            Sirikata::JS::Protocol::ScriptingMessage scripting_msg;
            Sirikata::JS::Protocol::IScriptingRequest scripting_req = scripting_msg.add_requests();
            scripting_req.set_id(0);
            //scripting_req.set_body(String(command_it->second));
            JSIter nexter = command_it + 1;
            String msgBody = String(nexter->begin());
            scripting_req.set_body(msgBody);
            std::string serialized_scripting_request;
            scripting_msg.SerializeToString(&serialized_scripting_request);
            mScriptingRequestPort->send(
                ODP::Endpoint(target_obj->getObjectReference(), Services::SCRIPTING),
                MemoryReference(serialized_scripting_request)
            );
        }
    }
}

static String convertAndEscapeJavascriptString(const String& in) {
    String result = "'";

    for(int ii = 0; ii < in.size(); ii++) {
        switch(in[ii]) {
          case '\n':
            result += "\\n"; break;
          case '\r':
            result += "\\r"; break;
          case '\'':
            result += "\'"; break;
          case '\t':
            result += "\\t"; break;
          default:
            result += in[ii]; break;
        }
    }

    result += "'";
    return result;
}

void OgreSystemMouseHandler::handleScriptReply(const ODP::Endpoint& src, const ODP::Endpoint& dst, MemoryReference payload) {
    Sirikata::JS::Protocol::ScriptingMessage scripting_msg;
    bool parsed = scripting_msg.ParseFromArray(payload.data(), payload.size());
    if (!parsed) return;


    if (mScriptingUIWebViews.find(src.spaceObject()) == mScriptingUIWebViews.end()) return;
    WebView* wv = mScriptingUIWebViews[src.spaceObject()];

    for(int32 ii = 0; ii < scripting_msg.replies_size(); ii++) {
        wv->evaluateJS("addMessage( " + convertAndEscapeJavascriptString(scripting_msg.replies(ii).body()) + " )");
    }
}

/**
   This function sends out a message on KnownServices port
   LISTEN_FOR_SCRIPT_BEGIN to the HostedObject.  Presumably, the hosted
   object receives the message and attaches a JSObjectScript to the HostedObject.
*/
void OgreSystemMouseHandler::initScriptOnSelectedObjects() {
    for (SelectedObjectSet::const_iterator selectIter = mSelectedObjects.begin();
         selectIter != mSelectedObjects.end(); ++selectIter) {
        ProxyObjectPtr obj(selectIter->lock());

        Sirikata::JS::Protocol::ScriptingInit init_script;

        // Filter out the script type from rest of args
        //String script_type = "js"; // FIXME how to decide this?
        init_script.set_script(ScriptTypes::JS_SCRIPT_TYPE);
        init_script.set_messager(KnownMessages::INIT_SCRIPT);
        String serializedInitScript;
        init_script.SerializeToString(&serializedInitScript);
        //std::string serialized;
        //init_script.SerializeToString(&serialized);

        obj->sendMessage(
            Services::LISTEN_FOR_SCRIPT_BEGIN,
            MemoryReference(serializedInitScript.data(), serializedInitScript.length())
            //  MemoryReference(serialized.data(), serialized.length())
        );
    }
}

ProxyObjectPtr OgreSystemMouseHandler::getTopLevelParent(ProxyObjectPtr camProxy)
{
    ProxyObjectPtr parentProxy;
    while ((parentProxy=camProxy->getParentProxy())) {
        camProxy=parentProxy;
    }
    return camProxy;
}


void OgreSystemMouseHandler::setDragModeAction(const String& modename) {
    if (modename == "")
        mDragAction[1] = 0;

    mDragAction[1] = DragActionRegistry::get(modename);
}

void OgreSystemMouseHandler::zoomAction(float value, Vector2f axes)
{
    if (!mParent||!mParent->mPrimaryCamera) return;
    zoomInOut(value, axes, mParent->mPrimaryCamera, mSelectedObjects, mParent);
}

///// Top Level Input Event Handlers //////

EventResponse OgreSystemMouseHandler::keyHandler(EventPtr ev) {
    std::tr1::shared_ptr<ButtonEvent> buttonev (
        std::tr1::dynamic_pointer_cast<ButtonEvent>(ev));
    if (!buttonev) {
        return EventResponse::nop();
    }

    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onButton(buttonev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::axisHandler(EventPtr ev) {
    std::tr1::shared_ptr<AxisEvent> axisev (
        std::tr1::dynamic_pointer_cast<AxisEvent>(ev));
    if (!axisev)
        return EventResponse::nop();

    float multiplier = mParent->mInputManager->mWheelToAxis->as<float>();

    if (axisev->mAxis == SDLMouse::WHEELY) {
        bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(0, axisev->mValue.getCentered()/multiplier));
        if (used)
            return EventResponse::cancel();
    }
    if (axisev->mAxis == SDLMouse::WHEELX) {
        bool used = WebViewManager::getSingleton().injectMouseWheel(WebViewCoord(axisev->mValue.getCentered()/multiplier, 0));
        if (used)
            return EventResponse::cancel();
    }

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::cancel();
}

EventResponse OgreSystemMouseHandler::textInputHandler(EventPtr ev) {
    std::tr1::shared_ptr<TextInputEvent> textev (
        std::tr1::dynamic_pointer_cast<TextInputEvent>(ev));
    if (!textev)
        return EventResponse::nop();

    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onKeyTextInput(textev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::mouseHoverHandler(EventPtr ev) {
    std::tr1::shared_ptr<MouseHoverEvent> mouseev (
        std::tr1::dynamic_pointer_cast<MouseHoverEvent>(ev));
    if (!mouseev)
        return EventResponse::nop();

    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMouseMove(mouseev);
    if (browser_resp == EventResponse::cancel())
        return EventResponse::cancel();

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, false);
    }

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::mousePressedHandler(EventPtr ev) {
    std::tr1::shared_ptr<MousePressedEvent> mouseev (
        std::tr1::dynamic_pointer_cast<MousePressedEvent>(ev));
    if (!mouseev)
        return EventResponse::nop();

    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMousePressed(mouseev);
    if (browser_resp == EventResponse::cancel()) {
        mWebViewActiveButtons.insert(mouseev->mButton);
        return EventResponse::cancel();
    }

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        hoverEntity(camera, time, mouseev->mXStart, mouseev->mYStart, true, &lhc, mWhichRayObject);
        mouseOverWebView(camera, time, mouseev->mXStart, mouseev->mYStart, true, false);
    }
    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::mouseClickHandler(EventPtr ev) {
    std::tr1::shared_ptr<MouseClickEvent> mouseev (
        std::tr1::dynamic_pointer_cast<MouseClickEvent>(ev));
    if (!mouseev)
        return EventResponse::nop();

    // Give the browsers a chance to use this input first
    EventResponse browser_resp = WebViewManager::getSingleton().onMouseClick(mouseev);
    if (mWebViewActiveButtons.find(mouseev->mButton) != mWebViewActiveButtons.end()) {
        mWebViewActiveButtons.erase(mouseev->mButton);
        return EventResponse::cancel();
    }
    if (browser_resp == EventResponse::cancel()) {
        return EventResponse::cancel();
    }
    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, mouseev->mX, mouseev->mY, false, true);
    }
    mMouseDownObject.reset();

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::mouseDragHandler(EventPtr evbase) {
    MouseDragEventPtr ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(evbase));
    if (!ev) {
        return EventResponse::nop();
    }
    if (!mParent||!mParent->mPrimaryCamera) return EventResponse::nop();
    std::set<int>::iterator iter = mWebViewActiveButtons.find(ev->mButton);
    if (iter != mWebViewActiveButtons.end()) {
        // Give the browser a chance to use this input
        EventResponse browser_resp = WebViewManager::getSingleton().onMouseDrag(ev);

        if (ev->mType == Input::DRAG_END) {
            mWebViewActiveButtons.erase(iter);
        }

        if (browser_resp == EventResponse::cancel()) {
            return EventResponse::cancel();
        }
    }

    if (mParent->mPrimaryCamera) {
        Camera *camera = mParent->mPrimaryCamera;
        Time time = mParent->simTime();
        int lhc=mLastHitCount;
        mouseOverWebView(camera, time, ev->mX, ev->mY, false, ev->mType == Input::DRAG_END);
    }
    if (ev->mType == Input::DRAG_END) {
        mMouseDownObject.reset();
    }

    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(evbase));
    delegateEvent(inputev);

    ActiveDrag * &drag = mActiveDrag[ev->mButton];
    if (ev->mType == Input::DRAG_START) {
        if (drag) {
            delete drag;
        }
        DragStartInfo info = {
            /*.sys = */ mParent,
            /*.camera = */ mParent->mPrimaryCamera, // for now...
            /*.objects = */ mSelectedObjects,
            /*.ev = */ ev
        };
        if (mDragAction[ev->mButton]) {
            drag = mDragAction[ev->mButton](info);
        }
    }
    if (drag) {
        drag->mouseMoved(ev);

        if (ev->mType == Input::DRAG_END) {
            delete drag;
            drag = 0;
        }
    }
    return EventResponse::nop();
}

EventResponse OgreSystemMouseHandler::webviewHandler(EventPtr ev) {
    WebViewEventPtr webview_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(ev));
    if (!webview_ev)
        return EventResponse::nop();

    // For everything else we let the browser go first, but in this case it should have
    // had its chance, so we just let it go
    InputEventPtr inputev (std::tr1::dynamic_pointer_cast<InputEvent>(ev));
    delegateEvent(inputev);

    return EventResponse::nop();
}



void OgreSystemMouseHandler::fpsUpdateTick(const Task::LocalTime& t) {
    if(mUIWidgetView) {
        Task::DeltaTime dt = t - mLastFpsTime;
        if(dt.toSeconds() > 1) {
            mLastFpsTime = t;
            Ogre::RenderTarget::FrameStats stats = mParent->getRenderTarget()->getStatistics();
            ostringstream os;
            os << stats.avgFPS;
            mUIWidgetView->evaluateJS("update_fps(" + os.str() + ")");
        }
    }
}

void OgreSystemMouseHandler::renderStatsUpdateTick(const Task::LocalTime& t) {
    if(mUIWidgetView) {
        Task::DeltaTime dt = t - mLastRenderStatsTime;
        if(dt.toSeconds() > 1) {
            mLastRenderStatsTime = t;
            Ogre::RenderTarget::FrameStats stats = mParent->getRenderTarget()->getStatistics();
            mUIWidgetView->evaluateJS(
                "update_render_stats(" +
                boost::lexical_cast<String>(stats.batchCount) +
                ", " +
                boost::lexical_cast<String>(stats.triangleCount) +
                ")"
            );
        }
    }
}



EventResponse OgreSystemMouseHandler::deviceListener(EventPtr evbase) {
    std::tr1::shared_ptr<InputDeviceEvent> ev (std::tr1::dynamic_pointer_cast<InputDeviceEvent>(evbase));
    if (!ev) {
        return EventResponse::nop();
    }
    switch (ev->mType) {
      case InputDeviceEvent::ADDED:
        if (!!(std::tr1::dynamic_pointer_cast<SDLMouse>(ev->mDevice))) {
            SubscriptionId subId = mParent->mInputManager->subscribeId(
                AxisEvent::getEventId(),
                std::tr1::bind(&OgreSystemMouseHandler::axisHandler, this, _1));
            mEvents.push_back(subId);
            mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
        }
        if (!!(std::tr1::dynamic_pointer_cast<SDLKeyboard>(ev->mDevice))) {
            // Key Pressed
            {
                SubscriptionId subId = mParent->mInputManager->subscribeId(
                    ButtonPressed::getEventId(),
                    std::tr1::bind(&OgreSystemMouseHandler::keyHandler, this, _1)
                );
                mEvents.push_back(subId);
                mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
            }
            // Key Released
            {
                SubscriptionId subId = mParent->mInputManager->subscribeId(
                    ButtonReleased::getEventId(),
                    std::tr1::bind(&OgreSystemMouseHandler::keyHandler, this, _1)
                );
                mEvents.push_back(subId);
                mDeviceSubscriptions.insert(DeviceSubMap::value_type(&*ev->mDevice, subId));
            }
        }
        break;
      case InputDeviceEvent::REMOVED: {
          DeviceSubMap::iterator iter;
          while ((iter = mDeviceSubscriptions.find(&*ev->mDevice))!=mDeviceSubscriptions.end()) {
              mParent->mInputManager->unsubscribe((*iter).second);
              mDeviceSubscriptions.erase(iter);
          }
      }
        break;
    }
    return EventResponse::nop();
}

OgreSystemMouseHandler::OgreSystemMouseHandler(OgreSystem *parent)
 : mParent(parent),
   mCurrentGroup(SpaceObjectReference::null()),
   mLastCameraTime(Task::LocalTime::now()),
   mLastFpsTime(Task::LocalTime::now()),
   mLastRenderStatsTime(Task::LocalTime::now()),
   mUploadWebView(NULL),
   mUIWidgetView(NULL),
   mQueryAngleWidgetView(NULL),
   mNewQueryAngle(0.f),
   mQueryAngleTimer( Network::IOTimer::create(parent->mContext->ioService, std::tr1::bind(&OgreSystemMouseHandler::handleSetQueryAngleTimeout, this)) ),
   mWhichRayObject(0),
   mScriptingRequestPort(NULL),
   mDelegate(NULL)
{
    mLastHitCount=0;
    mLastHitX=0;
    mLastHitY=0;

    mEvents.push_back(mParent->mInputManager->registerDeviceListener(
            std::tr1::bind(&OgreSystemMouseHandler::deviceListener, this, _1)));

    mDragAction[1] = 0;
    mDragAction[2] = DragActionRegistry::get("zoomCamera");
    mDragAction[3] = DragActionRegistry::get("panCamera");
    mDragAction[4] = DragActionRegistry::get("rotateCamera");

    mEvents.push_back(mParent->mInputManager->subscribeId(
            MouseHoverEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::mouseHoverHandler, this, _1)));

    mEvents.push_back(mParent->mInputManager->subscribeId(
            MousePressedEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::mousePressedHandler, this, _1)));

    mEvents.push_back(mParent->mInputManager->subscribeId(
            MouseDragEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::mouseDragHandler, this, _1)));

    mEvents.push_back(mParent->mInputManager->subscribeId(
            MouseClickEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::mouseClickHandler, this, _1)));

    mEvents.push_back(mParent->mInputManager->subscribeId(
            TextInputEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::textInputHandler, this, _1)));

    mEvents.push_back(mParent->mInputManager->subscribeId(
            WebViewEvent::getEventId(),
            std::tr1::bind(&OgreSystemMouseHandler::webviewHandler, this, _1)));

/*
    mInputResponses["openScriptingUI"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::createScriptingUIAction, this));

    mInputResponses["openObjectUI"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::createUIAction, this, "object/object.html"));


    //mInputResponses["executeScript"] = new WebViewStringMapInputResponse(std::tr1::bind(&OgreSystemMouseHandler::executeScript, this, _1, _2));

    mInputResponses["enterObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::enterObjectAction, this));
    mInputResponses["leaveObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::leaveObjectAction, this));
    mInputResponses["groupObjects"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::groupObjectsAction, this));
    mInputResponses["ungroupObjects"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::ungroupObjectsAction, this));

    mInputResponses["selectObject"] = new Vector2fInputResponse(std::tr1::bind(&OgreSystemMouseHandler::selectObjectAction, this, _1, 1));
    mInputResponses["selectObjectReverse"] = new Vector2fInputResponse(std::tr1::bind(&OgreSystemMouseHandler::selectObjectAction, this, _1, -1));

    mInputResponses["zoom"] = new AxisInputResponse(std::tr1::bind(&OgreSystemMouseHandler::zoomAction, this, _1, _2));

    mInputResponses["setDragModeNone"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, ""));
    mInputResponses["setDragModeMoveObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, "moveObject"));
    mInputResponses["setDragModeRotateObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, "rotateObject"));
    mInputResponses["setDragModeScaleObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, "scaleObject"));
    mInputResponses["setDragModeRotateCamera"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, "rotateCamera"));
    mInputResponses["setDragModePanCamera"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::setDragModeAction, this, "panCamera"));

    mInputResponses["startUploadObject"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::startUploadObject, this));
    mInputResponses["handleQueryAngleWidget"] = new SimpleInputResponse(std::tr1::bind(&OgreSystemMouseHandler::handleQueryAngleWidget, this));
*/

    // Allocate a random port for scripting requests
    mScriptingRequestPort = mParent->getViewer()->bindODPPort(mParent->getViewerPresence());
    mScriptingRequestPort->receive( std::tr1::bind(&OgreSystemMouseHandler::handleScriptReply, this, _1, _2, _3) );
}

OgreSystemMouseHandler::~OgreSystemMouseHandler() {
    delete mScriptingRequestPort;

    if(mUploadWebView) {
        WebViewManager::getSingleton().destroyWebView(mUploadWebView);
        mUploadWebView = NULL;
    }
    if(mUIWidgetView) {
        WebViewManager::getSingleton().destroyWebView(mUIWidgetView);
        mUIWidgetView = NULL;
    }

    for (std::vector<SubscriptionId>::const_iterator iter = mEvents.begin();
         iter != mEvents.end();
         ++iter) {
        mParent->mInputManager->unsubscribe(*iter);
    }
    for (std::map<int, ActiveDrag*>::iterator iter=mActiveDrag.begin(),iterend=mActiveDrag.end();iter!=iterend;++iter) {
        if(iter->second!=NULL)
            delete iter->second;
    }
}

void OgreSystemMouseHandler::setDelegate(Invokable* del) {
    mDelegate = del;
}

namespace {

// Fills in modifier fields
void fillModifiers(Invokable::Dict& event_data, Input::Modifier m) {
    Invokable::Dict mods;
    mods["shift"] = (bool)(m & MOD_SHIFT);
    mods["ctrl"] = (bool)(m & MOD_CTRL);
    mods["alt"] = (bool)(m & MOD_ALT);
    mods["super"] = (bool)(m & MOD_GUI);
    event_data["modifier"] = mods;
}

}

void OgreSystemMouseHandler::delegateEvent(InputEventPtr inputev) {
    if (mDelegate == NULL) return;

    Invokable::Dict event_data;
    {
        ButtonPressedEventPtr button_pressed_ev (std::tr1::dynamic_pointer_cast<ButtonPressed>(inputev));
        if (button_pressed_ev) {
            event_data["msg"] = String("button-pressed");
            event_data["button"] = keyButtonString(button_pressed_ev->mButton);
            event_data["keycode"] = button_pressed_ev->mButton;
            fillModifiers(event_data, button_pressed_ev->mModifier);
        }
    }

    {
        ButtonReleasedEventPtr button_released_ev (std::tr1::dynamic_pointer_cast<ButtonReleased>(inputev));
        if (button_released_ev) {
            event_data["msg"] = String("button-up");
            event_data["button"] = keyButtonString(button_released_ev->mButton);
            event_data["keycode"] = button_released_ev->mButton;
            fillModifiers(event_data, button_released_ev->mModifier);
        }
    }

    {
        ButtonDownEventPtr button_down_ev (std::tr1::dynamic_pointer_cast<ButtonDown>(inputev));
        if (button_down_ev) {
            event_data["msg"] = String("button-down");
            event_data["button"] = keyButtonString(button_down_ev->mButton);
            event_data["keycode"] = button_down_ev->mButton;
            fillModifiers(event_data, button_down_ev->mModifier);
        }
    }

    {
        AxisEventPtr axis_ev (std::tr1::dynamic_pointer_cast<AxisEvent>(inputev));
        if (axis_ev) {
            event_data["msg"] = String("axis");
            event_data["index"] = axis_ev->mAxis;
            event_data["value"] = axis_ev->mValue.value;
        }
    }

    {
        TextInputEventPtr text_input_ev (std::tr1::dynamic_pointer_cast<TextInputEvent>(inputev));
        if (text_input_ev) {
            event_data["msg"] = String("text");
            event_data["value"] = text_input_ev->mText;
        }
    }

    {
        MouseHoverEventPtr mouse_hover_ev (std::tr1::dynamic_pointer_cast<MouseHoverEvent>(inputev));
        if (mouse_hover_ev) {
            event_data["msg"] = String("mouse-hover");
            event_data["x"] = mouse_hover_ev->mX;
            event_data["y"] = mouse_hover_ev->mY;
        }
    }

    {
        MouseClickEventPtr mouse_click_ev (std::tr1::dynamic_pointer_cast<MouseClickEvent>(inputev));
        if (mouse_click_ev) {
            event_data["msg"] = String("mouse-click");
            event_data["button"] = mouse_click_ev->mButton;
            event_data["x"] = mouse_click_ev->mX;
            event_data["y"] = mouse_click_ev->mY;
        }
    }

    {
        MouseDragEventPtr mouse_drag_ev (std::tr1::dynamic_pointer_cast<MouseDragEvent>(inputev));
        if (mouse_drag_ev) {
            event_data["msg"] = String("mouse-drag");
            event_data["button"] = mouse_drag_ev->mButton;
            event_data["x"] = mouse_drag_ev->mX;
            event_data["y"] = mouse_drag_ev->mY;
        }
    }

    {
        DragAndDropEventPtr dd_ev (std::tr1::dynamic_pointer_cast<DragAndDropEvent>(inputev));
        if (dd_ev) {
            event_data["msg"] = String("dragdrop");
        }
    }

    {
        WebViewEventPtr wv_ev (std::tr1::dynamic_pointer_cast<WebViewEvent>(inputev));
        if (wv_ev) {
            event_data["msg"] = (String("webview"));
            event_data["webview"] = (wv_ev->webview);
            event_data["name"] = (wv_ev->name);
            Invokable::Array wv_args;
            for(uint32 ii = 0; ii < wv_ev->args.size(); ii++)
                wv_args.push_back(wv_ev->args[ii]);
            event_data["args"] = wv_args;
        }
    }

    if (event_data.empty()) return;

    std::vector<boost::any> args;
    args.push_back(event_data);
    mDelegate->invoke(args);
}


void OgreSystemMouseHandler::alert(const String& title, const String& text) {
    if (!mUIWidgetView) return;

    mUIWidgetView->evaluateJS("alert_permanent('" + title + "', '" + text + "');");
}

void OgreSystemMouseHandler::setParentGroupAndClear(const SpaceObjectReference &id) {
    clearSelection();
    mCurrentGroup = id;
}
const SpaceObjectReference &OgreSystemMouseHandler::getParentGroup() const {
    return mCurrentGroup;
}
void OgreSystemMouseHandler::addToSelection(const ProxyObjectPtr &obj) {
    mSelectedObjects.insert(obj);
}

void OgreSystemMouseHandler::onUIDirectoryListingFinished(String initial_path,
    std::tr1::shared_ptr<Transfer::DiskManager::ScanRequest::DirectoryListing> dirListing) {
    std::ostringstream os;
    os << "directory_list_request({path:'" << initial_path << "', results:[";
    if(dirListing) {
        bool needComma = false;
        for(Transfer::DiskManager::ScanRequest::DirectoryListing::iterator it =
                dirListing->begin(); it != dirListing->end(); it++) {
            if(needComma) {
                os << ",";
            } else {
                needComma = true;
            }
            os << "{path:'" << it->mPath.filename() << "', directory:" <<
                (it->mFileStatus.type() == Transfer::Filesystem::boost_fs::directory_file ?
                    "true" : "false") << "}";
        }
    }
    os << "]});";
    printf("Calling to JS: %s\n", os.str().c_str());
    mUIWidgetView->evaluateJS(os.str());
}

void OgreSystemMouseHandler::onUIAction(WebView* webview, const JSArguments& args) {
    printf("ui action event fired arg length = %d\n", (int)args.size());
    if (args.size() < 1) {
        printf("expected at least 1 argument, returning.\n");
        return;
    }

    String action_triggered(args[0].data());

    printf("UI Action triggered. action = '%s'.\n", action_triggered.c_str());

    if(action_triggered == "action_exit") {
        mParent->quit();
    } else if(action_triggered == "action_directory_list_request") {
        if(args.size() != 2) {
            printf("expected 2 arguments, returning.\n");
            return;
        }
        String pathRequested(args[1].data());
        std::tr1::shared_ptr<Transfer::DiskManager::DiskRequest> scan_req(
            new Transfer::DiskManager::ScanRequest(pathRequested,
                std::tr1::bind(&OgreSystemMouseHandler::onUIDirectoryListingFinished, this, pathRequested, _1)));
        Transfer::DiskManager::getSingleton().addRequest(scan_req);
    }
}

void OgreSystemMouseHandler::tick(const Task::LocalTime& t) {
    fpsUpdateTick(t);
    renderStatsUpdateTick(t);

    if(!mUIWidgetView) {
        mUIWidgetView = WebViewManager::getSingleton().createWebView("ui_widget","ui_widget",
            mParent->getRenderTarget()->getWidth(), mParent->getRenderTarget()->getHeight(),
            OverlayPosition(RP_TOPLEFT), false, 70, TIER_BACK, 0, WebView::WebViewBorderSize(0,0,0,0));
        mUIWidgetView->bind("ui-action", std::tr1::bind(&OgreSystemMouseHandler::onUIAction, this, _1, _2));
        mUIWidgetView->loadFile("chrome/ui.html");
        mUIWidgetView->setTransparent(true);
    }
}

}
}
