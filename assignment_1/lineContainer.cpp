/**************************************/ 
/* CS81A ­ Fall 2015                  */ 
/* HW1: Lines                         */ 
/* Student Name: Joseph Burger        */ 
/* SID: 20015548                      */ 
/**************************************/  

#include <point.cpp>
#include <vector>
#include <stdlib.h> // abs
using namespace std;

/*
 * lineContainer contains a vector of lines which get
 * drawn to the viewport. It also contains a useful 
 * function for finding points near a given point.
 */
class lineContainer
{
    // pixelDistance shall define the maximum distance two
    // points that are "near" eachother.
    int pixeldistance;
    public:
    vector<line> lines;
    // constructors
    lineContainer(){ 
        this->pixeldistance = 5; 
        this->lines = vector<line>();
    }
    lineContainer(int pixeldistance){
        pixeldistance = this->pixeldistance;
        lines = vector<line>();
    }
    
    // returns a vector of points near the click, or an empty vector
    vector<point*> getPointsNear(point click){
        vector<point*> near;
        for (int counter = 0, end = lines.size(); counter < end; counter++){
            if (abs(lines[counter].p1->x - click.x) <= pixeldistance && abs(lines[counter].p1->y - click.y) <= pixeldistance)
                near.push_back(lines[counter].p1.get());
            if (abs(lines[counter].p2->x - click.x) <= pixeldistance && abs(lines[counter].p2->y - click.y) <= pixeldistance)
                near.push_back(lines[counter].p2.get());
        }
        return near;
    }
};
