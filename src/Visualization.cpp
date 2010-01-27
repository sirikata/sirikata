#include "Analysis.hpp"
#include "Visualization.hpp"
#include "CoordinateSegmentation.hpp"
#include "RecordedMotionPath.hpp"
#include "Options.hpp"
#include <GL/glut.h>

namespace CBR {

void LocationVisualization::handleObjectEvent(const UUID& obj, bool add, const TimedMotionVector3f& loc) {
    VisibilityMap::iterator where;
    if (add) {
        mVisible[obj]=loc;
        if ((where=mInvisible.find(obj))!=mInvisible.end()) {
            mInvisible.erase(where);
        }
    }else {
        if ((where=mVisible.find(obj))!=mVisible.end()) {
            mVisible.erase(where);
        }else {
            std::cerr<<"Guy "<<obj.readableHexData()<<"not got erased from visibility\n";
            if ((where=mInvisible.find(obj))!=mInvisible.end()) {
                mInvisible.erase(where);
            }
        }
    }
}

void LocationVisualization::handleLocEvent(const UUID& obj, const TimedMotionVector3f& loc) {
    VisibilityMap::iterator where=mVisible.find(obj);
    if (where==mVisible.end()) {
        mInvisible[obj]=loc;
        std::cerr<<"Guy "<<obj.readableHexData()<<"not visible but gave update.\n";
    }else {
        where->second=loc;
    }
}

static LocationVisualization*gVis=NULL;
void LocationVisualization::mainLoop() {
    Time t = mSpaceContext->time + mSamplingRate;
    mSpaceContext->tick(t);

    while (mCurEvent!=mObservedEvents->end()) {
        Event*oe=*mCurEvent;
        if (oe->end_time()>mSpaceContext->time) {
            break;
        }

        ProximityEvent* pe = dynamic_cast<ProximityEvent*>(oe);
        if (pe != NULL)
            handleObjectEvent(pe->source, pe->entered, pe->loc);

        ServerObjectEventEvent* sobj_evt = dynamic_cast<ServerObjectEventEvent*>(oe);
        if (sobj_evt != NULL)
            handleObjectEvent(sobj_evt->object, sobj_evt->added, sobj_evt->loc);


        LocationEvent* le = dynamic_cast<LocationEvent*>(oe);
        if (le != NULL)
            handleLocEvent(le->source, le->loc);

        ServerLocationEvent* sloc_evt = dynamic_cast<ServerLocationEvent*>(oe);
        if (sloc_evt != NULL)
            handleLocEvent(sloc_evt->object, sloc_evt->loc);


	if (mSegmentationChangeIterator != mSegmentationChangeEvents.end()) {
	  if ((*mSegmentationChangeIterator)->begin_time() <= mSpaceContext->time) {
	    mDynamicBoxes.push_back((*mSegmentationChangeIterator)->bbox);
	    mSegmentationChangeIterator++;
	  }
	}

        ++mCurEvent;

    }
    if (mCurEvent!=mObservedEvents->end()) {
	mSeg->service();
    }

    glClear(GL_COLOR_BUFFER_BIT);		/* clear the display */
    uint32 i;

    uint32 minServerID=1;
    uint32 maxServerID=mSeg->numServers()+1;
    BoundingBox3f bboxwidest;
    for (i=minServerID;i<maxServerID;++i) {
        BoundingBoxList bboxList=mSeg->serverRegion(i);
	for (uint j=0; j<bboxList.size(); j++) {
	  BoundingBox3f bbox = bboxList[j];
	  if (i==minServerID) {
            bboxwidest=bbox;
	  }else {
            bboxwidest.mergeIn(bbox);
	  }
	}
    }

    for (i=0; i<mDynamicBoxes.size(); i++) {
      bboxwidest.mergeIn(mDynamicBoxes[i]);
    }

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vector3f centre=bboxwidest.min()+bboxwidest.diag()*.5f;
    //glScalef(1,1,1);
    glScalef(2.0/bboxwidest.diag().x,2.0/bboxwidest.diag().y,2.0);
    glTranslatef(-centre.x,-centre.y,0);

    glBegin(GL_QUADS);
    int k=0;
    for (i=minServerID;i<maxServerID;++i) {
      BoundingBoxList bboxList=mSeg->serverRegion(i);

      for (uint j=0;j<bboxList.size(); j++) {
	BoundingBox3f bbox = bboxList[j];
        glColor3f(0,0, (k/(float)maxServerID));	/* set current color to white */
	glVertex2f(bbox.min().x,bbox.min().y);
        glVertex2f(bbox.min().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.min().y);
	k++;
      }
    }

    for (i=0;i<mDynamicBoxes.size();++i) {
	BoundingBox3f bbox = mDynamicBoxes[i];
        glColor3f(0,0, (k/(float)maxServerID));	/* set current color to white */
	glVertex2f(bbox.min().x,bbox.min().y);
        glVertex2f(bbox.min().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.min().y);
	k++;
    }

    glEnd();


    glPointSize(2);
    glBegin(GL_POINTS);
    for (MotionPathMap::iterator it = mObjectMotions.begin(); it != mObjectMotions.end(); ++it) {
        UUID objid = it->first;
        MotionPath* motion = it->second;
        Vector3f pos = motion->at(mSpaceContext->time).extrapolate(mSpaceContext->time).position();
        if (objid == mObserver) {
            glEnd();
            glColor3f(.125,.125,.125);
/* FIXME this no longer makes sense since we're using solid angle queries
            glBegin(GL_LINE_STRIP);
            float rad=mFactory->getProximityRadius(objid);
            for (int i=0;i<180;++i) {
                glVertex2f(pos.x+rad*cos(3.141526536*i/90),pos.y+rad*sin(3.141526536*i/90));
            }
            glEnd();
*/
            glBegin(GL_POINTS);
            glColor3f(1,0,0);
        }else {
            VisibilityMap::iterator where;
            if ((where=mVisible.find(objid))==mVisible.end()) {
                glColor3f(0,.5,0);
                if ((where=mInvisible.find(objid))!=mInvisible.end()) {
                    Vector3f twhere=where->second.extrapolate(mSpaceContext->time).position();
                    glColor3f(.25,0,0);
                    glVertex2f(twhere.x,twhere.y);
                    glEnd();
                    glColor3f(.25,0,0);
                    glBegin(GL_LINES);
                    glVertex2f(twhere.x,twhere.y);
                    glVertex2f(pos.x,pos.y);
                    glEnd();
                    glBegin(GL_POINTS);
                    glColor3f(.5,0,0);
                }
            }else {
                glColor3f(.5,.5,.5);
                Vector3f twhere=where->second.extrapolate(mSpaceContext->time).position();
                glVertex2f(twhere.x,twhere.y);
                glEnd();
                glColor3f(.25,.25,.25);
                glBegin(GL_LINES);
                glVertex2f(twhere.x,twhere.y);
                glVertex2f(pos.x,pos.y);
                glEnd();
                glBegin(GL_POINTS);
                glColor3f(1,1,1);
            }
        }
        glVertex2f(pos.x,pos.y);
    }
    glEnd();
    glutSwapBuffers();				/* Complete any pending operations */
    usleep((int)(mSamplingRate.toMilliseconds()*1000));
}
void main_loop() {
  static LocationVisualization*sVis=gVis;
  sVis->mainLoop();

}
LocationVisualization::LocationVisualization(const char *opt_name, const uint32 nservers, SpaceContext* space_context, CoordinateSegmentation*cseg):
 LocationErrorAnalysis(opt_name,nservers),
 mSeg(cseg),
 mSpaceContext(space_context),
 mSamplingRate(Duration::milliseconds(30.0f))
{
    // read in all our data
    for(uint32 server_id = 1; server_id <= nservers; server_id++) {
        String loc_file = GetPerServerFile(opt_name, server_id);
        std::ifstream is(loc_file.c_str(), std::ios::in);

        while(is) {
            std::string raw_evt = read_record(is);
            Event* evt = Event::parse(raw_evt, server_id);
            SegmentationChangeEvent* sce;
	    if ((sce=dynamic_cast<SegmentationChangeEvent*>(evt))) {
	      mSegmentationChangeEvents.push_back(sce);
	    }
            else {
                delete evt;
            }
        }
    }

    printf("mSegmentationChangeEvents.size=%du\n", (int)mSegmentationChangeEvents.size());

    mSegmentationChangeIterator = mSegmentationChangeEvents.begin();

    // Create map of per-object motion paths
    for(ObjectEventListMap::iterator it = mEventLists.begin(); it != mEventLists.end(); it++) {
        UUID objid = it->first;
        mObjectMotions[objid] = new RecordedMotionPath( *(getEventList(objid)) );
    }
}

// When we can't find an object/server event list to display use this so we can still
// display the object info we have
LocationVisualization::EventList* LocationVisualization::NullObservedEvents = new LocationVisualization::EventList();

void LocationVisualization::displayError(const UUID&observer, const Duration&sampling_rate) {
    std::cout << "Observer is: "  << observer.readableHexData() << "\n";
    this->mObserver=observer;
    mObservedEvents = getEventList(observer);
    if (mObservedEvents == NULL) mObservedEvents = NullObservedEvents;

    displayError(sampling_rate);
}

void LocationVisualization::displayError(const ServerID&observer, const Duration&sampling_rate) {
    std::cout << "Observer is: "  << (uint32)observer << "\n";
    this->mObserver=UUID::null();
    mObservedEvents = getEventList(observer);
    if (mObservedEvents == NULL) mObservedEvents = NullObservedEvents;

    displayError(sampling_rate);
}

void LocationVisualization::displayError(const Duration&sampling_rate) {

    int argc=1; char argvv[]="test";

    char*argv=&argvv[0];
    glutInit(&argc,&argv);
    glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);
    glutInitWindowSize(1024,768);
    glViewport(0,0,1024,768);
    int win=glutCreateWindow("Viewer");
    gVis=this;
    mCurEvent=mObservedEvents->begin();
    mSamplingRate=sampling_rate;
    if (mCurEvent!=mObservedEvents->end()) {
        mSpaceContext->tick( (*mCurEvent)->begin_time() );
    }
    glutIdleFunc(&main_loop);
    glutDisplayFunc(&main_loop);
    glutMainLoop();
}

void LocationVisualization::displayRandomViewerError(int seed, const Duration&sampling_rate) {
    if (mEventLists.size() > 0) {
        unsigned int which=seed;
        which=which%mEventLists.size();
        ObjectEventListMap::iterator iter=mEventLists.begin();
        for (unsigned int i=0;i<which;++i,++iter) {
        }
        displayError(iter->first,sampling_rate);
    }
    else {
        displayError(UUID::null(),sampling_rate);
    }
}

void LocationVisualization::displayRandomServerError(int seed, const Duration&sampling_rate) {
    if (mServerEventLists.size() > 0) {
        unsigned int which=seed;
        which=which%mServerEventLists.size();
        ServerEventListMap::iterator iter=mServerEventLists.begin();
        for (unsigned int i=0;i<which;++i,++iter) {
        }
        displayError(iter->first,sampling_rate);
    }
    else {
        displayError(NullServerID,sampling_rate);
    }
}

}
