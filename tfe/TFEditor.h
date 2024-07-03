#pragma once

/*! @file
  @brief Header only transfer fucntion editor

  The following variables can be defined *before* including this file:

  TFE_ENABLE_OPENGL (default: undefined)
    if defined, the class TFEditorOpenGL becomes available; we assume that
    OpenGL 3.1 is available, e.g. via glad. The user can optionally
    enable and compiled against the glad header shipping with the library
    and define TFE_INCLUDE_GLAD_HEADER

  TFE_ENABLE_IMGUI (default: undefined)
    if defined, the class TFEditorImGui becomes available and the
    imgui.h header included; use the headers and source files contained
    in imgui to compile with your projects; this option automatically
    enables the option TFE_ENABLE_OPENGL

  TFE_INCLUDE_GLAD_HEADER (default: undefined)
    instruct TFE to include the glad header which will bring in OpenGL
    functions; the headers ship with this project and are found in the
    folders glad and KHR; the file glad.c must be compiled along with
    the project
 */

#ifdef TFE_ENABLE_IMGUI
#ifndef TFE_ENABLE_OPENGL
#define TFE_ENABLE_OPENGL 1
#endif
#endif

// std
#include <cmath>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
//ours
#include "math.h"

// GLAD
#ifdef TFE_INCLUDE_GLAD_HEADER
#include "glad/glad.h"
#endif

// ImGui
#ifdef TFE_ENABLE_IMGUI
#include "imgui.h"
#endif

namespace tfe {

  // import math namespace into namespace tfe!
  using namespace math;

  // color conversion functions:
  inline uint32_t cvt_uint32(const float &f)
  {
    return static_cast<uint32_t>(255.f * clamp(f, 0.f, 1.f));
  }
  
  inline uint32_t cvt_uint32(const vec4f &v)
  {
    return (cvt_uint32(v.x) << 0) | (cvt_uint32(v.y) << 8)
        | (cvt_uint32(v.z) << 16) | (cvt_uint32(v.w) << 24);
  }

  inline float cvt_float32(uint32_t u)
  {
    return u / 255.0f;
  }

  inline vec4f cvt_rgba32f(uint32_t u)
  {
    return vec4f(cvt_float32(u & 0xff),
                 cvt_float32((u >> 8) & 0xff),
                 cvt_float32((u >> 16) & 0xff),
                 cvt_float32((u >> 24) & 0xff));
  }

  inline vec4f over(const vec4f &A, const vec4f &B)
  {
    return A + (1.f-A.w)*B;
  }

  /*! texture class used by the functions, and by the TFEditor
    that over-composites the textures of all functions */
  struct Texture
  {
    Texture() : width(0), height(0) {}
    Texture(unsigned w, unsigned h) : width(w), height(h), data(w*size_t(h),0u) {}
    unsigned width, height;
    std::vector<uint32_t> data;

    size_t linearIndex(unsigned x, unsigned y) const
    { return x+size_t(width)*y; }

    unsigned flip(unsigned y) const
    { return height-y-1; }

    void set(unsigned x, unsigned y, uint32_t val)
    { data[linearIndex(x,flip(y))] = val; }

    uint32_t get(unsigned x, unsigned y) const
    { return data[linearIndex(x,flip(y))]; }
  };

  /*! Layer, can be drawn on top of each other */
  struct Layer
  {
    typedef std::shared_ptr<Layer> SP;
    virtual Texture rasterize(unsigned width, unsigned height) const = 0;
  };
  
  /*! 1D alpha function ayer, defined over a valueRange in X, and can be
    evaluated at position x; alpha is in [0:1] */
  struct Function : public Layer
  {
    typedef std::shared_ptr<Function> SP;
    box1f valueRange{0.f, 1.f};

    virtual ~Function() {}

    virtual float eval(float x) const = 0;

    Texture rasterize(unsigned width, unsigned height) const
    {
      Texture tex(width, height);
      for (unsigned x=0; x<width; ++x) {
        float yf = eval(x/float(width-1));
        unsigned yval = yf * height;
        for (unsigned y=0; y<yval; ++y) {
          tex.set(x, y, cvt_uint32(vec4f(0.6f, 0.6f, 0.6f, 0.95f)));
        }
      }
      return tex;
    }
  };

  class PiecewiseLinear : public Function
  {
   public:
    PiecewiseLinear() : controlPoints(2)
    {
      controlPoints[0] = 0.f;
      controlPoints[1] = 1.f;
      std::sort(controlPoints.begin(), controlPoints.end(),
        [](vec2f a, vec2f b) { return a.x<b.x; });
    }

    PiecewiseLinear(const vec2f *CPs, unsigned numCPs) : controlPoints(numCPs)
    {
      std::copy(CPs, CPs+numCPs, controlPoints.begin());
      std::sort(controlPoints.begin(), controlPoints.end(),
        [](vec2f a, vec2f b) { return a.x<b.x; });
    }

    float eval(float x) const
    {
      if (controlPoints.size() < 2 || x < valueRange.lower || x > valueRange.upper)
        return 0.f;

      for (size_t i=0; i<controlPoints.size()-1; ++i) {
        vec2f p1 = controlPoints[i];
        vec2f p2 = controlPoints[i+1];
        if (p1.x > x || p2.x < x)
          continue;


        float m = (p2.y-p1.y)/(p2.x-p1.x);
        float xf = x - p1.x;
        return p1.y + m * xf;
      }

      return 0.f;
    }

   private:
    std::vector<vec2f> controlPoints;
  };

  class Tent : public Function
  {
   public:
    Tent() : tipPos({0.5f,1.f}), topWidth(0.f), bottomWidth(1.f)
    { initInternal(); }

    Tent(vec2f tip, float tw, float bw) : tipPos(tip), topWidth(tw), bottomWidth(bw)
    { initInternal(); }

    float eval(float x) const
    {
      return internal.eval(x);
    }

   private:
    void initInternal()
    {
      std::vector<vec2f> CPs(4);
      CPs[0]=vec2f(tipPos.x-bottomWidth/2.f,0.f);
      CPs[1]=vec2f(tipPos.x-topWidth/2.0f,tipPos.y);
      CPs[2]=vec2f(tipPos.x+topWidth/2.0f,tipPos.y);
      CPs[3]=vec2f(tipPos.x+bottomWidth/2.f,0.f);
      internal = PiecewiseLinear(CPs.data(), CPs.size());
      internal.valueRange = valueRange;
    }

    vec2f tipPos;
    float topWidth, bottomWidth;
    PiecewiseLinear internal;
  };

  class Box : public Function
  {
  };

  class Gaussian : public Function
  {
  };

  class ColorMap : public Function
  {
  };

  // Checkers texture (use as background!)
  class Checkers : public Layer
  {
   public:
    Checkers(unsigned cs = 8, vec3f c1 = {0.f, 0.f, 0.f}, vec3f c2 = {1.f, 1.f, 1.f})
        : checkerSize(cs), color1(c1), color2(c2)
    {}

    Texture rasterize(unsigned width, unsigned height) const
    {
      Texture tex(width, height);
      vec4f colors[2] = {
        {color1.x,color1.y,color1.z,1.f},
        {color2.x,color2.y,color2.z,1.f},
      };
      for (unsigned y=0; y<height; ++y) {
        for (unsigned x=0; x<width; ++x) {
          unsigned xx = x/checkerSize;
          unsigned yy = y/checkerSize;
          int idx = (xx % 2) == (yy % 2) ? 0 : 1;
          tex.set(x,y,cvt_uint32(colors[idx]));
        }
      }
      return tex;
    }

   private:
    unsigned checkerSize;
    vec3f color1, color2;
  };

  class TFEditor
  {
   public:
    virtual void addFunction(const Function::SP &func)
    {
      functions.push_back(func);
    }

    virtual void setBackground(const Layer::SP &bg)
    {
      background = bg;
    }

    /*! search through the function list; if the function is
      present, make sure it is drawn on top of all the others */
    virtual void moveToTop(const Function::SP &func)
    {
      // TODO...!
    }

    /*! return the function underneath the (e.g., mouse) pos
      that is topmost on the function stack */
    Function::SP select(vec2f pos) const
    {
      for (ptrdiff_t i=functions.size()-1; i>=0; --i) {
        float y = functions[i]->eval(pos.x);
        if (pos.y < y) return functions[i];
      }
      return nullptr;
    }

    Texture rasterize(unsigned width, unsigned height) const
    {
      Texture tex(width, height);

      for (size_t i=0; i<functions.size(); ++i) {
        Texture layer = functions[i]->rasterize(width, height);
        layerOver(layer,tex);
      }

      if (background) {
        tex = layerOver(background->rasterize(width, height), tex);
      }

      if (showOutline) {
        for (unsigned x=0; x<width; ++x) {
          float xf = x/float(width-1);
          float yf = eval(xf);
          if (yf > 0.f) {
            unsigned y = yf * height;
            tex.set(x,y,cvt_uint32(vec4f(1.f,0.5f,0.f,1.f)));
          }
        }
      }

      return tex;
    }

    vec3f *getRGB(unsigned numSamples) const
    {
    }

    float *getAlpha(unsigned numSamples) const
    {
    }

    float eval(float x) const
    {
      float res = 0.f;
      for (size_t i=0; i<functions.size(); ++i) {
        float y = functions[i]->eval(x);
        res = fmaxf(res,y);
      }
      return res;
    }

   private:
    Texture& layerOver(const Texture &A, Texture &B) const
    {
      for (size_t y=0; y<A.height; ++y) {
        for (size_t x=0; x<A.width; ++x) {
          vec4f dst = cvt_rgba32f(A.get(x,y));
          vec4f src = cvt_rgba32f(B.get(x,y));
          dst = over(src,dst);
          B.set(x,y,cvt_uint32(dst));
        }
      }
      return B;
    }

    // Constant background; always the bottom layer
    Layer::SP background{nullptr};

    // Variable transfer functions layered on top of each other
    std::vector<Function::SP> functions;

    // Render outline of the convoluted alpha functions
    bool showOutline{true};
  };

#ifdef TFE_ENABLE_OPENGL
  class TFEditorOpenGL : public  TFEditor
  {
   public:
    virtual void addFunction(const Function::SP &func)
    { updated = true; TFEditor::addFunction(func); }

    virtual void setBackground(const Layer::SP &bg)
    { updated = true; TFEditor::setBackground(bg); }

    /*! search through the function list; if the function is
      present, make sure it is drawn on top of all the others */
    virtual void moveToTop(const Function::SP &func)
    { updated = true; TFEditor::moveToTop(func); }

   protected:
    // renders the alpha functions and background
    void setupTFETexture(unsigned width, unsigned height)
    {
      if (tfeTexture == 0)
        glGenTextures(1, &tfeTexture);

      GLint prevTexture;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);
      glBindTexture(GL_TEXTURE_2D, tfeTexture);

      Texture tex = TFEditor::rasterize(width, height);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          tex.width,
          tex.height,
          0,
          GL_RGBA,
          GL_UNSIGNED_BYTE,
          tex.data.data());

      // restore
      glBindTexture(GL_TEXTURE_2D, prevTexture);
    }

    // renders the TFE texture plus UI elements
    void setupTexture(unsigned width, unsigned height)
    {
      if (!updated && width == prevWidth && height == prevHeight)
        return;

      // setup framebuffer and renderbuffer

      if (framebuffer == 0)
        glGenFramebuffers(1, &framebuffer);

      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

      if (texture == 0)
        glGenTextures(1, &texture);

      GLint prevTexture;
      glGetIntegerv(GL_TEXTURE_BINDING_2D, &prevTexture);
      glBindTexture(GL_TEXTURE_2D, texture);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      glTexImage2D(GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          width,
          height,
          0,
          GL_RGBA, 
          GL_UNSIGNED_BYTE,
          0);

      if (depthbuffer == 0)
        glGenRenderbuffers(1, &depthbuffer);

      GLint prevDepthbuffer;
      glGetIntegerv(GL_RENDERBUFFER_BINDING, &prevDepthbuffer);
      glBindRenderbuffer(GL_RENDERBUFFER, depthbuffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, width, height);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
          GL_DEPTH_ATTACHMENT,
          GL_RENDERBUFFER,
          depthbuffer);

#ifdef __APPLE__
      glFramebufferTexture2D(GL_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          GL_TEXTURE_2D,
          texture,
          0);
#else
      glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, texture, 0);
#endif

      GLenum drawBuffers[1] = {GL_COLOR_ATTACHMENT0};
      glDrawBuffers(1, drawBuffers);
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        return;

      // render to framebuffer texture:
      glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    #if 1
      glViewport(0, 0, width, height);

      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      glOrtho(0.0, width, 0.0, height, -1.0, 1.0);

      unsigned margin=8;
      // resolution of the background texture;
      // can be different from widget's size
      unsigned resX = width-2*margin;
      unsigned resY = height-2*margin;

      setupTFETexture(resX, resY);
      glBindTexture(GL_TEXTURE_2D, tfeTexture);
      glBegin(GL_QUADS);

      glTexCoord2f(0.f, 0.f);
      glVertex2f(margin, margin);

      glTexCoord2f(0.f, 1.f);
      glVertex2f(margin, height-margin);

      glTexCoord2f(1.f, 1.f);
      glVertex2f(width-margin, height-margin);

      glTexCoord2f(1.f, 0.f);
      glVertex2f(width-margin, margin);

      glEnd();
    #endif

      // restore
      glBindTexture(GL_TEXTURE_2D, prevTexture);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      prevWidth = width;
      prevHeight = height;
    }

    bool updated{true};
    unsigned prevWidth = 0, prevHeight = 0;
    // texture containing functions + ui elements
    GLuint texture{0};
   private:
    // texture that the functions are rastered into
    GLuint tfeTexture{~0u};
    // framebuffer for render-to-texture
    GLuint framebuffer{0};
    GLuint depthbuffer{0};
  };
#endif

#ifdef TFE_ENABLE_IMGUI
  class TFEditorImGui : public  TFEditorOpenGL
  {
   public:
    void draw(unsigned width, unsigned height)
    {
      setupTexture(width, height);

      ImGui::GetWindowDrawList()->AddCallback(
        [](const ImDrawList *, const ImDrawCmd *)
        { glDisable(GL_BLEND); }, nullptr);

      ImGui::ImageButton(
          (void*)(intptr_t)texture,
          ImVec2(width, height),
          ImVec2(0, 0),
          ImVec2(1, 1),
          0 // frame size = 0
          );

      ImGui::GetWindowDrawList()->AddCallback(
        [](const ImDrawList *, const ImDrawCmd *)
        { glEnable(GL_BLEND); }, nullptr);

    }
  };
#endif
} // tfe
