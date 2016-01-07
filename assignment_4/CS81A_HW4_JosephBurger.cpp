/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW4: 3D Viewer                     */
/* Student Name: Joseph Burger        */ 
/* SID: 20015548                      */
/**************************************/  
/*
This program is a 2D graphics editor letting
the user scale, rotate and transform squares,
hexagons and circles.

If the user hits the "m" key, the program will
switch to 3D viewer mode, where the user may
view the drawn objects in 3D. The position, 
color and scale of objects is preserved.

The user may rotate the scene with the Q
and D keys, or by clicking and dragging the
mouse. The camera may also be moved forward or
backwards with W and X.
*/
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

struct Color {
    float r, g, b;
    Color(float r = (float)(rand() % 256)/256, 
    float g = (float)(rand() % 256)/256, 
    float b = (float)(rand() % 256)/256)
        : r(r), g(g), b(b) {}
    inline bool operator==(const Color& lhs){ return lhs.r == r && lhs.g == g && lhs.b == b;  }
};

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
    // false == 2d, true == 3d;
    bool display = false;
};

/*----- GLOBAL VARIABLES ------------*/
int button, state = 1;
float gx,gy;			// gx and gy are coordinates of 
						// a clicked point on screen
float x_win = 600.0;
float y_win = 400.0;
// Assignment 4 - Additional global variables
float z_win = 650.0;
float obj_material_amb[4] = {0.3,0.5,0.2,1.0};
float obj_material_diff[4] = {0.9,0.9,0.9,1.0};
float obj_material_spec[4] = {0.1,0.1,0.1,1.0};
float VIEWER[3]={400.0,300.0,0.0};
float LOOKAT[3]={static_cast<float>(x_win/2.0),0.0,static_cast<float>(z_win/2.0)};
Vector view_dir;           // direction vector from viewer to look-at-point
Color gray4(.4, .4, .4), gray5(.5, .5, .5), gray6(.6, .6, .6), gray7(.7, .7, .7);
Model TheModel;

// ======= Classes for assignment 3 =======
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
/*----- Assignment 3: FUNCTION DECLARATION --------*/
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

/*----- Assignment 4: FUNCTIONS DECLARATION --------*/
// If the current mode is 3D, the display function will simply
// call displayScreen, then end.
void screenSetUp();
void displayScreen();

// Keypresses is used from 3dtemplate cpp and moves the viewer's camera.
void keyPresses(unsigned char c, int x, int y);
// Helper funtions for keyPresses
void MoveUpDown(float step);
void RotateLeftRight(float angle);

// Now I know what normals are - They're like perpendicular vectors, only
// they're perpendicular to three vectors, not just two planes! I should
// really start taking some math classes...
Vector* calculateNormal(Point* a, Point* b, Point* c);
// Also didn't know what the right hand rule was until now; it's for normals!

// Functions for drawing 3d shapes - Sphere uses glut's solidsphere method
void DrawCube();
void DrawDiamond();
float xTranslateAmount = 0.0f, yTranslateAmount = 0.0f;

//===== main function ==================================
int main(int argc, char**argv){
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);

	// Create a window
	glutCreateWindow("Art Painter - 3D Edition");
	glutPositionWindow(0, 0);
	glutReshapeWindow(x_win, y_win);
	// STUDENT FUNCTION CALL(s)
    init();

	// Program start here...
	initScreen();
	glutDisplayFunc(display);
	glutMouseFunc(mouseClicks);
	glutMotionFunc(mouseMoves);
    glutKeyboardFunc(keyPresses);
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
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0,1.0,0.0,1.0);	// set coord for viewport
	glViewport(0,0,x_win,y_win);	// set viewport
}

//===== display: perform OpenGL drawing on the canvas =========
void display(){

    // Optionally switch to 3d mode
    if (TheModel.display)
        displayScreen();

    // ==============================
    // Switch back to 2D mode
    initScreen();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_NORMALIZE);
    glDisable(GL_LIGHT0);
    glDisable(GL_LIGHT1);
    // ===========================

    if (!TheModel.display){
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(0,0,0,0);

        for (auto s : TheArtCanvas->getShapes())
            glCallList(s.getGLuint());
    }
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
        if (TheModel.display) return; // This stops the placement of objects in 3D mode
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
        float oldx = gx;
		gx = float(x)/x_win;
		gy = float(y_win-y)/y_win;
		// Your code here...	
        TheArtCanvas->mouseMoves({gx, gy, button, state});

        if (TheModel.display)
            RotateLeftRight(0.01 * (oldx > gx ? 1 : -1));
            
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

// ASSIGNMENT 4 FUNCTION DEFINITIONS

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
    gluPerspective(120.0,(x_win * 4/5 )/z_win,5.0,5000.0);
    glViewport(0,0,x_win * 4/5,z_win);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    
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

//===== displayScreen ===================================
void displayScreen(){
    
    screenSetUp();

    // Lighting code from Lecture 14
    GLfloat amb_color[] = { 0.2, 0.2, 0.2, 1.0 };
    GLfloat diff_color[] = { 1.0, 1.0, 1.0, 1.0 };
    GLfloat spec_color[] = { 1.0,1.0,1.0,   1.0 };
    GLfloat pos[] = {1.0, 0.0, 0.0, 1.0 }; // point light source at (0, 1, )

    glLightfv(GL_LIGHT1, GL_AMBIENT, amb_color);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diff_color);
    glLightfv(GL_LIGHT1, GL_SPECULAR, spec_color);
    glLightfv(GL_LIGHT1, GL_POSITION, pos);

    glEnable(GL_LIGHT1);

    // Your code here for drawing objects...
        for (auto s : TheArtCanvas->getShapes()){
            glPushMatrix();
            glTranslatef(6/5 * x_win * s.center.first, 20, z_win * s.center.second);
            glScalef(s.radius * 250.0, s.radius * 250.0, s.radius * 250.0);
            glRotatef(0.0, 0.0, s.angle, 0);

            // Set the color.
            float arr[4] = {s.c.r, s.c.g, s.c.b, 1.0};
            glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, arr);	
            glMaterialfv(GL_FRONT, GL_SPECULAR, arr);             
            
            if (s.type == "square"){
                DrawCube();    
            }   
            else if (s.type == "hex"){
                glScalef(s.radius * 50.0, s.radius * 50.0, s.radius * 50.0);
                DrawDiamond();
            }
            else if (s.type == "circle"){
                glScalef(.25, 0.25, 0.25);
                glutSolidSphere(10, 20, 20);
            }
            glPopMatrix();
        }

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, obj_material_amb);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, obj_material_diff);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, obj_material_spec);
    
    glutSwapBuffers();
}

//===== keyPresses ====================================
void keyPresses(unsigned char c, int x, int y){
    
    if (c =='w'){ MoveUpDown(10.0);}
    else if (c =='x'){ MoveUpDown(-10.0);}
    else if (c =='a'){ RotateLeftRight(10*M_PI/180.0);}
    else if (c =='d'){ RotateLeftRight(-10*M_PI/180.0);}
    
    // Your code here...
    else if (c == 'r'){ xTranslateAmount += 1; }
    else if (c == 'l'){ xTranslateAmount -= 1; }

    else if (c == 'u'){ yTranslateAmount += 1; }
    else if (c == 'U'){ yTranslateAmount -= 1; }

    // If the user wants to change display mode
    else if (c == 'm'){ TheModel.display = !TheModel.display; }
    
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

/*
DrawCube draws a 3d cube by drawing all 6 sides, once at a time.
*/
void DrawCube(){
    glMatrixMode(GL_MODELVIEW);
    glBegin(GL_QUADS);        

    glNormal3f( 1.0f, 1.0f, 1.0f);    
    glVertex3f( 1.0f, 1.0f, 1.0f);    
    glVertex3f(-1.0f, 1.0f, 1.0f);    
    glVertex3f(-1.0f,-1.0f, 1.0f);    
    glVertex3f( 1.0f,-1.0f, 1.0f);    

    glNormal3f( 1.0f,-1.0f,-1.0f);    
    glVertex3f( 1.0f,-1.0f,-1.0f);    
    glVertex3f(-1.0f,-1.0f,-1.0f);    
    glVertex3f(-1.0f, 1.0f,-1.0f);    
    glVertex3f( 1.0f, 1.0f,-1.0f);    

    glNormal3f(-1.0f, 1.0f, 1.0f);    
    glVertex3f(-1.0f, 1.0f, 1.0f);    
    glVertex3f(-1.0f, 1.0f,-1.0f);    
    glVertex3f(-1.0f,-1.0f,-1.0f);    
    glVertex3f(-1.0f,-1.0f, 1.0f);    

    glNormal3f( 1.0f, 1.0f,-1.0f);    
    glVertex3f( 1.0f, 1.0f,-1.0f);    
    glVertex3f( 1.0f, 1.0f, 1.0f);    
    glVertex3f( 1.0f,-1.0f, 1.0f);    
    glVertex3f( 1.0f,-1.0f,-1.0f);    

    glNormal3f( 1.0f, 1.0f,-1.0f);    
    glVertex3f( 1.0f, 1.0f,-1.0f);    
    glVertex3f(-1.0f, 1.0f,-1.0f);    
    glVertex3f(-1.0f, 1.0f, 1.0f);    
    glVertex3f( 1.0f, 1.0f, 1.0f);    

    glNormal3f( 1.0f,-1.0f, 1.0f);    
    glVertex3f( 1.0f,-1.0f, 1.0f);    
    glVertex3f(-1.0f,-1.0f, 1.0f);    
    glVertex3f(-1.0f,-1.0f,-1.0f);    
    glVertex3f( 1.0f,-1.0f,-1.0f);    

    glEnd();            
}

/*
Triangle fan makes a triangle from 3 vertices, then attaches each following vertice
to the first and last vertice. It can be used to make "fan" like shapes, or in this
case, a 3d pyramid shape. Two of these facing opposite makes a diamond
Tip at origin: 0, 0, 0
2nd vertext: 0, 1, 0
3rd: 1, 1, 0

THEN, make a 2nd triangle with
0, 0, 1
THEN, make the third triangle with
0, 1, 1
*/
// Draw a diamond by drawing 2 sets of 3 triangles.
void DrawDiamond() {
    glMatrixMode(GL_MODELVIEW);
    glBegin(GL_TRIANGLE_FAN);

    glNormal3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);

    // The 2nd Triangle
    glVertex3f(0.0f, 0.0f, 1.0f);
    // The 3rd Triangle
    glVertex3f(0.0f, 1.0f, 0.0f);
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glNormal3f(1.0f, 1.0f, 1.0f);
    glVertex3f(1.0f, 1.0f, 1.0f);
    glVertex3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);

    // The 2nd Triangle
    glVertex3f(0.0f, 0.0f, 1.0f);
    // The 3rd Triangle
    glVertex3f(0.0f, 1.0f, 0.0f);

    glEnd();
}
