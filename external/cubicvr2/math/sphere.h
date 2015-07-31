//
//  sphere.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef CubicVR2_sphere_h
#define CubicVR2_sphere_h

#include "vec3.h"
#include "vec4.h"

namespace CubicVR {
    
    struct sphere {
        bool intersects(vec4 sphere, vec4 other) {
            vec3 spherePos(sphere[0], sphere[1], sphere[2]);
            vec3 otherPos(other[0], other[1], other[2]);
            vec3 diff = vec3::subtract(spherePos, otherPos);
            
            __float mag = sqrtf(diff[0] * diff[0] + diff[1] * diff[1] + diff[2] * diff[2]);
            __float sum_radii = sphere[3] + other[3];
            
            if (mag * mag < sum_radii * sum_radii) {
                return true;
            }
            return false;
        }
    };
}

#endif
