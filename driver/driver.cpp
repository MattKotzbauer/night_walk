// INCLUDES / GLOBALS START
#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define global static
#define internal static

typedef int32_t int32;
typedef int64_t int64;
typedef float real32;
typedef double real64;

// INCLUDES / GLOBALS END

// HELPERS START
internal void ResizeWindow(GLFWwindow* Window, int32 Width, int32 Height){
  glViewport(0, 0, Width, Height);
}

internal void ProcessInput(GLFWwindow* Window){
  if(glfwGetKey(Window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
    glfwSetWindowShouldClose(Window, true); 
  }
}
// HELPERS END

// MAIN START
int main(){
  glfwInit();

  // (Specify OpenGL 3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // (Specify OpenGL 3.3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // (Specify OpenGL Core)
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // (Create window + OpenGL context)
  GLFWwindow* Window = glfwCreateWindow(800, 600, "BaseWindow", NULL, NULL);
  if(!Window){
    OutputDebugStringA("Window creation failed");
    glfwTerminate();
    return -1;
  }

  // (Makes window's current context - required before OpenGL function calls)
  glfwMakeContextCurrent(Window);
  // (Sets ResizeWindow as window resize callback fn) 
  glfwSetFramebufferSizeCallback(Window, ResizeWindow);

  // (Initialize GLAD: loads OpenGL function pointers)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
      OutputDebugStringA("Failed to initialize GLAD");
      return -1;
    }     

  // (Sets OpenGL viewport dimensions)
  glViewport(0, 0, 800, 600);

  // Main render loop: 
  while (!glfwWindowShouldClose(Window)){
    
    ProcessInput(Window);

    // (Sets buffer background color: RGBA (current: teal))
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // (Clears buffer)
    glClear(GL_COLOR_BUFFER_BIT);

    // (Swap front and back buffers)
    glfwSwapBuffers(Window);
    // (Check for pending events (keyboard, mouse, window)
    glfwPollEvents();
    

    // (Main render loop end)
  }
 
  return 0;
}
// MAIN END

