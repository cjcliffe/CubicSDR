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

  std::vector<float> &getRed() {
      return r_val;
  }

  std::vector<float> &getGreen() {
      return g_val;
  }

  std::vector<float> &getBlue() {
      return b_val;
  }

  void generate(unsigned int len) {
      int chunk_size = len/(colors.size()-1);      

      int p = 0;
      r_val.resize(len);
      g_val.resize(len);
      b_val.resize(len);

      for (unsigned int j = 0, jMax = colors.size()-1; j < jMax; j++) {
          if (chunk_size*(jMax+1) < len && j == jMax-1) {
              chunk_size += len-chunk_size*(jMax+1);
          }

          for (unsigned int i = 0; i < chunk_size; i++) {
              float idx = (float)(i)/(float)chunk_size;
      
              float r1 = colors[j].r;
              float g1 = colors[j].g;
              float b1 = colors[j].b;

              float r2 = colors[j+1].r;
              float g2 = colors[j+1].g;
              float b2 = colors[j+1].b;
              
              float r = r1 + (r2-r1) * idx;
              float g = g1 + (g2-g1) * idx;
              float b = b1 + (b2-b1) * idx;
              
              if (r<0.0) r = 0.0;
              if (r>1.0) r = 1.0;
              if (g<0.0) g = 0.0;
              if (g>1.0) g = 1.0;
              if (b<0.0) b = 0.0;
              if (b>1.0) b = 1.0;

              r_val[p] = r;
              g_val[p] = g;
              b_val[p] = b;
              
              p++;
          }
      }
  }
  
  ~Gradient() {
      
  }
private:
    std::vector<GradientColor> colors;
    std::vector<float> r_val;
    std::vector<float> g_val;
    std::vector<float> b_val;
};
