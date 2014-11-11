#pragma once

#include <vector>

class GradientColor {
public:
    float r,g,b;
    float w;
    
  GradientColor(float r_in, float g_in, float b_in) : r(r_in), g(g_in), b(b_in) {
      
  };
};

class Gradient {
public:
  Gradient() {
      
  }  
  
  void addColor(GradientColor c) {
      colors.push_back(c);
  }
  
  void generate(std::vector<unsigned char> *out, unsigned int len) {
      int chunk_size = len/(colors.size()-1);      

      out->resize(len*3);

      int p = 0;

      for (unsigned int j = 0, jMax = colors.size()-1; j < jMax; j++) {
          if (chunk_size*3 < len && j == jMax-1) {
              chunk_size += len-chunk_size*3;
          }

          for (unsigned int i = 0; i < chunk_size; i++) {
              float idx = (float)(i+1)/(float)chunk_size;
      
              float r1 = colors[j].r;
              float g1 = colors[j].g;
              float b1 = colors[j].b;

              float r2 = colors[j+1].r;
              float g2 = colors[j+1].g;
              float b2 = colors[j+1].b;
              
              float r = r1 + (r2-r1) * idx;
              float g = g1 + (g2-g1) * idx;
              float b = b1 + (b2-b1) * idx;
              
              (*out)[p*3] = (unsigned char)(r*255.0);
              (*out)[p*3+1] = (unsigned char)(g*255.0);
              (*out)[p*3+2] = (unsigned char)(b*255.0);
              
              p++;
          }
      }
  }
  
  ~Gradient() {
      
  }
private:
    std::vector<GradientColor> colors;
};