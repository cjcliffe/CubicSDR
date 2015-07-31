//
//  frustum.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef CubicVR2_frustum_h
#define CubicVR2_frustum_h

#include <vector>
#include "mat4.h"
#include "vec3.h"
#include "vec4.h"
#include "plane.h"

namespace CubicVR {
    
    enum frustum_plane { PLANE_LEFT, PLANE_RIGHT, PLANE_TOP, PLANE_BOTTOM, PLANE_NEAR, PLANE_FAR };
    
    struct frustum {
        std::vector<vec4> planes;
        vec4 sphere;
        
        frustum() {
            planes.resize(6);
            for (int i = 0; i < 6; ++i) {
                planes[i] = vec4(0, 0, 0, 0);
            } //for
        } //Frustum::Constructor
        
        void extract(vec3 position, mat4 mvMatrix, mat4 pMatrix) {
            mat4 comboMatrix = mat4::multiply(pMatrix, mvMatrix, true);
            
            // Left clipping plane
            planes[PLANE_LEFT][0] = comboMatrix[3] + comboMatrix[0];
            planes[PLANE_LEFT][1] = comboMatrix[7] + comboMatrix[4];
            planes[PLANE_LEFT][2] = comboMatrix[11] + comboMatrix[8];
            planes[PLANE_LEFT][3] = comboMatrix[15] + comboMatrix[12];
            
            // Right clipping plane
            planes[PLANE_RIGHT][0] = comboMatrix[3] - comboMatrix[0];
            planes[PLANE_RIGHT][1] = comboMatrix[7] - comboMatrix[4];
            planes[PLANE_RIGHT][2] = comboMatrix[11] - comboMatrix[8];
            planes[PLANE_RIGHT][3] = comboMatrix[15] - comboMatrix[12];
            
            // Top clipping plane
            planes[PLANE_TOP][0] = comboMatrix[3] - comboMatrix[1];
            planes[PLANE_TOP][1] = comboMatrix[7] - comboMatrix[5];
            planes[PLANE_TOP][2] = comboMatrix[11] - comboMatrix[9];
            planes[PLANE_TOP][3] = comboMatrix[15] - comboMatrix[13];
            
            // Bottom clipping plane
            planes[PLANE_BOTTOM][0] = comboMatrix[3] + comboMatrix[1];
            planes[PLANE_BOTTOM][1] = comboMatrix[7] + comboMatrix[5];
            planes[PLANE_BOTTOM][2] = comboMatrix[11] + comboMatrix[9];
            planes[PLANE_BOTTOM][3] = comboMatrix[15] + comboMatrix[13];
            
            // Near clipping plane
            planes[PLANE_NEAR][0] = comboMatrix[3] + comboMatrix[2];
            planes[PLANE_NEAR][1] = comboMatrix[7] + comboMatrix[6];
            planes[PLANE_NEAR][2] = comboMatrix[11] + comboMatrix[10];
            planes[PLANE_NEAR][3] = comboMatrix[15] + comboMatrix[14];
            
            // Far clipping plane
            planes[PLANE_FAR][0] = comboMatrix[3] - comboMatrix[2];
            planes[PLANE_FAR][1] = comboMatrix[7] - comboMatrix[6];
            planes[PLANE_FAR][2] = comboMatrix[11] - comboMatrix[10];
            planes[PLANE_FAR][3] = comboMatrix[15] - comboMatrix[14];
            
            for (unsigned int i = 0; i < 6; ++i) {
                planes[i] = vec4::normalize(planes[i]);
            }
            
            //Sphere
            __float fov = 1 / pMatrix[5];
            __float near = -planes[PLANE_NEAR][3];
            __float far = planes[PLANE_FAR][3];
            __float view_length = far - near;
            __float height = view_length * fov;
            __float width = height;
            
            vec3 P(0, 0, near + view_length * 0.5f);
            vec3 Q(width, height, near + view_length);
            vec3 diff = vec3::subtract(P, Q);
            __float diff_mag = vec3::length(diff);
            
            vec3 look_v = vec3(comboMatrix[3], comboMatrix[9], comboMatrix[10]);
            __float look_mag = vec3::length(look_v);
            look_v = vec3::multiply(look_v, 1 / look_mag);
            
            vec3 pos = vec3(position[0], position[1], position[2]);
            pos = vec3::add(pos, vec3::multiply(look_v, view_length * 0.5f));
            pos = vec3::add(pos, vec3::multiply(look_v, 1));
            
            sphere = vec4(pos[0], pos[1], pos[2], diff_mag);
            
        }; //Frustum::extract
        
        int contains_sphere(vec4 sphere) {
            
            for (unsigned int i = 0; i < 6; ++i) {
                vec4 &p = planes[i];
                vec3 normal = vec3(p[0], p[1], p[2]);
                __float distance = vec3::dot(normal, vec3(sphere[0],sphere[1],sphere[2])) + p[3];
                
                //OUT
                if (distance < -sphere[3]) {
                    return -1;
                }
                
                //INTERSECT
                if (fabs(distance) < sphere[3]) {
                    return 0;
                }
                
            } //for
            //IN
            return 1;
        }; //Frustum::contains_sphere
        
        int contains_box(aabb bbox) {
            int total_in = 0;
            
            vec3 points[8];
            
            points[0] = bbox.min;
            points[1] = vec3(bbox.min[0], bbox.min[1], bbox.max[2]);
            points[2] = vec3(bbox.min[0], bbox.max[1], bbox.min[2]);
            points[3] = vec3(bbox.min[0], bbox.max[1], bbox.max[2]);
            points[4] = vec3(bbox.max[0], bbox.min[1], bbox.min[2]);
            points[5] = vec3(bbox.max[0], bbox.min[1], bbox.max[2]);
            points[6] = vec3(bbox.max[0], bbox.max[1], bbox.min[2]);
            points[7] = bbox.max;
            
            for (unsigned int i = 0; i < 6; ++i) {
                unsigned int in_count = 8;
                unsigned int point_in = 1;
                
                for (unsigned int j = 0; j < 8; ++j) {
                    if (plane::classifyPoint(planes[i], points[j]) == -1) {
                        point_in = 0;
                        --in_count;
                    } //if
                } //for j
                
                //OUT
                if (in_count == 0) {
                    return -1;
                }
                
                total_in += point_in;
            } //for i
            //IN
            if (total_in == 6) {
                return 1;
            }
            
            return 0;
        }; //Frustum::contains_box
        
    };
}

#endif
