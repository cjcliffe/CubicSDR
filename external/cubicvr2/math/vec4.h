//
//  vec4.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__vec4__
#define __CubicVR2__vec4__

#include <iostream>
#include "cubic_types.h"
#include <cmath>
#include <stddef.h>

namespace CubicVR {

#define vec4SG(c,x,y) \
    vec3 COMBINE(get,x)() { return y; } \
    c & COMBINE(set,x)(vec3 value) { y = value; return *this; }

    struct vec4 {

        __float x, y, z, w;

    public:
        __float& r() { return x; }
        __float& g() { return y; }
        __float& b() { return z; }
        __float& a() { return w; }
        
        //access as-array:
        inline __float& operator [] (size_t i) {
            __float* as_array = (__float*)this;
            return (as_array[i]);
        }

        inline const __float& operator [] (size_t i) const {
            __float* as_array = (__float*)this;
            return (as_array[i]);
        }

        vec4 (__float xi,__float yi,__float zi,__float wi) { x = xi; y = yi; z = zi; w = wi; }
        vec4 () { x = y = z = w =  0.0f; }
        
        vec4 operator*(__float v) { return vec4(x*v, y*v, z*v, w*v); }
//        vec4 operator*(vec4 v) { return vec4::cross(*this,v); }
//        vec4 operator+(vec4 v) { return vec4::add(*this,v); }
//        vec4 operator-(vec4 v) { return vec4::subtract(*this,v); }

        static __float length(vec4 a, vec4 b) {
            __float v[4] = {a[0]-b[0],a[1]-b[1],a[2]-b[2],a[3]-b[3]};
            return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
        };

        static __float length(vec4 v) {
            return sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]);
        };
        
        static vec4 normalize(vec4 v) {
            __float n = sqrtf(vec4::length(v));
            
            v[0] /= n;
            v[1] /= n;
            v[2] /= n;
            v[3] /= n;
            
            return v;
        };
        
        static __float dot(vec4 v1, vec4 v2) {
            return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] + v1[3] * v2[3];
        };
        
    };

}

#endif /* defined(__CubicVR2__vec4__) */
