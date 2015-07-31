//
//  math.cpp
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-22.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#include "cubic_math.h"

namespace CubicVR {
    std::ostream& operator<<(std::ostream &strm, const vec4 &v) {
        return strm << "{" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << "}";
    }
    std::ostream& operator<<(std::ostream &strm, const vec3 &v) {
        return strm << "{" << v.x << ", " << v.y << ", " << v.z << "}";
    }
    std::ostream& operator<<(std::ostream &strm, const vec2 &v) {
        return strm << "{" << v.x << ", " << v.y << "}";
    }
    std::ostream& operator<<(std::ostream &strm, const mat4 &m) {
        return strm << "{ " << m[0] << ", " << m[1] << ", " << m[2] << ", " << m[3] << endl
        << "  " << m[4] << ", " << m[5] << ", " << m[6] << ", " << m[7] << endl
        << "  " << m[8] << ", " << m[9] << ", " << m[10] << ", " << m[11] << endl
        << "  " << m[12] << ", " << m[13] << ", " << m[14] << ", " << m[15] << " }" << endl;
    }
}
