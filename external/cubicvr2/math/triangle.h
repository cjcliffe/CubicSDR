//
//  triangle.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__triangle__
#define __CubicVR2__triangle__

#include <iostream>
#include "vec3.h"

namespace CubicVR {
    
    struct triangle {
        static vec3 normal(vec3 pt1, vec3 pt2, vec3 pt3) {
            
            __float v10 = pt1[0] - pt2[0];
            __float v11 = pt1[1] - pt2[1];
            __float v12 = pt1[2] - pt2[2];
            __float v20 = pt2[0] - pt3[0];
            __float v21 = pt2[1] - pt3[1];
            __float v22 = pt2[2] - pt3[2];
            
            vec3 mOut;
            
            mOut[0] = v11 * v22 - v12 * v21;
            mOut[1] = v12 * v20 - v10 * v22;
            mOut[2] = v10 * v21 - v11 * v20;
            
            return mOut;
        };
    };
    
}


#endif /* defined(__CubicVR2__triangle__) */
