//
//  vec2.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__vec2__
#define __CubicVR2__vec2__

#include <iostream>
#include <cmath>
#include "cubic_types.h"

namespace CubicVR {
    #define vec2SG(c,x,y) \
        vec2 COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(vec2 value) { y = value; return *this; }

    struct vec2 {
        __float x, y;
    public:
        __float& u() { return x; }
        __float& v() { return y; }
        
//        __float  operator [] (unsigned i) const { return ((__float *)this)[i]; }
        __float& operator [] (unsigned i)       { return ((__float *)this)[i]; }
        
        vec2 (__float xi,__float yi) { x = xi; y = yi;  }
        vec2 () { x = y = 0.0f; }
        
        operator __float*() const { return (__float *)this; }
        
        vec2 operator*(__float v) { return vec2( x*v, y*v ); }
        //    vec2 operator*(vec2 v) { return vec2::cross(*this,v); }
        vec2 operator+(vec2 v) { return vec2::add(*this,v); }
        vec2 operator-(vec2 v) { return vec2::subtract(*this,v); }
        
        
        static bool equal(vec2 a, vec2 b, __float epsilon = 0.00000001) {
            return (fabs(a[0] - b[0]) < epsilon && fabs(a[1] - b[1]) < epsilon);
        };
        
        static bool onLine(vec2 a, vec2 b,vec2 c) {
            __float minx = (a[0]<b[0])?a[0]:b[0];
            __float miny = (a[1]<b[1])?a[1]:b[1];
            __float maxx = (a[0]>b[0])?a[0]:b[0];
            __float maxy = (a[1]>b[1])?a[1]:b[1];
            
            if ((minx <= c[0] && c[0] <= maxx) && (miny <= c[1] && c[1] <= maxy)) {
                return true;
            } else {
                return false;
            }
        };
        
        static vec2 lineIntersect(vec2 a1, vec2 a2, vec2 b1, vec2 b2) {
            __float x1 = a1[0], y1 = a1[1], x2 = a2[0], y2 = a2[1];
            __float x3 = b1[0], y3 = b1[1], x4 = b2[0], y4 = b2[1];
            
            __float d = ((x1-x2) * (y3-y4)) - ((y1-y2) * (x3-x4));
            if (d == 0) return vec2(INFINITY,INFINITY);
            
            __float xi = (((x3-x4) * ((x1*y2)-(y1*x2))) - ((x1-x2) *((x3*y4)-(y3*x4))))/d;
            __float yi = (((y3-y4) * ((x1*y2)-(y1*x2))) - ((y1-y2) *((x3*y4)-(y3*x4))))/d;
            
            return vec2( xi,yi );
        };
        
        static vec2 add(vec2 a,vec2 b) {
            return vec2(a[0]+b[0],a[1]+b[1]);
        };
        
        static vec2 subtract(vec2 a, vec2 b) {
            return vec2(a[0]-b[0],a[1]-b[1]);
        };
        
        static __float length(vec2 a,vec2 b) {
            vec2 s(a[0]-b[0],a[1]-b[1]);
            
            return sqrtf(s[0]*s[0]+s[1]*s[1]);
        };
        
        static __float length(vec2 a) {
            return sqrtf(a[0]*a[0]+a[1]*a[1]);
        };
        
    };
    
}
#endif /* defined(__CubicVR2__vec2__) */
