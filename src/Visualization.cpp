#include "Analysis.hpp"
#include "Visualization.hpp"
#include "CoordinateSegmentation.hpp"
#include "LocationService.hpp"
#include "ObjectFactory.hpp"
#include "AnalysisEvents.hpp"
#include <GL/glut.h>
namespace CBR {
static LocationVisualization*gVis=NULL;
void LocationVisualization::mainLoop(){
    mCurTime+=mSamplingRate;
    while (mCurEvent!=mObservedEvents->end()) {
        ObjectEvent*oe=*mCurEvent;
        if (oe->end_time()>mCurTime) {
            break;
        }
        

        ProximityEvent*pe;
        if ((pe=dynamic_cast<ProximityEvent*>(oe))) {
            if (pe->entered) {
                mVisible[pe->source]=pe->loc;
            }else {
                mVisible.erase(mVisible.find(pe->source));
            }
        }
        LocationEvent*le;
        if ((le=dynamic_cast<LocationEvent*>(oe))) {
            VisibilityMap::iterator where=mVisible.find(le->source);
            if (where==mVisible.end()) {
                std::cerr<<"Guy "<<le->source.readableHexData()<<"not visible who gave update to "<<le->receiver.readableHexData()<<"\n";
            }else {
                where->second=le->loc;
            }

        }        
        ++mCurEvent;

    }
    if (mCurEvent!=mObservedEvents->end())
        mLoc->tick(mCurTime);

    glClear(GL_COLOR_BUFFER_BIT);		/* clear the display */
    uint32 i;

    uint32 minServerID=1;
    uint32 maxServerID=mSeg->numServers()+1;
    BoundingBox3f bboxwidest;
    for (i=minServerID;i<maxServerID;++i) {
        BoundingBox3f bbox=mSeg->serverRegion(i);
        if (i==minServerID) {
            bboxwidest=bbox;
        }else {
            bboxwidest.mergeIn(bbox);
        }
    }
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Vector3f centre=bboxwidest.min()+bboxwidest.diag()*.5f;
    //glScalef(1,1,1);
    glScalef(2.0/bboxwidest.diag().x,2.0/bboxwidest.diag().y,2.0);
    glTranslatef(-centre.x,-centre.y,0);
    
    glBegin(GL_QUADS);
    for (i=minServerID;i<maxServerID;++i) {
        BoundingBox3f bbox=mSeg->serverRegion(i);
        glColor3f(0,0, (i/(float)maxServerID));		/* set current color to white */
        glVertex2f(bbox.min().x,bbox.min().y);
        glVertex2f(bbox.min().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.max().y);
        glVertex2f(bbox.max().x,bbox.min().y);
    }   
    glEnd();
    glPointSize(2);
    glBegin(GL_POINTS);
    for (ObjectFactory::iterator it=mFactory->begin();it!=mFactory->end();++it) {
        Vector3f pos=mLoc->currentPosition(*it);
        if (*it==mObserver) {
            glColor3f(1,0,0);
        }else {
            VisibilityMap::iterator where;
            if ((where=mVisible.find(*it))==mVisible.end()) {
                glColor3f(0,.5,0);
            }else {
                glColor3f(1,1,1);
                Vector3f twhere=where->second.extrapolate(mCurTime).position();
                glVertex2f(twhere.x,twhere.y);
                glEnd();
                glColor3f(.25,.25,.25);
                glBegin(GL_LINES);
                glVertex2f(twhere.x,twhere.y);
                glVertex2f(pos.x,pos.y);
                glEnd();
                glBegin(GL_POINTS);
                glColor3f(.5,.5,.5);
            }
        }
        glVertex2f(pos.x,pos.y);
    }
    glEnd();
    glutSwapBuffers();				/* Complete any pending operations */
    usleep((int)(mSamplingRate.milliseconds()*1000));
}
void main_loop() {
  static LocationVisualization*sVis=gVis;
  sVis->mainLoop();
  
}
LocationVisualization::LocationVisualization(const char *opt_name, const uint32 nservers, ObjectFactory*factory, LocationService*loc_serv, CoordinateSegmentation*cseg):
    LocationErrorAnalysis(opt_name,nservers),mCurTime(0){
    mFactory=factory;
    mLoc=loc_serv;
    mSeg=cseg;
    mSamplingRate=Duration::milliseconds(30.0f);
}

void LocationVisualization::displayError(const UUID&observer, const Duration&sampling_rate) {
    
    int argc=1; char argvv[]="test";
    char*argv=&argvv[0];
    glutInit(&argc,&argv);
    glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);
    glutInitWindowSize(1024,768);
    glViewport(0,0,1024,768);
    int win=glutCreateWindow("Viewer");
    gVis=this;
    this->mObserver=observer;
    mObservedEvents = getEventList(observer);
    mCurEvent=mObservedEvents->begin();
    mSamplingRate=sampling_rate;
    if (mCurEvent!=mObservedEvents->end()) {
        mCurTime=(*mCurEvent)->begin_time();
    }
    glutIdleFunc(&main_loop);
    glutDisplayFunc(&main_loop);    
    glutMainLoop();
}

void LocationVisualization::displayRandomViewerError(int seed, const Duration&sampling_rate) {
    unsigned int which=seed;
    which=which%mEventLists.size();
    ObjectEventListMap::iterator iter=mEventLists.begin();
    for (unsigned int i=0;i<which;++i,++iter) {
    }
    displayError(iter->first,sampling_rate);

}

}
