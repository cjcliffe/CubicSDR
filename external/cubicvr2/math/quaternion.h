//
//  quaternion.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__quaternion__
#define __CubicVR2__quaternion__

#include <iostream>
#include "vec4.h"
#include "mat4.h"

namespace CubicVR {
    
    struct quaternion : vec4 {
        
        static vec4 fromMatrix(mat4 mat) {
            __float t = 1 + mat[0] + mat[5] + mat[10];
            __float S,X,Y,Z,W;
            
            if ( t > 0.00000001 ) {
                S = sqrtf(t) * 2;
                X = ( mat[9] - mat[6] ) / S;
                Y = ( mat[2] - mat[8] ) / S;
                Z = ( mat[4] - mat[1] ) / S;
                W = 0.25f * S;
            } else {
                if ( mat[0] > mat[5] && mat[0] > mat[10] )  {	// Column 0:
                    S  = sqrtf( 1.0f + mat[0] - mat[5] - mat[10] ) * 2.0f;
                    X = 0.25f * S;
                    Y = (mat[4] + mat[1] ) / S;
                    Z = (mat[2] + mat[8] ) / S;
                    W = (mat[9] - mat[6] ) / S;
                } else if ( mat[5] > mat[10] ) {			// Column 1:
                    S  = sqrtf( 1.0f + mat[5] - mat[0] - mat[10] ) * 2.0f;
                    X = (mat[4] + mat[1] ) / S;
                    Y = 0.25f * S;
                    Z = (mat[9] + mat[6] ) / S;
                    W = (mat[2] - mat[8] ) / S;
                } else {						// Column 2:
                    S  = sqrtf( 1.0f + mat[10] - mat[0] - mat[5] ) * 2.0f;
                    X = (mat[2] + mat[8] ) / S;
                    Y = (mat[9] + mat[6] ) / S;
                    Z = 0.25f * S;
                    W = (mat[4] - mat[1] ) / S;
                }
            }
            
            return vec4(X,Y,Z,W);
        };
        
        static vec4 fromEuler(__float bank, __float heading, __float pitch) { // x,y,z
            __float c1 = cosf(((float)M_PI / 180.0f) * heading / 2.0f);
            __float s1 = sinf(((float)M_PI / 180.0f) * heading / 2.0f);
            __float c2 = cosf(((float)M_PI / 180.0f) * pitch / 2.0f);
            __float s2 = sinf(((float)M_PI / 180.0f) * pitch / 2.0f);
            __float c3 = cosf(((float)M_PI / 180.0f) * bank / 2.0f);
            __float s3 = sinf(((float)M_PI / 180.0f) * bank / 2.0f);
            __float c1c2 = c1 * c2;
            __float s1s2 = s1 * s2;
            
            vec4 mOut;
            
            mOut[0] = c1c2 * c3 - s1s2 * s3;
            mOut[1] = c1c2 * s3 + s1s2 * c3;
            mOut[2] = s1 * c2 * c3 + c1 * s2 * s3;
            mOut[3] = c1 * s2 * c3 - s1 * c2 * s3;
            
            return mOut;
        };
        
        static vec3 toEuler(vec4 q) {
            __float sqx = q[0] * q[0];
            __float sqy = q[1] * q[1];
            __float sqz = q[2] * q[2];
            __float sqw = q[3] * q[3];
            
            __float x = (180.0f / (float)M_PI) * ((atan2f(2.0f * (q[1] * q[2] + q[0] * q[3]), (-sqx - sqy + sqz + sqw))));
            __float y = (180.0f / (float)M_PI) * ((asinf(-2.0f * (q[0] * q[2] - q[1] * q[3]))));
            __float z = (180.0f / (float)M_PI) * ((atan2f(2.0f * (q[0] * q[1] + q[2] * q[3]), (sqx - sqy - sqz + sqw))));
            
            return vec3(x, y, z);
        };
        
        static vec4 multiply(vec4 q1, vec4 q2) {
            __float x = q1[0] * q2[3] + q1[3] * q2[0] + q1[1] * q2[2] - q1[2] * q2[1];
            __float y = q1[1] * q2[3] + q1[3] * q2[1] + q1[2] * q2[0] - q1[0] * q2[2];
            __float z = q1[2] * q2[3] + q1[3] * q2[2] + q1[0] * q2[1] - q1[1] * q2[0];
            __float w = q1[3] * q2[3] - q1[0] * q2[0] - q1[1] * q2[1] - q1[2] * q2[2];
            
            return vec4(x,y,z,w);
        };
        
    };
}

#endif /* defined(__CubicVR2__quaternion__) */
