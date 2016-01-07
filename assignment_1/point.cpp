/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW1: Lines                         */ 
/* Student Name: Joseph Burger        */ 
/* SID: 20015548                      */ 
/**************************************/  
#include <memory>
using namespace std;

/*
 * A point has two values in two dimensions
 */
struct point
{
    // x will hold horizontal pixel distance, y vertical
    int x, y;
    // constructors
    point() : x(0), y(0) {};
    point(int x, int y) : x(x), y(y) {};
};

/*
 * A line is composed of two points.
 */
struct line {
    // By using a shared_ptr, I don't need to make a deconstructor
    shared_ptr <point> p1;
    shared_ptr <point> p2;
    // constructors
    line (point * p1, point * p2) {
        this->p1 = shared_ptr<point>(p1); 
        this->p2 = shared_ptr<point>(p2);
    };
};
