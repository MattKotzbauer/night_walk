// INCLUDES / GLOBALS START
#include <windows.h>
#include <stdint.h> // (included by default with GLAD)
#include <math.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define global static
#define internal static

typedef uint32_t uint32;
typedef uint64_t uint64;
typedef int32_t int32;
typedef int64_t int64;
typedef float real32;
typedef double real64;

#include "shader.h"

global int32 GlobalWidth = 800;
global int32 GlobalHeight = 600;

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

internal GLFWwindow* GLFWFullInit(){
  // GLFW: Initialize and configure
  // -------------------------
  glfwInit();
  // (Specify OpenGL 3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  // (Specify OpenGL 3.3)
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  // (Specify OpenGL Core)
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  
 
  // GLFW Window Creation
  // -------------------------
  GLFWwindow* Window = glfwCreateWindow(GlobalWidth, GlobalHeight, "BaseWindow", NULL, NULL);
  if(!Window){
    OutputDebugStringA("Window creation failed");
    glfwTerminate();
    return 0;
  }
  // (Makes window's current context - required before OpenGL function calls)
  glfwMakeContextCurrent(Window);
  // (Sets ResizeWindow as window resize callback fn) 
  glfwSetFramebufferSizeCallback(Window, ResizeWindow);
    
  // (Init. GLAD: loads OpenGL function pointers)
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
      OutputDebugStringA("Failed to initialize GLAD");
      return 0;
    }     
  
  return Window;
  
}

// HELPERS END

// MAIN START
int main(){

  // GLFW + Window + Shader Init.
  // -------------------------
  // GLFW init + Window declaration
  GLFWwindow* Window = GLFWFullInit();
  if(!Window){return -1;}
  // Shader loading
  // Shader ShaderProgram("shader.vert", "shader.frag");
  Shader ShaderProgram("../driver/shader.vert", "../driver/shader.frag");
  
  // Setting Up Vertex Data 
  // -------------------------
  // (Init. triangle to render)
  real32 TriangleVertices[] = {
  // positions         // colors
    0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
    -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
    0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
  };
  
  // (Init. ID's for vertex array object, vertex buffer object)
  uint32 VAO, VBO;
  // (Create buffer object and place ID in VBO)
  glGenVertexArrays(1, &VAO);
  glGenBuffers(1, &VBO);

  // (Bind Vertex Objects)
  glBindVertexArray(VAO);
  glBindBuffer(GL_ARRAY_BUFFER, VBO);
  // (Populate currently-bound vertex buffer object with content of TriangleVertices)
  glBufferData(GL_ARRAY_BUFFER, sizeof(TriangleVertices), TriangleVertices, GL_STATIC_DRAW);
  // (Position attribute)
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(real32), (void*)0);
  glEnableVertexAttribArray(0);  
  // (Color attribute)
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(real32), (void*)(3 * sizeof(real32)));
  glEnableVertexAttribArray(1);
  
  
  
  // Main Render Loop
  // -------------------------
  while (!glfwWindowShouldClose(Window)){
    
    ProcessInput(Window);

    // (Sets buffer background color: RGBA (current: teal))
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    // (Clears buffer)
    glClear(GL_COLOR_BUFFER_BIT);

    // Draw Triangle
    ShaderProgram.Use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    // (Swap front and back buffers)
    glfwSwapBuffers(Window);
    // (Check for pending IO events (keyboard, mouse, window))
    glfwPollEvents();
    

    // (Main Render Loop End)
  }

  
  // TODO: Debug Cleanup Impact on Performance, Potentially Delete
  // Cleanup
  // -------------------------
  glDeleteVertexArrays(1, &VAO);
  glDeleteBuffers(1, &VBO);
  glfwTerminate();

  return 0;
}
// MAIN END

