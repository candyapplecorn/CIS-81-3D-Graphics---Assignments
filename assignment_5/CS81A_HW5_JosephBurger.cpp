//
//  3dtemplate.cpp
//
//  OpenGL 3D Template
//  Created by Viet Trinh on 1/31/12.
//  Copyright (c) 2012 Viet Trinh. All rights reserved.
//
//
/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW5: Procedural Modeling           */ 
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
#include <cmath>
#include <cstring>
#define PI 3.14159265359
using namespace std;

/*----- CLASSES DECLARATION ----------*/
//=== class Point ===
class Point{
    float x;
    float y;
    float z;
public:
    Point(){x=0;y=0;z=0;}
    Point(float xp,float yp,float zp){x = xp;y=yp;z=zp;}
    float getXCoord(){return x;}
    float getYCoord(){return y;}
    float getZCoord(){return z;}
};

//=== class Vector ===
class Vector{
public:
    float x;
    float y;
    float z;
    
    Vector(){x = 0.0;y=0.0;z=0.0;}
};

/*----- GLOBAL VARIABLES -------------*/
int button, state = 1;
float gx, gy;
float x_win = 800.0;
float z_win = 650.0;
float obj_material_amb[4] = {0.3,0.5,0.2,1.0};
float obj_material_diff[4] = {0.9,0.9,0.9,1.0};
float obj_material_spec[4] = {0.1,0.1,0.1,1.0};
float VIEWER[3]={400.0,300.0,0.0};
float LOOKAT[3]={static_cast<float>(x_win/2.0),0.0,static_cast<float>(z_win/2.0)};
Vector view_dir;           // direction vector from viewer to look-at-point
float roughnessFactor = .01;
const int width = 64;
float map[width][width];

/*----- FUNCTIONS DECLARATION --------*/
void init();
void screenSetUp();
void displayScreen();
void mouseClicks(int but,int sta,int x,int y);
void mouseMoves(int x, int y);
void keyPresses(unsigned char c, int x, int y);

// I actually figured out how to use this for lighting,
// unlike in the last assignment
Vector* calculateNormal(Point* a, Point* b, Point* c);
void MoveUpDown(float step);
void RotateLeftRight(float angle);

// Prepares the 3d terrain
void mapSetUp2d ();
// myRandom returns a float between 0 and 1. Useful
// for getting random numbers in a range
float myRandom(){ return rand() % 100 / 100.0; }
void FractalTree (float r, float h, int iter);
// Probably from Viet Trinh's lecture
// I say probably because I think I misunderstood it
float randomMidpointDisplacement ( float a, float b );
// Taken from Viet Trinh's lecture #16
// given random numbers x1 and x2 with equal distribution from -1 to 1
// generate numbers y1 and y2 with normal distribution centered at 0.0 
// and with standard deviation 1.0
void Gaussian (float &y1, float &y2);
//===== main function ==================================
int main(int argc, char**argv){
    srand(time(NULL));
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	
	// Create a window
	glutCreateWindow("HW5: Procedural Modeling");
	glutPositionWindow(0, 0);
	glutReshapeWindow(x_win, z_win);
	
	// Program start here...
    mapSetUp2d();
	glutDisplayFunc(displayScreen);
	glutMouseFunc(mouseClicks);
	glutMotionFunc(mouseMoves);
    glutKeyboardFunc(keyPresses);
	glutMainLoop();
	return 0;
}

//===== screenSetUp ====================================
void screenSetUp(){
    
    /*------ SET UP 3D SCREEN -------*/
    float light_position[4] = { 0.0, 50.0, 300.0 , 0.0 };   // x, y, z, w
    // set up object color
    float obj_light_ambient[4] = { 0.2, 0.2, 0.2, 1.0 };     // r, g, b, a
    float obj_light_diffuse[4] = { 0.8, 0.3, 0.1, 1.0 };     // r, g, b, a
    float obj_light_specular[4] = { 0.8, 0.3, 0.1, 1.0 };    // r, g, b, a
    
    // set up background/terrain color
    float terrain_material_amb_diff[4] = { 0.0, 0.0, 1.0, 1.0 }; // r, g, b, a
    float terrain_material_specular[4] = { 0.0, 1.0, 1.0, 1.0 }; // r, g, b, a
    
    glClearColor(0.0,0.0,0.0,1.0);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0,x_win/z_win,5.0,5000.0);
    glViewport(0,0,x_win,z_win);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    //glEnable(GL_POINT_SMOOTH);
    //glEnable(GL_BLEND);
    //ddglBlendFunc(GL_SRC_ALPHA, GL_ONE);
    
    glLightfv(GL_LIGHT0, GL_AMBIENT, obj_light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, obj_light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, obj_light_specular);
    glLightfv(GL_LIGHT0, GL_POSITION, light_position);
    glEnable(GL_LIGHT0);
	
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, terrain_material_amb_diff);	// set up color for background/terrain
    glMaterialfv(GL_FRONT, GL_SPECULAR, terrain_material_specular);             // set up color for background/terrain
    
	// viewer is looking towards the center of the terrain
	gluLookAt(VIEWER[0],VIEWER[1],VIEWER[2],LOOKAT[0],LOOKAT[1],LOOKAT[2],0,1,0);
    
	// draw the terrain
    // a x_win x z_win rectangle with lower left corner (0,0,0)
    // up direction is y
    glBegin(GL_QUADS);
    glNormal3f(0.0,1.0,0.0);
    glVertex3f(0.0,0.0,0.0);
    glVertex3f(x_win,0.0,0.0);
    glVertex3f(x_win,0.0,z_win);
    glVertex3f(0.0,0.0,z_win);
    glEnd();

}
/*
 *  I couldn't figure out how to use the midpoint equation
 *  nor the guassian random number algorithm from lecture 16, 
 *  but mapSetUp2d fills all the elements in an array with values
 *  between 1 and -1
 */
void mapSetUp2d () {
    int seed = 0, flag = -100;
    // initialize all values of map to a flag number
    for (int i = 0; i < width; i++) for (int k = 0; k < width; k++) map[i][k] = flag;
    // set the four corners to 0
    map[0][0] = seed, map[0][width - 1] = seed, map[width - 1][width - 1] = seed, map[width - 1][0] = seed;

    int step = width / 2;
    do {
        for (int z = 0; z < width; z += step) if (z - step >= 0 && z + step <= width)
            for (int x = 0; x < width; x += step) if (x - step >= 0 && x + step <= width)
                if (map[z-step][x - step] != flag && map[z+step][x + step] != flag){
                    Gaussian(map[z - step][x - step], map[z + step][x + step]);
                    map[z][x] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z][x - step] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z][x + step] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z - step][x] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z + step][x - step] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z - step][x - step] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
                    map[z + step][x + step] = randomMidpointDisplacement(map[z - step][x - step], map[z + step][x + step]);
            }
    } while (step /= 2, (float)step / 2.0 >= 0.5); // 4 2 1
}
//===== displayScreen ===================================
void displayScreen(){
    
    screenSetUp();
    // Your code here for drawing objects...

    /*
     * Draw the trees - Redraw trees every new frame
     */
    for (int i = myRandom() * 8 + 8; i >= 0; i--) {
        glPushMatrix();
        glTranslatef(x_win / sqrt(width) * myRandom() * sqrt(width),
                    map[(int)(myRandom() * width)][(int)(myRandom() * width)],
                    z_win / sqrt(width) * myRandom() * sqrt(width));
        FractalTree(myRandom() * 2 + 1, 15 + myRandom() * 15, 6);
        glPopMatrix();
    }
    /*
     *  Draw the terrain with lighting
     */
    float brown[4] = {55 / 255.0, 25 / 255.0, 0.0, 1.0}, HEIGHT = 50.0, 
          quadx = x_win / width, quadz = z_win / width;

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, brown);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, brown);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, brown);
    glBegin(GL_QUADS);
    for (int x = 1; x < width; x++) {
        for (int z = 1; z < width; z++) {
            // Need to calculate the normal for lighting 
            Point * a = new Point(quadx * (x - 1), HEIGHT * map[x-1][z-1],  quadz * (z - 1)),
                  * b = new Point(quadx * (x - 1), HEIGHT * map[x-1][z],    quadz * z),
                  * c = new Point(quadx * x,       HEIGHT * map[x][z],      quadz * z);
            Vector *v = calculateNormal(a, b, c);
            glNormal3f(v->x,v->y,v->z);
            delete a, delete b, delete c, delete v;

            glVertex3f(quadx * (x - 1), HEIGHT * map[x-1][z-1],    quadz * (z - 1));
            glVertex3f(quadx * (x - 1), HEIGHT * map[x-1][z],      quadz * z);
            glVertex3f(quadx * x,       HEIGHT * map[x][z],        quadz * z);
            glVertex3f(quadx * x,       HEIGHT * map[x][z-1],      quadz * (z - 1));
        }
    }
    glEnd();

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, obj_material_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, obj_material_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, obj_material_spec);
    
    glutSwapBuffers();
}

//===== mouseClicks ====================================
void mouseClicks(int but,int sta,int x,int y){
    button = but;
    state = sta;
    
    if(button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
        gx = float(x)/x_win;
        gy = float(z_win-y)/z_win;
        
        // Your code here...
    }
    
    glutPostRedisplay();
}

//===== mouseMoves ====================================
void mouseMoves(int x, int y){
    
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){
		gx = float(x)/x_win;
        gy = float(z_win-y)/z_win;
        
        // Your code here..   
    }
    
    glutPostRedisplay();
}

//===== keyPresses ====================================
void keyPresses(unsigned char c, int x, int y){
    
    if (c =='w'){ MoveUpDown(10.0);}
    else if (c =='x'){ MoveUpDown(-10.0);}
    else if (c =='a'){ RotateLeftRight(10*PI/180.0);}
    else if (c =='d'){ RotateLeftRight(-10*PI/180.0);}
    
    // Your code here...
    
    glutPostRedisplay();
}

//======= calculateNormal ==============================
Vector* calculateNormal(Point* a, Point* b, Point* c){
/* Right hand rule: Thumb is the direction of normal vector,
   curve of fingers is the orientation of vertices
    n = ab x bc
 */
    
    Vector* ret = new Vector();
    Vector v1,v2,v;
    float d;
    
    // Vector ab
    v1.x = b->getXCoord() - a->getXCoord();
    v1.y = b->getYCoord() - a->getYCoord();
    v1.z = b->getZCoord() - a->getZCoord();
    
    // Vector bc
    v2.x = c->getXCoord() - b->getXCoord();
    v2.y = c->getYCoord() - b->getYCoord();
    v2.z = c->getZCoord() - b->getZCoord();
    
    // Vector ab x bc
    v.x = v1.y*v2.z - v2.y*v1.z;
    v.y = v1.z*v2.x - v2.z*v1.x;
    v.z = v1.x*v2.y - v2.x*v1.y;
    
    d = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
    
    // Normalize vector ab x bc
    ret->x = v.x/d;
    ret->y = v.y/d;
    ret->z = v.z/d;
    
    return ret;
}

//========= MoveUpDown ==============================
void MoveUpDown(float step){
    
    float distance;
    
    // normalize direction vector
    view_dir.x = LOOKAT[0] - VIEWER[0];
    view_dir.y = LOOKAT[1] - VIEWER[1];
    view_dir.z = LOOKAT[2] - VIEWER[2];
    
    distance = sqrt(view_dir.x*view_dir.x + view_dir.y*view_dir.y + view_dir.z*view_dir.z);
    
    view_dir.x = view_dir.x/distance;
    view_dir.y = view_dir.y/distance;
    view_dir.z = view_dir.z/distance;
    
    // translate viewer and look-at-point positions
    VIEWER[0] += step*view_dir.x;
    VIEWER[2] += step*view_dir.z;
    LOOKAT[0] += step*view_dir.x;
    LOOKAT[2] += step*view_dir.z;
    
}

//============ RotateLeftRight ============================
void RotateLeftRight(float angle){
    float x1,y1,z1,x2,y2,z2;
    
    // translate to origin
    x1 = LOOKAT[0] - VIEWER[0];
    y1 = LOOKAT[1] - VIEWER[1];
    z1 = LOOKAT[2] - VIEWER[2];
    
    // rotate around a pivot
    x2 = x1*cos(angle) + z1*sin(angle);
    y2 = y1;
    z2 = -x1*sin(angle) + z1*cos(angle);
    
    // translate back
    LOOKAT[0] = x2 + VIEWER[0];
    LOOKAT[1] = y2 + VIEWER[1];
    LOOKAT[2] = z2 + VIEWER[2];
}

void FractalTree (float r, float h, int iter) {
    GLUquadricObj * optr;

    // draw the vertical cylinder
    optr = gluNewQuadric();
    gluQuadricDrawStyle(optr, GLU_FILL);
    glPushMatrix();
    glRotatef(-90.0, 1.0, 0.0, 0.0);
    gluCylinder(optr, r, r, h, 10, 2); // ptr, rbase, rtop, height, nLongitude, nLatitudes
    glPopMatrix();

    // Modify the color
    float f[4] = {102.0 / 255.0, 51.0 / 255.0, 0 / 255.0, 1},
          g[4] = {0.0, 1.0, 0.0, 1.0};
    if ( iter == 1 ) 
        memcpy (f, g, sizeof(float) * 4);
    glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, f);	// set up color for background/terrain
    glMaterialfv(GL_FRONT, GL_SPECULAR, f);             // set up color for background/terrain

    // return a random number between 1.0 and 0.8
    auto random = [](){ return ((rand() % 100 / 100.0 * 20.0) + 80 ) / 100.0; };
    
    // if more iterations, then recursively draw branches
    if (iter > 0) {
        glPushMatrix();
        glTranslatef(0.0, h, 0.0); // translate upwards by h
        glRotatef(30.0 * random(), 1.0, 0.0, 0.0); // rotate about the x axis by 30 degrees
        FractalTree(0.8*r * random(), 0.8*h,iter-1); // draw the next iteration fractal tree
        glPopMatrix();

        glPushMatrix();
        glTranslatef(0.0, h, 0.0); // translate upwards by h
        glRotatef(120.0 * random(), 0.0, 1.0, 0.0); // rotate about the y axis by 120 degrees
        glRotatef(30.0 * random(), 1.0, 0.0, 0.0); // rotate about the x axis by 30 degrees
        FractalTree(0.8 * r * random(), 0.8 * h, iter-1); // draw the next iteration fractal tree
        glPopMatrix();
        
        glPushMatrix();
        glTranslatef(0.0, h, 0.0);
        glRotatef(240.0 * random(), 0.0, 1.0, 0.0);
        glRotatef(30.0 * random(), 1.0, 0.0, 0.0);
        FractalTree(0.8 * r * random(), 0.8 * h, iter-1); // draw the next iteration fractal tree
        glPopMatrix();
    }


}
float randomMidpointDisplacement ( float a, float b ) {
    float ymid, randomOffset, r;
    // Make the random variable the average of a and b
    // Because I don't actually know how to use this gaussian function
    r = (a + b) / 2;
    randomOffset = roughnessFactor * r * abs(b - a);
    ymid = ( a + b ) / 2 + randomOffset;
    return ymid;
}
void Gaussian (float &y1, float &y2) {
    float x1, x2, w;
    do {
        x1 = 2.0 * 0.001 * (float)(rand()%1000) - 1.0;
        x2 = 2.0 * 0.001 * (float)(rand()%1000) - 1.0;
        w = x1 * x1 + x2 * x2;
    } while (w >= 1.0);

    w = sqrt( (-2.0 * log(w)) / 2);
    y1 = x1 * w;
    y2 = x2 * w;
}
