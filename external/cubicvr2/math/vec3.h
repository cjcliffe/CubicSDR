//
//  vec3.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-21.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__vec3__
#define __CubicVR2__vec3__

#include <iostream>
#include "cubic_types.h"
#include <cmath>
#include <stddef.h>

namespace CubicVR {
    
#define vec3SG(c,x,y) \
    vec3 COMBINE(get,x)() { return y; } \
    c & COMBINE(set,x)(vec3 value) { y = value; return *this; }
    

    struct vec3 {
        __float x,y,z;

        __float& r() { return x; }
        __float& g() { return y; }
        __float& b() { return z; }
        
        //access as-array:
        inline __float& operator [] (size_t i) {
            __float* as_array = (__float*)this;
            return (as_array[i]);
        }

        inline const __float& operator [] (size_t i) const {
            __float* as_array = (__float*)this;
            return (as_array[i]);
        }

        //compare to ZERO-filled vector
        inline operator bool() const {

            return (x != 0.0f || y != 0.0f || z != 0.0f);
        }

        vec3 (__float xi,__float yi,__float zi) { x = xi; y = yi; z = zi; }
        vec3 () { x = y = z = 0.0f; }
        
        vec3 operator*(__float v) { return vec3(x*v, y*v, z*v); }
        vec3 operator*(vec3 v) { return vec3::cross(*this,v); }
        vec3 operator+(vec3 v) { return vec3::add(*this,v); }
        vec3 operator-(vec3 v) { return vec3::subtract(*this,v); }
        
        
        static __float length(vec3 pta, vec3 ptb) {
            __float a,b,c;
            a = ptb[0]-pta[0];
            b = ptb[1]-pta[1];
            c = ptb[2]-pta[2];
            return sqrtf((a*a) + (b*b) + (c*c));
        };
        static __float length(vec3 pta) {
            __float a,b,c;
            a = pta[0];
            b = pta[1];
            c = pta[2];
            return sqrtf((a*a) + (b*b) + (c*c));
        };
        static vec3 normalize(vec3 pt) {
            __float a = pt[0], b = pt[1], c = pt[2],
            d = sqrtf((a*a) + (b*b) + (c*c));
            if (d) {
                pt[0] = pt[0]/d;
                pt[1] = pt[1]/d;
                pt[2] = pt[2]/d;
                
                return pt;
            }
            
            pt = vec3(0.0f,0.0f,0.0f);
            
            return pt;
        };
        static __float dot(vec3 v1, vec3 v2) {
            return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
        };
        static __float angle(vec3 v1, vec3 v2) {
            __float a = acosf((v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2]) / (sqrtf(v1[0] * v1[0] + v1[1] * v1[1] + v1[2] * v1[2]) * sqrtf(v2[0] * v2[0] + v2[1] * v2[1] + v2[2] * v2[2])));
            
            return a;
        };
        static vec3 cross(vec3 vectA, vec3 vectB) {
            return vec3(
                vectA[1] * vectB[2] - vectB[1] * vectA[2], vectA[2] * vectB[0] - vectB[2] * vectA[0], vectA[0] * vectB[1] - vectB[0] * vectA[1]
            );
        };
        static vec3 multiply(vec3 vectA, __float constB) {
            return vec3(vectA[0] * constB, vectA[1] * constB, vectA[2] * constB);
        };
        static vec3 add(vec3 vectA, vec3 vectB) {
            return vec3(vectA[0] + vectB[0], vectA[1] + vectB[1], vectA[2] + vectB[2]);
        };
        
        static vec3 subtract(vec3 vectA, vec3 vectB) {
            return vec3(vectA[0] - vectB[0], vectA[1] - vectB[1], vectA[2] - vectB[2]);
        };
        
        static bool equal(vec3 a, vec3 b, __float epsilon = 0.0000001f) {
            return ((fabs(a[0] - b[0]) < epsilon) && (fabs(a[1] - b[1]) < epsilon) && (fabs(a[2] - b[2]) < epsilon));
        };
        
        static vec3 moveViewRelative(vec3 position, vec3 target, __float xdelta, __float zdelta) {
            __float ang = atan2f(zdelta, xdelta);
            __float cam_ang = atan2f(target[2] - position[2], target[0] - position[0]);
            __float mag = sqrtf(xdelta * xdelta + zdelta * zdelta);
            
            __float move_ang = cam_ang + ang + (float)M_PI/2.0f;
            
            //        if (typeof(alt_source) === 'object') {
            //            return [alt_source[0] + mag * Math.cos(move_ang), alt_source[1], alt_source[2] + mag * Math.sin(move_ang)];
            //        }
            
            return vec3(position[0] + mag * cosf(move_ang), position[1], position[2] + mag * sinf(move_ang));
        };
        
        static vec3 trackTarget(vec3 position, vec3 target, __float trackingSpeed, __float safeDistance) {
            vec3 camv = vec3::subtract(target, position);
            vec3 dist = camv;
            __float fdist = vec3::length(dist);
            vec3 motionv = camv;
            
            motionv = vec3::normalize(motionv);
            motionv = vec3::multiply(motionv, trackingSpeed * (1.0f / (1.0f / (fdist - safeDistance))));
            
            vec3 ret_pos;
            
            if (fdist > safeDistance) {
                ret_pos = vec3::add(position, motionv);
            } else if (fdist < safeDistance) {
                motionv = camv;
                motionv = vec3::normalize(motionv);
                motionv = vec3::multiply(motionv, trackingSpeed * (1.0f / (1.0f / (fabsf(fdist - safeDistance)))));
                ret_pos = vec3::subtract(position, motionv);
            } else {
                ret_pos = vec3(position[0], position[1] + motionv[2], position[2]);
            }
            
            return ret_pos;
        };
        
        static vec3 getClosestTo(vec3 ptA, vec3 ptB, vec3 ptTest) {
            vec3 S, T, U;
            
            S = vec3::subtract(ptB, ptA);
            T = vec3::subtract(ptTest, ptA);
            U = vec3::add(vec3::multiply(S, vec3::dot(S, T) / vec3::dot(S, S)), ptA);
            
            return U;
        };
        
        //        linePlaneIntersect: function(normal, point_on_plane, segment_start, segment_end)
        //        {
        //            // form a plane from normal and point_on_plane and test segment start->end to find intersect point
        //            var denom,mu;
        //
        //            var d = - normal[0] * point_on_plane[0] - normal[1] * point_on_plane[1] - normal[2] * point_on_plane[2];
        //
        //            // calculate position where the plane intersects the segment
        //            denom = normal[0] * (segment_end[0] - segment_start[0]) + normal[1] * (segment_end[1] - segment_start[1]) + normal[2] * (segment_end[2] - segment_start[2]);
        //            if (Math.fabs(denom) < 0.001) return false;
        //
        //            mu = - (d + normal[0] * segment_start[0] + normal[1] * segment_start[1] + normal[2] * segment_start[2]) / denom;
        //            return [
        //                    (segment_start[0] + mu * (segment_end[0] - segment_start[0])),
        //                    (segment_start[1] + mu * (segment_end[1] - segment_start[1])),
        //                    (segment_start[2] + mu * (segment_end[2] - segment_start[2]))
        //                    ];
        //        }
    };
    
}
#endif /* defined(__CubicVR2__vec3__) */
