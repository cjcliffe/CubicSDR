//
//  mat3.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__mat3__
#define __CubicVR2__mat3__

#include <iostream>
#include "vec3.h"

namespace CubicVR {

    #define mat3SG(c,x,y) \
        mat3 COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(mat3 value) { y = value; return *this; }

    struct mat3 {
        __float a,b,c,d,e,f,g,h,i;

        //        __float  operator [] (unsigned i) const { return ((__float *)this)[i]; }
        __float& operator [] (unsigned i)       { return ((__float *)this)[i]; }
        operator __float*() const { return (__float *)this; }
        
        mat3(__float ai,__float bi,__float ci,__float di,__float ei,__float fi,__float gi,__float hi,__float ii) {
            a = ai; b = bi; c = ci; d = di; e = ei; f = fi; g = gi; h = hi; i = ii;
        };
        
        mat3() { memset((__float *)this, 0, sizeof(mat3)); }
        //        mat3 operator* (mat4 m) { return mat3::multiply(*this,m); };
        //        void operator*= (mat4 m) { *this = mat3::multiply(*this,m); };
 
        
        static mat3 identity() {
            return mat3(1.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 1.0f);
        }

        static void transpose_inline(mat3 &mat) {
            __float a01 = mat[1], a02 = mat[2], a12 = mat[5];
            
            mat[1] = mat[3];
            mat[2] = mat[6];
            mat[3] = a01;
            mat[5] = mat[7];
            mat[6] = a02;
            mat[7] = a12;
        };
        
        static mat3 transpose(mat3 mat_in) {
            __float a01 = mat_in[1], a02 = mat_in[2], a12 = mat_in[5];
            
            mat3 mat;
            
            mat[1] = mat_in[3];
            mat[2] = mat_in[6];
            mat[3] = a01;
            mat[5] = mat_in[7];
            mat[6] = a02;
            mat[7] = a12;
            
            return mat;
        };
        
        static vec3 multiply(mat3 m1, vec3 m2) {
            vec3 mOut;
            
            mOut[0] = m2[0] * m1[0] + m2[3] * m1[1] + m2[6] * m1[2] ;
            mOut[1] = m2[1] * m1[0] + m2[4] * m1[1] + m2[7] * m1[2] ;
            mOut[2] = m2[2] * m1[0] + m2[5] * m1[1] + m2[8] * m1[2];
            
            return mOut;
        };
    };
    
    
}

#endif /* defined(__CubicVR2__mat3__) */
