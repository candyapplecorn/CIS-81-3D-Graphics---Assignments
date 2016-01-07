//
//  2dtemplate.cpp
//
//  OpenGL 2D Template
//  Created by Viet Trinh on 1/31/12.
//  Copyright (c) 2012 Viet Trinh. All rights reserved.
//

/*
 * Notes -
 * One of the requirements of this assignment was to have lines near
 * eachother share vertices. In my solution, they don't. Instead,
 * when two lines should share vertices, a new point is created by passing
 * the other point's address to the new point's constructor.
 * This means they aren't literally sharing vertices, even if the vertices are
 * identical.
 *
 * Thus to draw a line with 3 or more points, I don't use the glBegin(GL_LINE_STRIP).
 * I don't know if this is bad - if so, I could work more and try a different solution.
 *
 * I tried to make lines actually share the same point() object by using shared pointers,
 * but I was getting segmentation faults after using the delete function. I was annoyed
 * because I thought using shared pointers would make it work. It didn't. 
 *
 * A different solution I would have used to tackle this entire assignment would be, rather
 * than tracking line objects, to make a vector of linked multi lists of points. It would 
 * be a linked list with forward iterators. If a point joins multiple lines points, that 
 * point would have multiple forward iterators. To draw this, a function would start at a 
 * point with only one link, and traverse the linked multilist in a loop until all forward
 * pointers were null. The function would pass point pairs (lines) into a container as it 
 * went, and at the end, that container would be used to draw the lines to the screen.
 *
 */

/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW1: Lines                         */ 
/* Student Name: Joseph Burger        */ 
/* SID: 20015548                      */ 
/**************************************/  

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <time.h>

#include <lineContainer.cpp>
using namespace std;

/*----- GLOBAL VARIABLES ------------*/
int button, state = 1;
float gx,gy;			// gx and gy are coordinates of 
						// a clicked point on screen
float x_win = 512.0;
float y_win = 512.0;

const int FRAMERATE = 1000 / 24;
// lineContanier contains all the lines we will draw
lineContainer lc = lineContainer();
// following contains all points that are currently
// attached to the mouse. They're moving with the mouse.
vector<point*> following = vector<point*>();

/*----- FUNCTION DECLARATION --------*/
void initScreen();
void display();
void mouseClicks(int but,int sta,int x,int y);
void mouseMoves(int x, int y);

// These are student functions
void updateFollowing(int x, int y);
void addToFollowing(vector<point*> near);
void keypress(unsigned char key, int x, int y);

//===== main function ==================================
int main(int argc, char**argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	
	// Create a window
	glutCreateWindow("Line Art");
	glutPositionWindow(0, 0);
	glutReshapeWindow(x_win, y_win);
	
	// Program start here...
	initScreen();
	glutDisplayFunc(display);
	glutMouseFunc(mouseClicks);
	glutMotionFunc(mouseMoves);
    glutPassiveMotionFunc(mouseMoves);
    glutKeyboardFunc(keypress);
	glutMainLoop();
	return 0;
}

//===== initScreen: set up the 2D canvas ==================
void initScreen(){
	glClearColor(0., 0.5, 0.5, 0.);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,1.0,0.0,1.0);	// set coord for viewport
	glViewport(0,0,x_win,y_win);	// set viewport
}

//===== display: perform OpenGL drawing on the canvas =========
void display(){
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0.0, 0.1, 0.1, 0.);
	
	// Your code here...

    glLineWidth(2);
    for (auto it = lc.lines.begin(), end = lc.lines.end(); it != end; ++it){
        // Draw the line with glbegin and glend
        glBegin(GL_LINES);
        glColor3f(1.0, 0.0, 1.0);
        glVertex3f(float((it)->p1->x)/x_win, float(y_win-(it)->p1->y)/y_win, 0);
        glVertex3f(float((it)->p2->x)/x_win, float(y_win-(it)->p2->y)/y_win, 0);
        glEnd();

        // Draw points for 20 points!
        glPointSize(3);
        glBegin(GL_POINTS);
        glColor3f(1.0,1.0,1.0);
        glVertex3f(float((it)->p1->x)/x_win, float(y_win-(it)->p1->y)/y_win, 0);
        glVertex3f(float((it)->p2->x)/x_win, float(y_win-(it)->p2->y)/y_win, 0);
        glEnd();
    }
	
	glutSwapBuffers();
}

//===== mouseClicks: handle left mouse click actions =========
void mouseClicks(int but,int sta,int x,int y){
	button = but;
	state = sta;
	
	if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
		gx = float(x)/x_win;
		gy = float(y_win-y)/y_win;
		// Your code here...
        if (following.empty()){
            // Check to see if a point is near and if so, use that point
            vector<point*> near = lc.getPointsNear(point(x, y));
            
            // This way allows the undo function of the program without a segfault
            point * p1 = near.empty() ? new point(x, y) : new point(*near[0]);
            // This is the old way which literally uses the same point, but
            // crashes the program when the undo "z" function is used:
            // point * p1 = near.empty() ? new point(x, y) : near[0];

            point * p2 = new point(x, y);
            // make a new line and add it the line container
            lc.lines.push_back(line(p1, p2));
            // following now has p2 until the next time a left click is depressed
            following.push_back(p2);
        }
        else{
            updateFollowing(x, y);
            following.clear();
        }
	}
	
	if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
		// Your code here...
        vector<point*> near = lc.getPointsNear(point(x, y));
        addToFollowing(near);
	}
    else if(button == GLUT_RIGHT_BUTTON && state == GLUT_UP){
        updateFollowing(x, y);
        following.clear();
    }
    
	glutPostRedisplay();
}

//=== mouseMoves: handle right mouse moves =============
void mouseMoves(int x, int y){

	if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){		
		gx = float(x)/x_win;
		gy = float(y_win-y)/y_win;
		// Your code here...	
	}

    // See if enough time has passed to draw another frame
    static clock_t t = clock();
    t = clock() - t;
    if (float(t)/CLOCKS_PER_SEC > FRAMERATE)
        return;
    updateFollowing(x, y);
    
	glutPostRedisplay();
}

/*----- Student Defined Functions -----*/

// I wanted to add an undo feature, so I did
// If the user presses z, the last placed line will be removed
void keypress(unsigned char key, int x, int y){
    if (key == 'z'){
        if (!lc.lines.empty())
            lc.lines.pop_back();
	    glutPostRedisplay();
    }
}

// update following updates all points in following to the passed params
void updateFollowing(int x, int y){
    if (!following.empty())
        for (auto it = following.begin(), end = following.end(); it != end; ++it)
            (*it)->x = x, (*it)->y = y;
}

// addToFollowing takes a vector and adds its contents to following
void addToFollowing(vector<point*> near){
    if (!near.empty())
        for (auto it = near.begin(), end = near.end(); it != end; ++it)
            following.push_back(*it);
}
