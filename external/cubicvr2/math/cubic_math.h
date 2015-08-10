//
//  math.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef __CubicVR2__math__
#define __CubicVR2__math__

#include <iostream>

#include "aabb.h"
#include "mat3.h"
#include "mat4.h"
#include "quaternion.h"
#include "transform.h"
#include "triangle.h"
#include "vec2.h"
#include "vec3.h"
#include "vec4.h"
#include "plane.h"
#include "sphere.h"
#include "frustum.h"

namespace CubicVR {
    std::ostream& operator<<(std::ostream &strm, const vec4 &v);
    std::ostream& operator<<(std::ostream &strm, const vec3 &v);
    std::ostream& operator<<(std::ostream &strm, const vec2 &v);
    std::ostream& operator<<(std::ostream &strm, const mat4 &m);
}


#endif /* defined(__CubicVR2__math__) */
