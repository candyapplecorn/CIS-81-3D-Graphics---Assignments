//
//  2dtemplate.cpp
//
//  OpenGL 2D Template
//  Created by Viet Trinh on 1/31/12.
//  Copyright (c) 2012 Viet Trinh. All rights reserved.
//

/*
 * Notes
 * - Coming up with a way to calculate the current color was difficult, and
 *   I tried to tackle the problem a number of ways before I found a simpler
 *   solution that uses the absolute value of the distance between the 
 *   current angle, and the "brightest" angle of a color being calculated
 *
 * - Getting the porportions of the circle to be like the attached sample
 *   ouput took me a few tries, and eventually I settled on breaking my
 *   distances into thirds; it's a donut with the hole going from the
 *   center to radius/3, the darkest to the brightest going from radius/3
 *   to 2*radius/3, and the brightest to the whitest from 2*radius/3 to
 *   radius.
 *
 * - Turns out the color values returned by readPixel are from 0 to 256.
 *   it took me a while to figure out why my square wasn't drawing; it was
 *   because I was sending numbers greater than 1 to the color function.
 */

/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW2: Color Wheel                   */ 
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
#include <cstdlib>
#include <time.h>
#include <cmath>
using namespace std;

/*----- GLOBAL VARIABLES ------------*/
int button, state = 1;
float gx,gy;			// gx and gy are coordinates of 
// a clicked point on screen
int mx = 0, my = 0; // mx and my are the pixel coordinates of a
// clicked point on screen
float x_win = 512.0;
float y_win = 512.0;

const int FRAMERATE = 1000 / 24;
const float PI = 3.14159;
int diameter = (x_win > y_win ? y_win : x_win) * 7 / 8;
int radius = diameter / 2;

/*----- FUNCTION DECLARATION --------*/
void initScreen();
void display();
void mouseClicks(int but,int sta,int x,int y);
void mouseMoves(int x, int y);

// These are student functions
void drawCenterRect();

//===== main function ==================================
int main(int argc, char**argv){
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

    // Create a window
    glutCreateWindow("Color Wheel");
    glutPositionWindow(0, 0);
    glutReshapeWindow(x_win, y_win);

    // Program start here...
    initScreen();
    glutDisplayFunc(display);
    glutMouseFunc(mouseClicks);
    glutMotionFunc(mouseMoves);
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
    glClearColor(0.0, 0.0, 0.0, 0.);

    // Your code here...
    int xc = x_win > y_win ? y_win / 2 : x_win / 2, yc = xc;
    float r4 = radius / 5;
    float rAngle = 0, gAngle = 2 * PI / 3, bAngle = 2 * PI / 3 * 2;
    float RED, GREEN, BLUE;
    float distance;

    glLineWidth(1);
    glShadeModel(GL_SMOOTH);

    glBegin(GL_LINES);
    for (float theta = 0, TWO_PI = PI * 2; theta < TWO_PI; theta += 0.001){
        // Calculate the color
        // There are 3 sections of the circle, one for red, green and blue.
        // There are three points which have only one primary color -
        //  1,0,0 - 0,1,0 - 0,0,1
        // Red will be strongest (1, 0, 0) at y = 0, x = radius.
        // Green will be strongest (0, 1, 0) at y = sin(2PI/3), x = cos(2PI/3)
        // Blue will be strongest (0, 0, 1) at y = sin(2PI/3*2), x = cos(2PI/3*2)

        // Calculate red
        if (theta > bAngle){
            distance = abs( theta - 2 * PI );
            RED = 1 - distance / (2 * PI / 3);
        }
        else if (theta < gAngle){
            distance = abs( theta - rAngle );
            RED = 1 - distance / (2 * PI / 3);
        }
        else RED = 0;

        // Calculate green
        if (theta < gAngle || theta < bAngle){
            distance = abs( theta - gAngle );
            GREEN = 1 - distance / (2 * PI / 3);
        }
        else GREEN = 0;

        // Calculate blue
        if (theta > gAngle && theta < 2 * PI){
            distance = abs( theta - bAngle );
            BLUE = 1 - distance / (2 * PI / 3);
        }
        else BLUE = 0;

        // For many points along the circumference of the circle, draw
        // a line that goes from white to a color, then where that
        // line ends, draw another line going from that color to
        // black. In effect this draws a donut, and the black background
        // of our window fills in the center.

        glColor4f(1.0, 1.0, 1.0, 0.0); // White
        glVertex2f( (xc + radius * cos(theta)) / x_win, (yc + radius * sin(theta)) / y_win);
        glColor4f(RED, GREEN, BLUE, 0.0); // The current color
        glVertex2f( (xc + (2*radius/3) * cos(theta)) / x_win, (yc + (2*radius/3) * sin(theta)) / y_win);

        glVertex2f( (xc + (2*radius/3) * cos(theta)) / x_win, (yc + (2*radius/3) * sin(theta)) / y_win);
        glColor4f(0, 0, 0, 0.0); // Black
        glVertex2f( (xc + radius/3 * cos(theta)) / x_win, (yc + radius/3 * sin(theta)) / y_win);
    }
    glEnd();
    drawCenterRect();

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
        mx = x, my = y;
    }

    glutPostRedisplay();
}

//=== mouseMoves: handle right mouse moves =============
void mouseMoves(int x, int y){

    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){		
        gx = float(x)/x_win;
        gy = float(y_win-y)/y_win;
        // Your code here...	
        mx = x, my = y;
    }

    // See if enough time has passed to draw another frame
    static clock_t t = clock();
    t = clock() - t;
    if (float(t)/CLOCKS_PER_SEC > FRAMERATE)
        return;

    glutPostRedisplay();
}

/*----- Student Defined Functions -----*/
// I call this in mouseclicks and mousemoves, so even though
// calling a function is a slight performance cost, it saves LOC
// This function draws a square in the center of the circle, with
// a color equal to the pixel's color under the mouse coordinates.
void drawCenterRect(){
    int x = mx, y = my;
    unsigned char pixel[4];
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glReadPixels(x, y, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel); 
    float RED = (pixel[0]);
    float BLUE = (pixel[1]);
    float GREEN= (pixel[2]);

    int xc = x_win/2, yc = y_win/2;
    int halfboxwidth = radius / 5;

    glColor3f(RED/256, GREEN/256, BLUE/256);
    glBegin(GL_QUADS);
    glVertex3f((xc + halfboxwidth)/x_win, (yc + halfboxwidth)/y_win, 0); 
    glVertex3f((xc - halfboxwidth)/x_win, (yc + halfboxwidth)/y_win, 0);
    glVertex3f((xc - halfboxwidth)/x_win, (yc - halfboxwidth) / y_win, 0);
    glVertex3f((xc + halfboxwidth)/x_win, (yc - halfboxwidth) / y_win, 0);
    glEnd();
}
