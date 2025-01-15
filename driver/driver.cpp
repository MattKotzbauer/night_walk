// INCLUDES / GLOBALS START
#include <windows.h>
// #include <gl.h>
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

typedef int32_t bool32;

#include "shader.h"

struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

struct game_offscreen_buffer
{
  int Width;
  int Height;
  int Pitch;
  void *Memory;
}; 

struct game_button_state {
  // int HalfTransitionCount;
  bool EndedDown;
};

struct game_input {
  union{
    game_button_state Buttons[6];
    struct{
      game_button_state Up;
      game_button_state Down;
      game_button_state Left;
      game_button_state Right;
      game_button_state LeftShoulder;
      game_button_state RightShoulder;
    };
  };
  
};


internal void Win32InitOpenGL(HWND Window){

  HDC WindowDC = GetDC(Window);
  
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW;
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
 
  int32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);

  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
  
  SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
  
  HGLRC OpenGLRC = wglCreateContext(WindowDC);
  if(wglMakeCurrent(WindowDC, OpenGLRC)){
    // Success!
  }
  else{OutputDebugStringA("ERROR::OpenGL Initialization");}
  ReleaseDC(Window, WindowDC);
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win32_offscreen_buffer *Buffer){
  glViewport(0, 0, WindowWidth, WindowHeight);

  glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Triangles: 
  // (Lower Triangle)
  glBegin(GL_TRIANGLES);
  
  glVertex2i(0, 0);
  glVertex2i(WindowWidth, 0);
  glVertex2i(WindowWidth, WindowHeight);

  // (Upper Triangle)
0  
  glVertex2i(0, 0);
  glVertex2i(WindowWidth, WindowHeight);
  glVertex2i(0, WindowHeight);

  glEnd();

  SwapBuffers(DeviceContext);

}

/*

TODO: Implement:
* Win32ResizeDIBSection
* game_input struct (means of storing game input)
* game_offscreen_buffer struct (means of storing )
* (game memory handling)


 */

/* Driver Function */
int WINAPI WinMain(HINSTANCE Instance,
		   HINSTANCE PrevInstance,
		   PSTR CommandLine,
		   int ShowCode)
{ 
  WNDCLASSA WindowClass = {};

  Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
  
  WindowClass.style = CS_HREDRAW|CS_VREDRAW;
  WindowClass.lpfnWndProc = Win64MainWindowCallback;
  WindowClass.hInstance = Instance; 
  WindowClass.lpszClassName = "NightWalkWindowClass"; 

  LARGE_INTEGER PerfCountFrequencyResult;
  QueryPerformanceFrequency(&PerfCountFrequencyResult);
  int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
  
  if (RegisterClassA(&WindowClass))
    {
      // Window creation
      HWND Window = CreateWindowExA(
				    0,
				    WindowClass.lpszClassName,
				    "Night Walk",
				    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
				    CW_USEDEFAULT,
				    CW_USEDEFAULT,
				    CW_USEDEFAULT,
				    CW_USEDEFAULT,
				    0,
				    0,
				    Instance,
				    0);
      if(Window)
	{
	  HDC DeviceContext = GetDC(Window);

	  // (Sound)
	  /*             
	  // Sound initializations
	  Win64InitWASAPI(Window);
	  HRESULT hr;
	  int32 SoundBufferSize = 48000 * sizeof(int16) * 2;
	  int16* Samples = (int16*)VirtualAlloc(0, SoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	  
	  // (Game Memory Handling)
#if HANDMADE_INTERNAL
	  LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
	  LPVOID BaseAddress = 0;
#endif

	  // Memory allocations
	  game_memory GameMemory = {};
	  GameMemory.PermanentStorageSize = Megabytes(64);
	  GameMemory.TransientStorageSize = Gigabytes(4);


	  uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

	  GameMemory.PermanentStorage = (int16*)VirtualAlloc(BaseAddress,
							     TotalSize, 
							     MEM_RESERVE|MEM_COMMIT,
							     PAGE_READWRITE);
	  GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
	  // GameMemory.TransientStorage = (int16*)VirtualAlloc(0, GameMemory.TransientStorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	  if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage){
	  */
	  
	  game_input Input[2] = {};
	  game_input* NewInput = &Input[0];
	  game_input* OldInput = &Input[1];
	  
	  // (Performance tracking initializations)
	  LARGE_INTEGER LastCounter;
	  QueryPerformanceCounter(&LastCounter);
	  int64 LastCycleCount =  __rdtsc();
	  
	  GlobalRunning = true;	  
	  while(GlobalRunning){
	 
	    MSG Message;
	    
 	    // win64_offscreen_buffer Buffer;
	    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){
	      if(Message.message == WM_QUIT){
		GlobalRunning = false;
	      }
	      TranslateMessage(&Message);
	      DispatchMessage(&Message); 
	    }
	    
	    // (Win32 audio padding)
	    /* 
	    UINT32 currentPaddingFrames;
	    hr = pAudioClientGlobal->GetCurrentPadding(&currentPaddingFrames);
	    if(FAILED(hr)){ OutputDebugStringA("Failed to get current audio frame padding");}
	    UINT32 currentAvailableFrames = bufferFrameCountGlobal - currentPaddingFrames; */
	    
	    // Video declarations
	    
	    game_offscreen_buffer GraphicalBuffer = {};
	    GraphicalBuffer.Memory = GlobalBackBuffer.Memory;
	    GraphicalBuffer.Width = GlobalBackBuffer.Width;
	    GraphicalBuffer.Height = GlobalBackBuffer.Height;
	    GraphicalBuffer.Pitch = GlobalBackBuffer.Pitch;
	    
	    if (currentAvailableFrames > 0){

	      // Platform-ind. Audio Init
	      // safer size: (48000 < currentAvailableFrames) ? currentAvailableFrames : 48000

	      int16* Samples = (int16*)VirtualAlloc(0, currentAvailableFrames * sizeof(int16), MEM_COMMIT, PAGE_READWRITE);
	      game_sound_output_buffer SoundBuffer = {};
	      SoundBuffer.SamplesPerSecond = sampleRateGlobal;
	      SoundBuffer.SampleCount = currentAvailableFrames;
	      SoundBuffer.Samples = Samples;

	      // Main Audio + Video Render Call
	      GameUpdateAndRender(&GameMemory, NewInput, &GraphicalBuffer, &SoundBuffer);

	      // Writing Audio to Win32
	      BYTE* pData = NULL; 
	      hr = pRenderClientGlobal->GetBuffer(currentAvailableFrames, &pData);
	      if(FAILED(hr)){ OutputDebugStringA("Failed to get audio buffer"); }
	      for(UINT32 i = 0; i < currentAvailableFrames; ++i){
		((float*)pData)[i*2] = SoundBuffer.Samples[i] / 32768.0f; // Left Channel
		((float*)pData)[i*2 + 1] = SoundBuffer.Samples[i] / 32768.0f; // Right Channel
	      }
	      pRenderClientGlobal->ReleaseBuffer(currentAvailableFrames, 0);
	      
	    }
	    else{
	      GameUpdateAndRender(&GameMemory, NewInput, &GraphicalBuffer, NULL);
	    }
	    
	    win64_window_dimension Dimension = GetWindowDimension(Window);
	    Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
	 
	    //++GlobalXOffset;

	    // Time Tracking (QueryPerformanceCounter, rdtsc): 
	    int64 EndCycleCount = __rdtsc();
	    LARGE_INTEGER EndCounter;
	    QueryPerformanceCounter(&EndCounter);	    
	    
	    int64 CyclesElapsed = EndCycleCount - LastCycleCount;
	    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
	    LastCounter = EndCounter;

	    // (Debugging: )
	    real32 MSPerFrame = ((real32)(1000.0f * (real32)CounterElapsed) / (real32)PerfCountFrequency);
	    real32 FPS = (real32)PerfCountFrequency / (real32)CounterElapsed;
	    real32 MCPF = (real32)CyclesElapsed / (1000.0f * 1000.0f);

	    char Buffer[256];
	    sprintf(Buffer, "ms / frame: %.02fms --- FPS: %.02ffps --- m-cycles / frame: %.02f\n", MSPerFrame, FPS, MCPF);
	    OutputDebugStringA(Buffer);
	    

	    game_input* Temp = NewInput;
	    NewInput = OldInput;
	    OldInput = Temp;
	    // TODO: should we clear these here?

	    
	    // (end of while(GlobalRunning loop)
	  }

	  hr = pAudioClientGlobal->Stop();
	  if(FAILED(hr)){ OutputDebugStringA("stopping of audio client failed"); }
       

	  // (Memory closing braces)
	  /* }
	  else{
	    // TODO: memory error logging
	    } */
	}
      else{
	// TODO: CreateWindowExA error handling
      }
    }
  else{
    // TODO: RegisterClassA error handling
  }
  
  
  return 0;}





