//
//  plane.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef CubicVR2_plane_h
#define CubicVR2_plane_h

#include "vec4.h"
#include "vec3.h"

namespace CubicVR {
    
    struct plane : vec4 {
        static int classifyPoint(vec4 plane, vec3 pt) {
            __float dist = (plane[0] * pt[0]) + (plane[1] * pt[1]) + (plane[2] * pt[2]) + (plane[3]);
            if (dist < 0) {
                return -1;
            }
            else if (dist > 0) {
                return 1;
            }
            return 0;
        };
    };
    
}

#endif
