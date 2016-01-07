//
//  2dtemplate.cpp
//
//  OpenGL 2D Template
//  Created by Viet Trinh on 1/31/12.
//  Copyright (c) 2012 Viet Trinh. All rights reserved.
//

/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW3: 2D Graphics Editor            */ 
/* Student Name: Joseph Burger        */ 
/* SID: 20015548                      */
/**************************************/  
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include <iostream> // For debug messages
#include <iomanip> // For debug messages
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <math.h> // for pi, sin and cos
#include <string>
#include <utility> // for pair
using namespace std;
bool DEBUG = false; // Set this to true for lots of console logging
/*----- GLOBAL VARIABLES ------------*/
int button, state = 1;
float gx,gy;			// gx and gy are coordinates of 
						// a clicked point on screen
float x_win = 600.0;
float y_win = 400.0;

struct Color {
    float r, g, b;
    Color(float r = (float)(rand() % 256)/256, 
    float g = (float)(rand() % 256)/256, 
    float b = (float)(rand() % 256)/256)
        : r(r), g(g), b(b) {}
    inline bool operator==(const Color& lhs){ return lhs.r == r && lhs.g == g && lhs.b == b;  }
};
Color gray4(.4, .4, .4), gray5(.5, .5, .5), gray6(.6, .6, .6), gray7(.7, .7, .7);

struct Rect {
    float x, y, w, h;
    Rect(float x, float y, float w, float h) : x(x), y(y), w(w), h(h) {}
};
struct Mouse {
    float x, y;
    int button, state;
};
struct Model {
    Color currentColor;
    // mode can be:  translate, rotate, resize.
    string mode = "translate";
    // Shape can be: hex, circle, square
    string shape = "square";
};

Model TheModel;
class ClickHandler { // contained inside a click distributor??
    public:
        virtual void onClick() = 0;
};
/*
BoundingBox contains a rect and is able to tell whether
clicks happen inside it
*/
class BoundingBox{
    public:
        int zIndex;     // zIndex is used for animation order.
        Rect r;
        string name;
        // Constructors
        BoundingBox(Rect r, int zIndex = 0, string name = "") : 
            r(r), name(name), zIndex(zIndex) {}
        // in - used to find if a click is inside
        virtual bool in(Mouse click){
            return (click.x < r.x || click.x > r.x + r.w || click.y < r.y || click.y > r.y + r.h) ?
                false : true;
        }
        virtual void mouseClicks(Mouse click){
            if (!this->in(click)) return;
            if (DEBUG && this->name != "") cout << "A click inside " << this->name << "\n";
            onClick(click);
        }
        virtual void mouseMoves(Mouse click){
            if (!this->in(click)) return;
            onMove(click);
        }
        virtual void display(){ drawMe(); }
        virtual void onClick(Mouse click){}
        virtual void onMove(Mouse click){}
        virtual void drawMe(){}
};
/*
BoundryShape inherits from bounding box and adds in 
a color and a GLuint, as well as a function to draw
a rect.
*/
class BoundryShape : public BoundingBox {
    public:
        BoundryShape(Rect r, Color c = Color(), int zIndex = 0, string name = "")
            : BoundingBox(r, zIndex, name), co(c), list(drawRect(r, c)) {}
        virtual GLuint getGLuint(){ return list; }
        virtual void setGLuint(GLuint g){ this->list = g; }
    protected:
        GLuint list;
        Color co;
        virtual GLuint drawRect(Rect r, Color c){
            GLuint ID = glGenLists(1);    
            glNewList(ID, GL_COMPILE);

            glColor3f(c.r, c.g, c.b); 
            glBegin(GL_QUADS);
            glVertex3f(r.x, r.y, 0);
            glVertex3f(r.x + r.w, r.y, 0);
            glVertex3f(r.x + r.w, r.y + r.h, 0);
            glVertex3f(r.x, r.y + r.h, 0);
            glEnd();    

            glEndList();
            return ID;
        }
};
class ArtCanvas : public BoundryShape{
    protected:
        struct shape {
            vector<pair<float, float> > points; // A list of points used to make this shape - needed for click detection
            pair<float, float> center; // The center of the shape
            float radius, angle; 
            string type; // hex, circle, square
            int id; 
            GLint list;
            Color c;
            GLint getGLuint(){ return this->list; }
            shape(vector<pair<float, float> > p, pair<float ,float> c, float r, string type, int id, GLint l, Color col, float angle = 0) : points(p), center(c), radius(r), type(type), id(id), list(l), c(col), angle(angle) {}
        };
        unsigned int numshapes = 0; // Used to assign an ID to shape objects
        vector <shape> shapes;
    public:
        ArtCanvas(Rect r, Color c = Color(), int zIndex = 0, string name = "") : BoundryShape(r, c, zIndex, name) {};
        vector<shape> getShapes(){ return this->shapes; }
    protected:
        // Keep track of what's being translated
        shape * translating, * rotating, * resizing;
        // r = radius, a = angle
        shape square(float x, float y, float r = 0.08f, Color c = TheModel.currentColor, float angle = M_PI / 4.000f){
            //x -= r/2.0, y -= r/2.0;
            vector<pair<float, float> > p;
            pair<float, float> center(x, y);

            GLuint ID = glGenLists(1);    
            glNewList(ID, GL_COMPILE);

            glColor3f(c.r, c.g, c.b); 
            glBegin(GL_QUADS);
            for (float end = M_PI * 2.0f + angle; angle <= end; angle += M_PI / 2.0f){
                glVertex3f(x + r * cos(angle) * static_cast<float>(y_win) / static_cast<float>(x_win), y + r * sin(angle), 0);
                p.push_back(pair<float, float>(x + r * cos(angle) * 2.0f / 3.0f, y + r * sin(angle)));
            }
            glEnd();    

            glEndList();

            // Prepare variables to construct a shape
            return shape( p, center, r, "square", numshapes++, ID, c, angle);
        }
        shape circle(float x, float y, float r = .1 / 2.0f, Color c = TheModel.currentColor, float angle = 0) {
            vector<pair<float, float> > p;
            GLuint ID = glGenLists(1);    
            glNewList(ID, GL_COMPILE);
            glColor3f(c.r, c.g, c.b); 

            glBegin(GL_TRIANGLE_FAN);
            // This is the center point
            glVertex3f(x, y, 0);
            for (float angle = 0, end = M_PI * 2.1f; angle <= end; angle += .1){
                glVertex3f(x + (r * cos(angle)) * static_cast<float>(y_win) / static_cast<float>(x_win), y + (r * sin(angle)), 0);
                p.push_back(pair<float, float>(x + r * cos(angle) * static_cast<float>(y_win) / static_cast<float>(x_win), y + r * sin(angle)));
            }
            glEnd();    

            glEndList();

            pair<float, float> center(x, y);
            return shape( p, center, r, "circle", numshapes++, ID, c, angle);
        }
        shape hex(float x, float y, float r = .1 / 2.0f, Color c = TheModel.currentColor, float angle = 0) {
            vector<pair<float, float> > p;
            GLuint ID = glGenLists(1);    
            glNewList(ID, GL_COMPILE);
            glColor3f(c.r, c.g, c.b); 

            glBegin(GL_TRIANGLE_FAN);
            // This is the center point
            glVertex3f(x, y, 0);
            for (float end = M_PI * 2.1f + angle; angle <= end; angle += M_PI / 3.0f){
                glVertex3f(x + r * cos(angle) * static_cast<float>(y_win)/x_win, y + r * sin(angle), 0);
                p.push_back(pair<float, float>(x + r * cos(angle) * static_cast<float>(y_win)/x_win, y + r * sin(angle)));
            }
            glEnd();    

            glEndList();

            pair<float, float> center(x, y);
            return shape( p, center, r, "hex", numshapes++, ID, c, angle);
        }
        // Returns index of element which contains m, else returns -1 if none contain m
        int inShape(Mouse m){
            int index = 0, ret = -1;
            for (shape s : shapes){
                // This treats the shapes as a circles, and thus, is slightly innaccurate.
                // If the user clicks slightly outside of one of the shape's sides, if
                // the click is close enough to the edge, it will register as a click inside
                // the shape.
                float a = pow(m.x - s.center.first, 2), b = pow(m.y - s.center.second, 2), c;
                c = sqrt(a + b);
                if (c <= s.radius)
                    ret = index;
                index ++;
            }
            return ret;
        }
        virtual void onClick(Mouse m){
            if (DEBUG) cout << "A click inside ArtCanvas\n";

            translating = rotating = resizing = NULL;

            // First, find out whether a click is inside an object or not.
            // If it is, perform a transform. Otherwise, place an object
            int index = inShape(m);

            if (index < 0) {
                if (m.button != GLUT_LEFT_BUTTON || m.state != GLUT_DOWN) return;
                // Place a shape 
                int type = TheModel.shape == "square" ? 0 : TheModel.shape == "circle" ? 1 : 2;
                switch (type){
                    case (0):
                        if (DEBUG) cout << "ArtCanvas wants to draw a square\n";
                        shapes.push_back(square(m.x, m.y));
                        return;
                    case (1) :
                        if (DEBUG) cout << "ArtCanvas wants to draw a circle\n";
                        shapes.push_back(circle(m.x, m.y));
                        return;
                    case (2) :
                        if (DEBUG) cout << "ArtCanvas wants to draw a hexagon\n";
                        shapes.push_back(hex(m.x, m.y));
                        return;
                }
            } else {
                // Transform a shape
                if (DEBUG) cout << "Index " << index << " found! It's a " << this->shapes[index].type << "!\n";
                int mode = TheModel.mode == "translate" ? 0 : TheModel.mode == "rotate" ? 1 : 2;
                switch (mode){
                    case (0):
                        if (DEBUG) cout << "ArtCanvas wants to translate\n"
                        << "The index of the thing to be translated: "
                        << index << " and its id: " << shapes[index].id;
                        translating = &shapes[index];
                        return;
                    case (1) :
                        if (DEBUG) cout << "ArtCanvas wants to rotate\n";
                        rotating = &shapes[index];
                        return;
                    case (2) :
                        if (DEBUG) cout << "ArtCanvas wants to resize\n";
                        resizing = &shapes[index];
                        return;
                }

            }
        }
        virtual void onMove(Mouse m) {
            if (GLUT_LEFT_BUTTON == m.button && m.state == GLUT_DOWN){
                // Translating
                if (translating && translating != NULL && TheModel.mode == "translate") {
                    if (translating->type == "square"){
                        float angle = translating->angle;
                        *translating = square(m.x, m.y, translating->radius, translating->c, translating->angle);
                        translating->angle = angle;
                    } else if (translating->type == "circle"){
                        *translating = circle(m.x, m.y, translating->radius, translating->c, translating->angle);
                    } else if (translating->type == "hex"){
                        float angle = translating->angle;
                        *translating = hex(m.x, m.y, translating->radius, translating->c, translating->angle);
                        translating->angle = angle;
                    }
                }
                // Rotating
                else if (rotating && rotating != NULL && TheModel.mode == "rotate") {
                    if (rotating->type == "square"){
                        *rotating = square(rotating->center.first, rotating->center.second, rotating->radius, rotating->c, rotating->angle + .033);
                    } else if (rotating->type == "circle"){
                        *rotating = circle(m.x, m.y, rotating->radius, rotating->c, rotating->angle + .1);
                    } else if (rotating->type == "hex"){
                        *rotating = hex(rotating->center.first, rotating->center.second, rotating->radius, rotating->c, rotating->angle + .033);
                    }
                }
                // Resizing
                else if (resizing && resizing != NULL && TheModel.mode == "resize") {
                    float resizeFactor = 1.01;
                    if (resizing->center.first > m.x)
                        resizeFactor = 0.99;
                        
                    if (resizing->type == "square"){
                        float angle = resizing->angle;
                        *resizing = square(resizing->center.first, resizing->center.second, resizing->radius * resizeFactor, resizing->c, resizing->angle);
                        resizing->angle = angle;
                    } else if (resizing->type == "circle"){
                        *resizing = circle(resizing->center.first, resizing->center.second, resizing->radius * resizeFactor, resizing->c, resizing->angle);
                    } else if (resizing->type == "hex"){
                        float angle = resizing->angle;
                        *resizing = hex(resizing->center.first, resizing->center.second, resizing->radius * resizeFactor, resizing->c, resizing->angle);
                        resizing->angle = angle;
                    }
                }
            }
        }
};
ArtCanvas * TheArtCanvas;
// Mode is used for selecting the transform
class Mode : public BoundryShape, public ClickHandler {
    public:
        Mode(Rect r, Color c = Color(), int zIndex = 0, string name = "") :
        BoundryShape(r, c, zIndex, name) { this->onClick(); }
        virtual GLuint getGLuint(){ 
            if (this->name == TheModel.mode)
                return drawRect(r, gray5);
            return this->list; 
        }
        virtual void onClick(){}
        virtual void onClick(Mouse m){ 
            if (DEBUG && this->name != "") cout << "A click inside SELECTOR " << this->name << "\n";
            TheModel.mode = this->name; 
        }
};
// Shape is used for the 3 shape selectors
class Shape : public BoundryShape, public ClickHandler {
    public:
        Shape(Rect r, Color c = Color(), int zIndex = 0, string name = "") :
        BoundryShape(r, c, zIndex, name) { this->onClick(); }
        virtual GLuint getGLuint(){ 
            if (this->name == TheModel.shape)
                return drawRect(r, gray5);
            return this->list; 
        }
        virtual void onClick(){}
        virtual void onClick(Mouse m){ 
            if (DEBUG && this->name != "") cout << "A click inside SELECTOR " << this->name << "\n";
            TheModel.shape = this->name; 
        }
};
// Selector is a controller and a view, and used to select colors
class Selector : public BoundryShape, public ClickHandler {
    public:
        Selector(Rect r, Color c = Color(), int zIndex = 0, string name = "") :
        BoundryShape(r, c, zIndex, name) { this->onClick(); }
        virtual GLuint getGLuint(){ return this->list; }
        virtual void onClick(){}
        virtual void onClick(Mouse m){ 
            if (DEBUG && this->name != "") cout << "A click inside SELECTOR " << this->name << "\n";
            TheModel.currentColor = co; 
        }
};
// CurrentColor shows the currently selected color
class currentColor : public BoundryShape{
    public:
        currentColor(Rect r, Color c = Color(), int zIndex = 0, string name = "") :
        BoundryShape(r, c, zIndex, name) {}
        virtual GLuint getGLuint(){ 
            return this->drawRect(r, TheModel.currentColor);
        }
};
// Containers for GUI elements
vector<BoundryShape *> lists;
vector<Selector *> selectors;
vector<BoundryShape *> concatenatedLists; // for display()
/*----- FUNCTION DECLARATION --------*/
void initScreen();
void display();
void mouseClicks(int but,int sta,int x,int y);
void mouseMoves(int x, int y);
GLuint drawRect(Rect r, Color c = Color());
GLuint drawRectOutline(Rect r, Color c = Color());
void init();
void drawString(float x, float y, char * text, Color c = Color(.4, .4, .4));
void drawCircle(float x, float y, float r, Color c = Color(.4, .4, .4));
void drawHex(float x, float y, float r, Color c);
void drawPlus(float x, float y, float r, Color c);
void drawRotIcon(float x, float y, float r, Color c);
void drawResizeIcon(float x, float y);

//===== main function ==================================
int main(int argc, char**argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Create a window
	glutCreateWindow("Art Painter");
	glutPositionWindow(0, 0);
	glutReshapeWindow(x_win, y_win);
	// STUDENT FUNCTION CALL(s)
    init();

	// Program start here...
	initScreen();
	glutDisplayFunc(display);
	glutMouseFunc(mouseClicks);
	glutMotionFunc(mouseMoves);
	glutMainLoop();

    // Cleanup
    for (auto item : concatenatedLists) delete item;

	return 0;
}

void init(){
    int ColorCol = 1, guiZIndex = 5, ColorsZIndex = 6, butz = 100;
    float csep = 1.1, ColorBoxHeight = 3.5 / 10.0, height = 2.8/3*1.0/20, width = .8/20;

    // Define the ArtCanvas area
    TheArtCanvas = new ArtCanvas(Rect(0, 0, .8, 1.0), {0, 0, 0});
    // This boundryShape defines the GUI area
    lists.push_back(new BoundryShape(Rect(0.8, .0, 0.2, 1.0), gray7));

    float shos = .12, sha = .1, shapey = 1, shawidth = .15, sqadj = 1.02;
    // Define the shape selectors
    lists.push_back(new Shape(Rect(0.8 + .025, shapey - shos * 1, shawidth, sha), Color(1,1,1), butz, "hex"));
    lists.push_back(new Shape(Rect(0.8 + .025, shapey - shos * 2, shawidth, sha), Color(1,1,1), butz, "circle"));

    lists.push_back(new Shape(Rect(0.8 + .025, shapey- shos * 3, shawidth, sha), Color(1,1,1), butz, "square"));
    // Draw a square to go over the square selector
    lists.push_back(new BoundryShape(Rect(.1, .1, .1, .1), Color(.6, .6, .6), 101));
    lists[lists.size() - 1]->setGLuint(drawRectOutline({(.8f+.025f)*(float)sqadj, (shapey - shos*3)*sqadj, shawidth * .8, sha * .75},  Color(.6, .6, .6)));

    // Define the action selectors
    lists.push_back(new Mode(Rect(0.8 + .025, shapey - shos * 4, shawidth, sha), Color(1,1,1), butz, "translate"));
    lists.push_back(new Mode(Rect(0.8 + .025, shapey - shos * 5, shawidth, sha), Color(1,1,1), butz, "rotate"));
    lists.push_back(new Mode(Rect(0.8 + .025, shapey - shos * 6, shawidth, sha), Color(1,1,1), butz, "resize"));

    // This boundryshape defines the zone for the color selectors
    lists.push_back(new BoundryShape(Rect(0.8, 0, 0.2, 3.0/10), gray7));

    // instantiate 8 color selectors
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * ColorCol,       ColorBoxHeight - 1.0 / 5, width, height), Color(1, 0, 0), ColorsZIndex, "red"));
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * (ColorCol + 1), ColorBoxHeight - 1.0 / 5, width, height), Color(1, 1, 0), ColorsZIndex, "yellow"));
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * (ColorCol + 2), ColorBoxHeight - 1.0 / 5, width, height), Color(0, 1, 0), ColorsZIndex, "green"));

    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * ColorCol,       ColorBoxHeight - 1.3 / 5, width, height), Color(0, 1, 1), ColorsZIndex, "cyan"));
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * (ColorCol + 1), ColorBoxHeight - 1.3 / 5, width, height), Color(0, 0, 1), ColorsZIndex, "blue"));
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * (ColorCol + 2), ColorBoxHeight - 1.3 / 5, width, height), Color(1, 0, 1), ColorsZIndex, "pink"));

    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * ColorCol,       ColorBoxHeight - 1.6 / 5, width, height), Color(0, 0, 0), ColorsZIndex, "black"));
    selectors.push_back(new Selector(Rect(0.8 + (.2/5) * (ColorCol + 1), ColorBoxHeight - 1.6 / 5, width, height), Color(1, 1, 1), ColorsZIndex, "white"));

    // These two lines make a 'grid' effect for the color selectors.
    lists.push_back(new BoundryShape(Rect(0.8 + (.2/5) * (ColorCol + 1), ColorBoxHeight - 1.6 / 5, .005, height * 4), gray7, ColorsZIndex+1));
    lists.push_back(new BoundryShape(Rect(0.8 + (.2/5) * (ColorCol + 2), ColorBoxHeight - 1.6 / 5, .005, height * 4), gray7, ColorsZIndex+1));

    // This line divides Color selector area from Transformation area
    lists.push_back(new BoundryShape(Rect(0.81, ColorBoxHeight - .090, .175, .01), gray4, butz + 1));

    // This line divides Transformation area from the Object Types area
    lists.push_back(new BoundryShape(Rect(0.81, .63, .175, .01), gray4, butz + 1));

    // Instantiate the currentColor displayer
    lists.push_back(new currentColor(Rect(0.8 + (.1/5) * (ColorCol + 3), ColorBoxHeight - .7 / 5, width * 2, height), Color(), ColorsZIndex+2));

    TheModel.currentColor = Color(1, 1, 1); // For consistency's sake

    // Concatenate lists; concatenatedLists is used in the display() function
    concatenatedLists.insert(concatenatedLists.end(), lists.begin(), lists.end());
    concatenatedLists.insert(concatenatedLists.end(), selectors.begin(), selectors.end());
    // Sort the lists by zIndex. This allows drawing in the correct order.
    for (int index = 1, end = concatenatedLists.size(); index < end; index++)
       if (concatenatedLists[index-1]->zIndex > concatenatedLists[index]->zIndex)
            swap(concatenatedLists[index-1], concatenatedLists[index]), index = 1; // ',' saved me two lines of code!
    
}
//===== initScreen: set up the 2D canvas ==================
void initScreen(){
	glClearColor(0., 0., 0., 0.);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,1.0,0.0,1.0);	// set coord for viewport
	glViewport(0,0,x_win,y_win);	// set viewport
}

//===== display: perform OpenGL drawing on the canvas =========
void display(){
	
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClearColor(0,0,0,0);

    for (auto s : TheArtCanvas->getShapes())
        glCallList(s.getGLuint());

    // Draw the GUI to the canvas
    for (auto li : concatenatedLists)
        glCallList(li->getGLuint());
	
    // Draw some bitmap text to the canvas
    drawString(.81, .97, "Object Types", Color(0, 0 ,0));
    drawString(.8, .61, "Transformation", Color(0, 0 ,0));
    drawString(.8, .22, "Color", Color(0, 0 ,0));
    

    // Draw the icons for selectors
    drawCircle(.895, .81, .04, gray6);
    drawHex(.895, .93, .04f, gray6);
    drawPlus(.895, .565, .04f, gray6);
    drawRotIcon(.895, .45, .04f, gray6);
    drawResizeIcon(.84, .29);

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
        for (auto li: lists)
            li->mouseClicks({gx, gy, but, sta});
        for (auto li: selectors)
            li->mouseClicks({gx, gy, but, sta});
        TheArtCanvas->mouseClicks({gx, gy, but, sta});

        if (DEBUG) cout << "Click at: " << "x:" << setw(5) << gx << "\ty:" << setw(5) << gy << endl;
	}
	
	if(button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN){
		// Your code here...
	}
	
	glutPostRedisplay();
}

//=== mouseMoves: handle right mouse moves =============
void mouseMoves(int x, int y){

	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN){		
		gx = float(x)/x_win;
		gy = float(y_win-y)/y_win;
		// Your code here...	
        TheArtCanvas->mouseMoves({gx, gy, button, state});
	}
	
	glutPostRedisplay();
}
//=== Functions to draw required shapes
GLuint drawRect(Rect r, float h, Color c){
    GLuint ID = glGenLists(1);    
    glNewList(ID, GL_COMPILE);

    glColor3f(c.r, c.g, c.b); 
    glBegin(GL_QUADS);
    glVertex3f(r.x, r.y, 0);
    glVertex3f(r.x + r.w, r.y, 0);
    glVertex3f(r.x + r.w, r.y + r.h, 0);
    glVertex3f(r.x, r.y + r.h, 0);
    glEnd();    

    glEndList();
    return ID;
}
GLuint drawRectOutline(Rect r, Color c){
    GLuint ID = glGenLists(1);    
    glNewList(ID, GL_COMPILE);

    glColor3f(c.r, c.g, c.b); 
    glLineWidth(4);
    glBegin(GL_LINE_STRIP);
    glVertex3f(r.x,         r.y, 0);
    glVertex3f(r.x + r.w,   r.y, 0);
    glVertex3f(r.x + r.w,   r.y + r.h, 0);
    glVertex3f(r.x,         r.y + r.h, 0);
    glVertex3f(r.x,         r.y, 0);
    glEnd();    

    glEndList();
    return ID;
}
void drawString(float x, float y, char * text, Color c){
    glColor3f(c.r, c.g, c.b);
    glRasterPos2f(x, y);
    for (char * p = text; *p; p++)
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *p);
}
void drawCircle(float x, float y, float r, Color c){
    glBegin(GL_LINE_STRIP);
    glColor3f(c.r, c.g, c.b);
    for (float angle = 0, twoP = M_PI * 2.00; angle <= twoP; angle += .1)
        glVertex3f(x+cos(angle)*r, y+sin(angle)*r, 0.0f);   
    glEnd();
}
void drawHex(float x, float y, float r, Color c){
    glColor3f(c.r, c.g, c.b);
    glBegin(GL_LINE_STRIP);
    glLineWidth(2);
    for (float angle = 0, end = M_PI * 2.00; angle <= end; angle += M_PI / 3.0f)
        glVertex3f(x+cos(angle)*r, y+sin(angle)*r, 0.0f);   
    glEnd();
}
void drawPlus(float x, float y, float r, Color c){
    glColor3f(c.r, c.g, c.b);
    glLineWidth(5);
    glBegin(GL_LINES);
    for (float angle = 0, end = M_PI * 2.0; angle <= end; angle += M_PI / 2){
        glVertex3f(x+cos(angle)*r, y+sin(angle)*r, 0.0f);   
        glVertex3f(x+cos(angle+M_PI)*r, y+sin(angle+M_PI)*r, 0.0f);   
    }
    glEnd();
}
void drawRotIcon(float x, float y, float r, Color c){
    glBegin(GL_LINE_STRIP);
    glColor3f(c.r, c.g, c.b);
    for (float angle = 0, end = M_PI * 3/2.0f; angle <= end; angle += 0.1f)
        glVertex3f(x+cos(angle)*r, y+sin(angle)*r, 0.0f);   
    glEnd();
}
void drawResizeIcon(float x, float y){
    float w = 0.08f, adj = .01;
    glColor3f(gray4.r, gray4.g, gray4.b);
    glBegin(GL_QUADS);
    glVertex3f(x + adj ,       y + adj       ,0.0);    
    glVertex3f(x + w + adj,  y + adj       ,0.0);    
    glVertex3f(x+ w + adj,   y+w     ,0.0);    
    glVertex3f(x + adj,       y+w ,0.0);    
    glEnd();    

    // Now, the smaller square
    w = 0.05f;
    glColor3f(gray7.r, gray7.g, gray7.b);
    glBegin(GL_QUADS);
    glVertex3f(x,       y       ,0.0);    
    glVertex3f(x + w,  y       ,0.0);    
    glVertex3f(x+ w,   y+w    ,0.0);    
    glVertex3f(x,       y+w    ,0.0);    
    glEnd();    
}
