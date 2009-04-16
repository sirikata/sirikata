#include "Analysis.hpp"
#include "Visualization.hpp"
#include <GL/glut.h>
namespace CBR {
void main_loop() {
  glClear(GL_COLOR_BUFFER_BIT);		/* clear the display */
  glColor3f(1.0, 1.0, 1.0);		/* set current color to white */
  glBegin(GL_TRIANGLES);
  glVertex2f(.5,.5);
  glVertex2f(.5,.75);
  glVertex2f(.75,.5);
  glEnd();
  glutSwapBuffers();				/* Complete any pending operations */

}
LocationVisualization::LocationVisualization(const char *opt_name, const uint32 nservers):
    LocationErrorAnalysis(opt_name,nservers){

}

void LocationVisualization::displayError(const UUID&observer, const Duration&sampling_rate, ObjectFactory*obj_factory) {
    
    int argc=1; char argvv[]="test";
    char*argv=&argvv[0];
    glutInit(&argc,&argv);
    glutInitDisplayMode(GLUT_RGB|GLUT_DEPTH|GLUT_DOUBLE);
    glutInitWindowSize(1024,768);
    glViewport(0,0,1024,768);
    int win=glutCreateWindow("Viewer");
    glutIdleFunc(&main_loop);
    glutDisplayFunc(&main_loop);
    glutMainLoop();
}

void LocationVisualization::displayRandomViewerError(int seed, const Duration&sampling_rate, ObjectFactory*obj_factory) {
    unsigned int which=seed;
    which=which%mEventLists.size();
    ObjectEventListMap::iterator iter=mEventLists.begin();
    for (unsigned int i=0;i<which;++i,++iter) {
    }
    displayError(iter->first,sampling_rate,obj_factory);

}

}
