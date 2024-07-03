#include <tfe/TFEditor.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION 1
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

int main() {
  using namespace tfe;
  TFEditor editor;

  editor.setBackground(std::make_shared<Checkers>(16,vec3f(0.f),vec3f(1.f)));

  vec2f CPs[] =  {
    {0.0f,1.0f},
    {0.3f,0.8f},
    {1.0f,1.0f}, };
  auto pl = std::make_shared<PiecewiseLinear>(CPs,3);
  editor.addFunction(pl);

  auto tn = std::make_shared<Tent>();
  editor.addFunction(tn);

  Texture tex = editor.rasterize(256, 128);
  stbi_write_png("simple.png",tex.width,tex.height,4,tex.data.data(),tex.width*4);
}




