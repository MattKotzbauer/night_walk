#include <windows.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

int main(){
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  return 0;
}


/* 
int WINAPI WinMain(HINSTANCE Instance,
		   HINSTANCE PrevInstance,
		   PSTR CommandLine,
		   int ShowCode)
{
  OutputDebugStringA("foo");
  return 0;
}
*/
