//
//  mat4.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-21.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__mat4__
#define __CubicVR2__mat4__

#include <iostream>
#include "cubic_types.h"
#include "vec3.h"
#include "vec4.h"
#include "mat3.h"
#include <cmath>

namespace CubicVR {
    using namespace std;
    #define mat4SG(c,x,y) \
        mat4 COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(mat4 value) { y = value; return *this; }
    struct mat4 {
        __float a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p;

        //        __float  operator [] (unsigned i) const { return ((__float *)this)[i]; }
#ifndef _WIN32
        __float& operator [] (unsigned i)       { return ((__float *)this)[i]; }
#endif

        operator __float*() const { return (__float *)this; }
        mat4(__float ai,__float bi,__float ci,__float di,__float ei,__float fi,__float gi,__float hi,__float ii,__float ji,__float ki,__float li,__float mi,__float ni,__float oi,__float pi) {
            a = ai; b = bi; c = ci; d = di; e = ei; f = fi; g = gi; h = hi; i = ii; j = ji; k = ki; l = li; m = mi; n = ni; o = oi; p = pi;
        }
        mat4() { memset(this,0,sizeof(mat4)); }
        mat4 operator* (mat4 m) { return mat4::multiply(*this, m, true); };
        void operator*= (mat4 m) { *this = mat4::multiply(*this, m, true); };
//        mat4 &operator= (const mat4 &m) { memcpy(this,(__float *)m,sizeof(__float)*16); return *this; };
        
        static mat4 identity() {
            return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        }
        
        static mat4 multiply(mat4 mLeft, mat4 mRight, bool /* updated */) {
            mat4 mOut;
            
            mOut[0] = mLeft[0] * mRight[0] + mLeft[4] * mRight[1] + mLeft[8] * mRight[2] + mLeft[12] * mRight[3];
            mOut[1] = mLeft[1] * mRight[0] + mLeft[5] * mRight[1] + mLeft[9] * mRight[2] + mLeft[13] * mRight[3];
            mOut[2] = mLeft[2] * mRight[0] + mLeft[6] * mRight[1] + mLeft[10] * mRight[2] + mLeft[14] * mRight[3];
            mOut[3] = mLeft[3] * mRight[0] + mLeft[7] * mRight[1] + mLeft[11] * mRight[2] + mLeft[15] * mRight[3];
            mOut[4] = mLeft[0] * mRight[4] + mLeft[4] * mRight[5] + mLeft[8] * mRight[6] + mLeft[12] * mRight[7];
            mOut[5] = mLeft[1] * mRight[4] + mLeft[5] * mRight[5] + mLeft[9] * mRight[6] + mLeft[13] * mRight[7];
            mOut[6] = mLeft[2] * mRight[4] + mLeft[6] * mRight[5] + mLeft[10] * mRight[6] + mLeft[14] * mRight[7];
            mOut[7] = mLeft[3] * mRight[4] + mLeft[7] * mRight[5] + mLeft[11] * mRight[6] + mLeft[15] * mRight[7];
            mOut[8] = mLeft[0] * mRight[8] + mLeft[4] * mRight[9] + mLeft[8] * mRight[10] + mLeft[12] * mRight[11];
            mOut[9] = mLeft[1] * mRight[8] + mLeft[5] * mRight[9] + mLeft[9] * mRight[10] + mLeft[13] * mRight[11];
            mOut[10] = mLeft[2] * mRight[8] + mLeft[6] * mRight[9] + mLeft[10] * mRight[10] + mLeft[14] * mRight[11];
            mOut[11] = mLeft[3] * mRight[8] + mLeft[7] * mRight[9] + mLeft[11] * mRight[10] + mLeft[15] * mRight[11];
            mOut[12] = mLeft[0] * mRight[12] + mLeft[4] * mRight[13] + mLeft[8] * mRight[14] + mLeft[12] * mRight[15];
            mOut[13] = mLeft[1] * mRight[12] + mLeft[5] * mRight[13] + mLeft[9] * mRight[14] + mLeft[13] * mRight[15];
            mOut[14] = mLeft[2] * mRight[12] + mLeft[6] * mRight[13] + mLeft[10] * mRight[14] + mLeft[14] * mRight[15];
            mOut[15] = mLeft[3] * mRight[12] + mLeft[7] * mRight[13] + mLeft[11] * mRight[14] + mLeft[15] * mRight[15];
            
            return mOut;
        };
        
        static vec3 multiply(mat4 m1, vec3 m2, bool /* updated */) {
            vec3 mOut;
            
            mOut[0] = m1[0] * m2[0] + m1[4] * m2[1] + m1[8] * m2[2] + m1[12];
            mOut[1] = m1[1] * m2[0] + m1[5] * m2[1] + m1[9] * m2[2] + m1[13];
            mOut[2] = m1[2] * m2[0] + m1[6] * m2[1] + m1[10] * m2[2] + m1[14];
            
            return mOut;
        }
        static mat4 frustum(__float left, __float right, __float bottom, __float top, __float zNear, __float zFar) {
            __float A = (right + left) / (right - left);
            __float B = (top + bottom) / (top - bottom);
            __float C = - (zFar + zNear) / (zFar - zNear);
            __float D = - (-2.0f * zFar * zNear) / (zFar - zNear);
            
            
            return mat4((2.0f * zNear) / (right - left), 0, A, 0,
                        0, (2.0f * zNear) / (top - bottom), B, 0,
                        0, 0, C, D,
                        0, 0, -1, 0);
        };
        static mat4 perspective(__float fovy, __float aspect, __float zNear, __float zFar) {
            __float yFac = tan(fovy * (float)M_PI / 360.0f);
            __float xFac = yFac * aspect;
            
            return mat4::frustum(-xFac, xFac, -yFac, yFac, zNear, zFar);

        };
        static mat4 ortho(__float left,__float right,__float bottom,__float top,__float znear,__float zfar) {
            return mat4(2.0f / (right - left), 0, 0, 0, 0, 2.0f / (top - bottom), 0, 0, 0, 0, -2.0f / (zfar - znear), 0, -(left + right) / (right - left), -(top + bottom) / (top - bottom), -(zfar + znear) / (zfar - znear), 1);
        };
        static __float determinant(mat4 m) {
            
            __float a0 = m[0] * m[5] - m[1] * m[4];
            __float a1 = m[0] * m[6] - m[2] * m[4];
            __float a2 = m[0] * m[7] - m[3] * m[4];
            __float a3 = m[1] * m[6] - m[2] * m[5];
            __float a4 = m[1] * m[7] - m[3] * m[5];
            __float a5 = m[2] * m[7] - m[3] * m[6];
            __float b0 = m[8] * m[13] - m[9] * m[12];
            __float b1 = m[8] * m[14] - m[10] * m[12];
            __float b2 = m[8] * m[15] - m[11] * m[12];
            __float b3 = m[9] * m[14] - m[10] * m[13];
            __float b4 = m[9] * m[15] - m[11] * m[13];
            __float b5 = m[10] * m[15] - m[11] * m[14];
            
            __float det = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
            
            return det;
        };
        //        coFactor: function (m, n, out) {
        //            // .. todo..
        //        },
        
        static mat4 transpose(mat4 m) {
            return mat4(m[0], m[4], m[8], m[12], m[1], m[5], m[9], m[13], m[2], m[6], m[10], m[14], m[3], m[7], m[11], m[15]);
        };
        
        static mat3 inverse_mat3(mat4 mat) {
            mat3 dest;

            __float a00 = mat[0], a01 = mat[1], a02 = mat[2],
            a10 = mat[4], a11 = mat[5], a12 = mat[6],
            a20 = mat[8], a21 = mat[9], a22 = mat[10];

            __float b01 = a22*a11-a12*a21,
            b11 = -a22*a10+a12*a20,
            b21 = a21*a10-a11*a20;

            __float d = a00*b01 + a01*b11 + a02*b21;
            if (!d) { return dest; }
            __float id = 1/d;

            dest[0] = b01*id;
            dest[1] = (-a22*a01 + a02*a21)*id;
            dest[2] = (a12*a01 - a02*a11)*id;
            dest[3] = b11*id;
            dest[4] = (a22*a00 - a02*a20)*id;
            dest[5] = (-a12*a00 + a02*a10)*id;
            dest[6] = b21*id;
            dest[7] = (-a21*a00 + a01*a20)*id;
            dest[8] = (a11*a00 - a01*a10)*id;

            return dest;
        };
        
        static mat4 inverse(mat4 m) {
            mat4 m_inv;
            
            __float a0 = m[0] * m[5] - m[1] * m[4];
            __float a1 = m[0] * m[6] - m[2] * m[4];
            __float a2 = m[0] * m[7] - m[3] * m[4];
            __float a3 = m[1] * m[6] - m[2] * m[5];
            __float a4 = m[1] * m[7] - m[3] * m[5];
            __float a5 = m[2] * m[7] - m[3] * m[6];
            __float b0 = m[8] * m[13] - m[9] * m[12];
            __float b1 = m[8] * m[14] - m[10] * m[12];
            __float b2 = m[8] * m[15] - m[11] * m[12];
            __float b3 = m[9] * m[14] - m[10] * m[13];
            __float b4 = m[9] * m[15] - m[11] * m[13];
            __float b5 = m[10] * m[15] - m[11] * m[14];
            
            __float determinant = a0 * b5 - a1 * b4 + a2 * b3 + a3 * b2 - a4 * b1 + a5 * b0;
            
            if (determinant != 0) {
                m_inv[0] = 0 + m[5] * b5 - m[6] * b4 + m[7] * b3;
                m_inv[4] = 0 - m[4] * b5 + m[6] * b2 - m[7] * b1;
                m_inv[8] = 0 + m[4] * b4 - m[5] * b2 + m[7] * b0;
                m_inv[12] = 0 - m[4] * b3 + m[5] * b1 - m[6] * b0;
                m_inv[1] = 0 - m[1] * b5 + m[2] * b4 - m[3] * b3;
                m_inv[5] = 0 + m[0] * b5 - m[2] * b2 + m[3] * b1;
                m_inv[9] = 0 - m[0] * b4 + m[1] * b2 - m[3] * b0;
                m_inv[13] = 0 + m[0] * b3 - m[1] * b1 + m[2] * b0;
                m_inv[2] = 0 + m[13] * a5 - m[14] * a4 + m[15] * a3;
                m_inv[6] = 0 - m[12] * a5 + m[14] * a2 - m[15] * a1;
                m_inv[10] = 0 + m[12] * a4 - m[13] * a2 + m[15] * a0;
                m_inv[14] = 0 - m[12] * a3 + m[13] * a1 - m[14] * a0;
                m_inv[3] = 0 - m[9] * a5 + m[10] * a4 - m[11] * a3;
                m_inv[7] = 0 + m[8] * a5 - m[10] * a2 + m[11] * a1;
                m_inv[11] = 0 - m[8] * a4 + m[9] * a2 - m[11] * a0;
                m_inv[15] = 0 + m[8] * a3 - m[9] * a1 + m[10] * a0;
                
                __float inverse_det = 1.0f / determinant;
                
                m_inv[0] *= inverse_det;
                m_inv[1] *= inverse_det;
                m_inv[2] *= inverse_det;
                m_inv[3] *= inverse_det;
                m_inv[4] *= inverse_det;
                m_inv[5] *= inverse_det;
                m_inv[6] *= inverse_det;
                m_inv[7] *= inverse_det;
                m_inv[8] *= inverse_det;
                m_inv[9] *= inverse_det;
                m_inv[10] *= inverse_det;
                m_inv[11] *= inverse_det;
                m_inv[12] *= inverse_det;
                m_inv[13] *= inverse_det;
                m_inv[14] *= inverse_det;
                m_inv[15] *= inverse_det;
                
                return m_inv;
            }
            
            return mat4::identity();
        };
        
        static mat4 translate(__float x, __float y, __float z) {
            mat4 m = mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, x,   y,   z, 1.0f);
            
            return m;
        };
        
        static mat4 rotateAxis(__float r, __float x, __float y, __float z) {   // rotate r about axis x,y,z
            __float sAng = sinf(r*((float)M_PI/180.0f));
            __float cAng = cosf(r*((float)M_PI/180.0f));
            
            return mat4( cAng+(x*x)*(1.0f-cAng), x*y*(1.0f-cAng) - z*sAng, x*z*(1.0f-cAng) + y*sAng, 0,
                y*x*(1.0f-cAng)+z*sAng, cAng + y*y*(1.0f-cAng), y*z*(1.0f-cAng)-x*sAng, 0,
                z*x*(1.0f-cAng)-y*sAng, z*y*(1.0f-cAng)+x*sAng, cAng+(z*z)*(1.0f-cAng), 0,
                0, 0, 0, 1 );
        };
        
        static mat4 rotate(__float x, __float y, __float z) {   // rotate each axis, angles x, y, z in turn
            __float  sAng,cAng;
            mat4 mOut = mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            
            if (z!=0) {
                sAng = sinf(z*((float)M_PI/180.0f));
                cAng = cosf(z*((float)M_PI/180.0f));
                
                mOut *= mat4(cAng, sAng, 0.0f, 0.0f, -sAng, cAng, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }
            
            if (y!=0) {
                sAng = sinf(y*((float)M_PI/180.0f));
                cAng = cosf(y*((float)M_PI/180.0f));
                
                mOut *= mat4(cAng, 0.0f, -sAng, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, sAng, 0.0f, cAng, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }
            
            if (x!=0) {
                sAng = sinf(x*((float)M_PI/180.0f));
                cAng = cosf(x*((float)M_PI/180.0f));
                
                mOut *= mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, cAng, sAng, 0.0f, 0.0f, -sAng, cAng, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            }
            
            return mOut;
        };
        
        static mat4 scale(__float x, __float y, __float z) {
            return mat4(x, 0.0f, 0.0f, 0.0f, 0.0f, y, 0.0f, 0.0f, 0.0f, 0.0f, z, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        };
        
        static mat4 transform(vec3 position, vec3 rotation, vec3 scale) {
            mat4 m = mat4::identity();
            
            if (position!=NULL) {
                m *= mat4::translate(position[0],position[1],position[2]);
            }
            if (rotation!=NULL) {
                if (!(rotation[0] == 0 && rotation[1] == 0 && rotation[2] == 0)) {
                    m *= mat4::rotate(rotation[0],rotation[1],rotation[2]);
                }
            }
            if (scale!=NULL) {
                if (!(scale[0] == 1 && scale[1] == 1 && scale[2] == 1)) {
                    m *= mat4::scale(scale[0],scale[1],scale[2]);
                }
            }
            
            return m;
        };
        static vec4 vec4_multiply(vec4 m1, mat4 m2) {
            vec4 mOut;

            mOut[0] = m2[0] * m1[0] + m2[4] * m1[1] + m2[8] * m1[2] + m2[12] * m1[3];
            mOut[1] = m2[1] * m1[0] + m2[5] * m1[1] + m2[9] * m1[2] + m2[13] * m1[3];
            mOut[2] = m2[2] * m1[0] + m2[6] * m1[1] + m2[10] * m1[2] + m2[14] * m1[3];
            mOut[3] = m2[3] * m1[0] + m2[7] * m1[1] + m2[11] * m1[2] + m2[15] * m1[3];

            return mOut;
        };
        static mat4 lookat(__float eyex, __float eyey, __float eyez, __float centerx, __float centery, __float centerz, __float upx, __float upy, __float upz) {
            vec3 forward, side, up;
            
            forward[0] = centerx - eyex;
            forward[1] = centery - eyey;
            forward[2] = centerz - eyez;
            
            up[0] = upx;
            up[1] = upy;
            up[2] = upz;
            
            forward = vec3::normalize(forward);
            
            /* Side = forward x up */
            side = vec3::cross(forward, up);
            side = vec3::normalize(side);
            
            /* Recompute up as: up = side x forward */
            up = vec3::cross(side, forward);
            
            return mat4::translate(-eyex,-eyey,-eyez) * mat4( side[0], up[0], -forward[0], 0, side[1], up[1], -forward[1], 0, side[2], up[2], -forward[2], 0, 0, 0, 0, 1);
        };
        
        static vec3 unProject(mat4 pMatrix, mat4 mvMatrix, float width, float height, float winx, float winy, float /* winz */) {
            vec4 p(((winx / width) * 2.0f) - 1.0, -(((winy / height) * 2.0f) - 1.0), 1.0, 1.0);
            
            vec4 invp = mat4::vec4_multiply(mat4::vec4_multiply(p, mat4::inverse(pMatrix)), mat4::inverse(mvMatrix));
            
            vec3 result(invp[0] / invp[3], invp[1] / invp[3], invp[2] / invp[3]);
            
            return result;
        };
    };
  }


#endif /* defined(__CubicVR2__mat4__) */
