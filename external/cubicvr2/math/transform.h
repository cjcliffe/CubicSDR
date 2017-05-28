//
//  Transform.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__Transform__
#define __CubicVR2__Transform__

#include <iostream>
#include "cubic_types.h"
#include "mat4.h"
#include "vec3.h"
#include <vector>

namespace CubicVR {
    
    class transform {
        std::vector<mat4> m_stack;
        std::vector<mat4> m_cache;
        int c_stack;
        int valid;
        mat4 result;
        
        transform() {
            c_stack = 0;
            valid = false;
            result = mat4::identity();
        };
        
        transform(mat4 init_mat) {
            clearStack(init_mat);
        };
        
        void setIdentity() {
            m_stack[c_stack] = mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
            if (valid == c_stack && c_stack) {
                valid--;
            }
        }
        
        mat4 getIdentity() {
            return mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
        }
        
        void invalidate() {
            valid = 0;
            result = mat4::identity();
        }
        
        mat4 getResult() {
            if (!c_stack) {
                return m_stack[0];
            }
            
            mat4 m = getIdentity();
            
            if (valid > c_stack-1) valid = c_stack-1;
            
            for (int i = valid; i < c_stack+1; i++) {
                m = mat4::multiply(m, m_stack[i], true);
                m_cache[i] = m;
            }
            
            valid = c_stack-1;
            
            result = m_cache[c_stack];
            
            return result;
        }
        
        void pushMatrix(mat4 m) {
            c_stack++;
            m_stack[c_stack] = (m ? m : getIdentity());
        }
        
        void popMatrix() {
            if (c_stack == 0) {
                return;
            }
            c_stack--;
        }
        
        void clearStack(mat4 init_mat) {
            m_stack.clear();
            m_cache.clear();
            c_stack = 0;
            valid = 0;

            result = mat4::identity();
            
            if (!init_mat) {
                m_stack[0] = init_mat;
            } else {
                setIdentity();
            }
        }
        
        void translate(__float x, __float y, __float z) {
            mat4 m = getIdentity();
            
            m[12] = x;
            m[13] = y;
            m[14] = z;
            
            m_stack[c_stack] = mat4::multiply(m, m_stack[c_stack], true);
            if (valid == c_stack && c_stack) {
                valid--;
            }
        }
        
        void scale(__float x, __float y, __float z) {
            mat4 m = getIdentity();
            
            m[0] = x;
            m[5] = y;
            m[10] = z;
            
            m_stack[c_stack] = mat4::multiply(m, m_stack[c_stack], true);
            if (valid == c_stack && c_stack) {
                valid--;
            }
        }
        
        void rotate(__float ang, __float x, __float y, __float z) {
            __float sAng, cAng;
            
            if (x || y || z) {
                sAng = sin(-ang * ((float)M_PI / 180.0f));
                cAng = cos(-ang * ((float)M_PI / 180.0f));
            }
            
            if (x) {
                mat4 X_ROT = getIdentity();
                
                X_ROT[5] = cAng * x;
                X_ROT[9] = sAng * x;
                X_ROT[6] = -sAng * x;
                X_ROT[10] = cAng * x;
                
                m_stack[c_stack] = mat4::multiply(m_stack[c_stack], X_ROT, true);
            }
            
            if (y) {
                mat4 Y_ROT = getIdentity();
                
                Y_ROT[0] = cAng * y;
                Y_ROT[8] = -sAng * y;
                Y_ROT[2] = sAng * y;
                Y_ROT[10] = cAng * y;
                
                m_stack[c_stack] = mat4::multiply(m_stack[c_stack], Y_ROT, true);
            }
            
            if (z) {
                mat4 Z_ROT = getIdentity();
                
                Z_ROT[0] = cAng * z;
                Z_ROT[4] = sAng * z;
                Z_ROT[1] = -sAng * z;
                Z_ROT[5] = cAng * z;
                
                m_stack[c_stack] = mat4::multiply(m_stack[c_stack], Z_ROT, true);
            }
            
            if (valid == c_stack && c_stack) {
                valid--;
            }
        };
    };
}

#endif /* defined(__CubicVR2__Transform__) */
