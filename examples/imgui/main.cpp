#define TFE_ENABLE_IMGUI 1
#define TFE_INCLUDE_GLAD_HEADER 1
#include <tfe/TFEditor.h>

#include <GLFW/glfw3.h>

#include "imgui_impl_glfw_gl3.h"
#include <imgui.h>

using namespace tfe;
int main() {

  if (!glfwInit()) {
    std::cerr << "Failed to initialize GLFW!\n";
    return -1;
  }

  GLFWwindow *glfwWindow = glfwCreateWindow(
        512, 512, "TFE Imgui Example", NULL, NULL);

  if (!glfwWindow) {
    glfwTerminate();
    std::cerr << "Failed to create GLFW window!\n";
    return -1;
  }

  glfwMakeContextCurrent(glfwWindow);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    std::cerr << "Failed to initialize GLAD loader\n";
    return -1;
  }

  ImGui_ImplGlfwGL3_Init(glfwWindow, true);

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  GLuint framebufferTexture{0};
  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  static bool quitNextFrame = false;

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        glViewport(0, 0, newWidth, newHeight);
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    //ImGuiIO &io = ImGui::GetIO();
    //if (!io.WantCaptureMouse) {
      //activeWindow->motion(int2{int(x), int(y)});
    //}
  });

  glfwSetKeyCallback(
      glfwWindow, [](GLFWwindow *, int key, int, int action, int) {
        if (action == GLFW_PRESS) {
          switch (key) {
          case GLFW_KEY_Q:
            quitNextFrame = true;
            break;
          }
        }
      });

  // TF editor (imgui flavor):
  TFEditorImGui editor;
  editor.setBackground(std::make_shared<Checkers>(16,vec3f(0.8f),vec3f(1.f)));

  auto tn = std::make_shared<Tent>();
  editor.addFunction(tn);

  // main loop:
  while (true) {
    ImGui_ImplGlfwGL3_NewFrame();
    ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
    ImGui::Begin(
        "Transfer Function Editor", nullptr, flags);
    editor.draw(256, 128);
    ImGui::End();

    // clear current OpenGL color buffer
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui::Render();
    ImGui_ImplGlfwGL3_Render();

    // swap buffers
    glfwSwapBuffers(glfwWindow);

    glfwPollEvents();
    if (glfwWindowShouldClose(glfwWindow) || quitNextFrame) {
      break;
    }
  }

  ImGui_ImplGlfwGL3_Shutdown();
  glfwTerminate();
}




