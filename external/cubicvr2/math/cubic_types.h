//
//  types.h
//  CubicVR2
//
//  Created by Charles J. Cliffe on 2013-02-21.
//  Copyright (c) 2013 Charles J. Cliffe. All rights reserved.
//

#ifndef CubicVR2_types_h
#define CubicVR2_types_h

namespace CubicVR {
    
    typedef double __float64;
    typedef float __float32;
    
    typedef __float32 __float;
    
    #define COMBINE(x,y) x ## y
    #define floatSG(c, x,y) \
        __float COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(__float value) { y = value; return *this; }
    #define intSG(c, x,y) \
        int COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(int value) { y = value; return *this; }
    #define uintSG(c, x,y) \
        unsigned int COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(unsigned int value) { y = value; return *this; }
    #define boolSG(c,x,y) \
        bool COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(bool value) { y = value; return *this; }
    #define stringSG(c,x,y) \
        string COMBINE(get,x)() { return y; } \
        c & COMBINE(set,x)(string value) { y = value; return *this; }
    #define isBoolSG(c,x,y) \
        bool COMBINE(is,x)() { return y; } \
        c & COMBINE(set,x)(bool value) { y = value; return *this; }

}

#endif
