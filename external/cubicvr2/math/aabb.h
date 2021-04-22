//
//  aabb.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__aabb__
#define __CubicVR2__aabb__

#include <iostream>
#include "vec3.h"

namespace CubicVR {

    enum class aabb_intersect { AABB_DISJOINT, AABB_A_INSIDE_B, AABB_B_INSIDE_A, AABB_INTERSECT };

    struct aabb {
        vec3 min, max;
        
        aabb(vec3 min_in, vec3 max_in) {
            min=min_in;
            max=max_in;
        }

        aabb() {
            min=max=vec3(0,0,0);
        }

        aabb engulf(aabb aabb, vec3 point) {
            if (aabb.min[0] > point[0]) {
                aabb.min[0] = point[0];
            }
            if (aabb.min[1] > point[1]) {
                aabb.min[1] = point[1];
            }
            if (aabb.min[2] > point[2]) {
                aabb.min[2] = point[2];
            }
            if (aabb.max[0] < point[0]) {
                aabb.max[0] = point[0];
            }
            if (aabb.max[1] < point[1]) {
                aabb.max[1] = point[1];
            }
            if (aabb.max[2] < point[2]) {
                aabb.max[2] = point[2];
            }
            return aabb;
        };
        
        static aabb reset(aabb aabb, vec3 point=vec3(0.0f,0.0f,0.0f)) {
            
            aabb.min[0] = point[0];
            aabb.min[1] = point[1];
            aabb.min[2] = point[2];
            aabb.max[0] = point[0];
            aabb.max[1] = point[1];
            aabb.max[2] = point[2];
            
            return aabb;
        };
        
        static vec3 size(aabb aabb) {
            __float x = aabb.min[0] < aabb.max[0] ? aabb.max[0] - aabb.min[0] : aabb.min[0] - aabb.max[0];
            __float y = aabb.min[1] < aabb.max[1] ? aabb.max[1] - aabb.min[1] : aabb.min[1] - aabb.max[1];
            __float z = aabb.min[2] < aabb.max[2] ? aabb.max[2] - aabb.min[2] : aabb.min[2] - aabb.max[2];
            return vec3(x,y,z);
        };
        /**
         Returns positive integer if intersect between A and B, 0 otherwise.
         For more detailed intersect result check value:
         CubicVR.enums.aabb.INTERSECT if AABBs intersect
         CubicVR.enums.aabb.A_INSIDE_B if boxA is inside boxB
         CubicVR.enums.aabb.B_INSIDE_A if boxB is inside boxA
         CubicVR.enums.aabb.DISJOINT if AABBs are disjoint (do not intersect)
         */
        aabb_intersect intersects(aabb boxA, aabb boxB) {
            // Disjoint
            if( boxA.min[0] > boxB.max[0] || boxA.max[0] < boxB.min[0] ){
                return aabb_intersect::AABB_DISJOINT;
            }
            if( boxA.min[1] > boxB.max[1] || boxA.max[1] < boxB.min[1] ){
                return aabb_intersect::AABB_DISJOINT;
            }
            if( boxA.min[2] > boxB.max[2] || boxA.max[2] < boxB.min[2] ){
                return aabb_intersect::AABB_DISJOINT;
            }
            
            // boxA is inside boxB.
            if( boxA.min[0] >= boxB.min[0] && boxA.max[0] <= boxB.max[0] &&
               boxA.min[1] >= boxB.min[1] && boxA.max[1] <= boxB.max[1] &&
               boxA.min[2] >= boxB.min[2] && boxA.max[2] <= boxB.max[2]) {
                return aabb_intersect::AABB_A_INSIDE_B;
            }
            // boxB is inside boxA.
            if( boxB.min[0] >= boxA.min[0] && boxB.max[0] <= boxA.max[0] &&
               boxB.min[1] >= boxA.min[1] && boxB.max[1] <= boxA.max[1] &&
               boxB.min[2] >= boxA.min[2] && boxB.max[2] <= boxA.max[2]) {
                return aabb_intersect::AABB_B_INSIDE_A;
            }
            
            // Otherwise AABB's intersect.
            return aabb_intersect::AABB_INTERSECT;
        }
    };
};

#endif /* defined(__CubicVR2__aabb__) */
