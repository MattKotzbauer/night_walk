// INCLUDES / DEFINES

#include <windows.h> // (base windows functionality)
#include <mmdeviceapi.h> // (core audio api's)
#include <audioclient.h>
#include <stdint.h> // (included by default with GLAD)
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stdlib.h> // TODO: see if we can put in rand() ourselves without the rest of the file
#include <math.h> // TODO: examine math implementations, see if we can hand-make? (e.g. sin)
#include <stdio.h> // (for temporary debugging (sprintf for performance tracking))

#define global_variable static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef float real32;
typedef double real64;

typedef int32_t bool32;

#include "shader.h"

// (Used in particle structs)
global_variable const uint32 InternalWidth = 320;
global_variable const uint32 InternalHeight = 180;

// STRUCTS

// (Window structs)
struct win64_window_dimension
{
  int Width;
  int Height;
};
struct win64_offscreen_buffer
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

// (Input structs)
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

// (Sound structs)
struct game_sound_output_buffer
{
  int SamplesPerSecond;
  int16 SampleCount;
  int16 *Samples;
};

// (Particle structs)
struct game_raindrop {
  real32 PosX, PosY;
  real32 Angle, Velocity;
  real32 Size; // (Radius in pixels)
  bool32 Active;
};

struct rain_system {
  internal const uint32 MAX_RAINDROPS = 1000;
  game_raindrop GameRaindrops[MAX_RAINDROPS];
  uint32 ActiveCount;
  
  void Init(){
    //ActiveCount = MAX_RAINDROPS / 2;
    ActiveCount = 900;
    for(int i = 0; i < ActiveCount; ++i){
      Reset(GameRaindrops[i]);
    }
  }
  
  void Update(){
    for(int i = 0; i < ActiveCount; ++i){
      game_raindrop& Drop = GameRaindrops[i];
      if(!Drop.Active) continue;
      
      // (Move pixel along velocity if defined)
      Drop.PosX += Drop.Velocity * sin(Drop.Angle);;
      Drop.PosY += -Drop.Velocity * cos(Drop.Angle);
      // (Angle, Velocity, Size remain constant)
    
      // (Check for ground collision)
      if(Drop.PosY < 0 || Drop.PosX < 0 || Drop.PosX > InternalWidth){
	// (resetting logic)
	Reset(Drop);
      }
    }
  }

  void Reset(game_raindrop& Drop){
    // (When drop is either (A) unitialized or (B) reaches bottom of screen)

    // (set random posX (any pixel within viewport))
    Drop.PosX = (real32)(rand() % InternalWidth);
    Drop.PosY = (real32)(InternalHeight + 10); // TODO: do we need to randomize? e.g. (real32)(InternalHeight + (rand() % 50));
    
    // (set random angle: (4 - 4.5 radians))
    Drop.Angle = 4.0f + (0.5f * ((real32)rand() / RAND_MAX));

    // (set random velocity: (3 - 5))
    Drop.Velocity = 3.0f + (2.0f * ((real32)rand() / RAND_MAX));

    // (size, active)
    Drop.Size = 1.0f + ((real32)rand() / RAND_MAX); // TODO: can size be uniform?
    Drop.Active = true;

    
  }
  
};

global_variable rain_system GlobalRainSystem;

// (GLOBALS)

// (Audio)
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient = __uuidof(IAudioRenderClient);
global_variable IAudioClient* pAudioClientGlobal;
global_variable IAudioRenderClient* pRenderClientGlobal;
global_variable UINT32 bufferFrameCountGlobal;
global_variable real32 sampleRateGlobal;

global_variable bool GlobalRunning;
global_variable win64_offscreen_buffer GlobalBackBuffer;

// (Internal Display)
#define IX(i,j) (((i)*InternalWidth)+(j))
global_variable uint32* InternalPixels;

global_variable const uint32 Red = (0xFF << 24);
global_variable const uint32 Green = (0xFF << 16);
global_variable const uint32 Blue = (0xFF << 8);
global_variable const uint32 Alpha = 0XFF;

struct GLBuffer {
  // (Base rendering resources)
  GLuint MainTexture;
  GLuint FrameVAO;
  GLuint FrameVBO;
  GLuint FrameEBO;
  Shader* BaseShader;
  uint32* Pixels;
  
  // (Rain rendering)
  GLuint RainVAO;
  GLuint RainVBO; // (single-raindrop vertex data)
  GLuint RainInstanceVBO; // (data for instances)
  GLuint RainTexture;
  Shader* RainShader;
};

global_variable GLBuffer GlobalGLRenderer;

// HELPER FUNCTIONS

// (OpenGL Error Checking))
internal void CheckGLError(char* label){
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR) {
    char errorMsg[256];
    sprintf(errorMsg, "OpenGL error at %s: 0x%x\n", label, err);
    OutputDebugStringA(errorMsg);
  }
}

// (Gets Current Window Dimensions)
internal win64_window_dimension GetWindowDimension(HWND Window){
  win64_window_dimension Result;
  
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;

  return Result;
}

// (Windows Audio Init.)
internal void Win64InitWASAPI(HWND Window){

  // https://learn.microsoft.com/en-us/windows/win64/coreaudio/wasapi
  HRESULT hr;

  // 1: Initialize COM
  hr = CoInitializeEx(
			 NULL,
			 COINIT_MULTITHREADED
			 );
  if FAILED(hr){ return;}

  // 2: Create IMMDeviceEnumerator
  IMMDeviceEnumerator* pEnumerator = NULL;
  hr = CoCreateInstance(
			__uuidof(MMDeviceEnumerator),
			NULL,
			CLSCTX_ALL,
			IID_PPV_ARGS(&pEnumerator)
			);
  if FAILED(hr){ return; }
  
  // 3: Get default audio endpoint
  IMMDevice* pDevice = NULL;
  hr = pEnumerator->GetDefaultAudioEndpoint(
					    eRender,
					    eConsole,
					    &pDevice
					    );
  if(FAILED(hr)){ return; }

  // 4: Activate IAudioClient
  //IAudioClient *pAudioClient = NULL;
  pAudioClientGlobal = NULL;
  hr = pDevice->Activate(
			 IID_IAudioClient,
			 CLSCTX_ALL,
			 NULL,
			 (void**)&pAudioClientGlobal
			 );
  if (FAILED(hr)){
    OutputDebugStringA("Activate IAudioClient failed.\n"); return; }
  
  // 5: Get mix format
  WAVEFORMATEX *pwfx = NULL;
  hr = pAudioClientGlobal->GetMixFormat(&pwfx);
  if (FAILED(hr)) { return; }
  
  // 6: Initialize audio format
  hr = pAudioClientGlobal->Initialize(
				AUDCLNT_SHAREMODE_SHARED,
				0,
				10000000,
				0,
				pwfx,
				NULL
				);
  if (FAILED(hr)) { return; }
      
  // 7: get size of allocated buffer
  //  UINT32 bufferFrameCount;
  
  hr = pAudioClientGlobal->GetBufferSize(&bufferFrameCountGlobal);
  if (FAILED(hr)) { return; }
  
  // 8: 
  //IAudioRenderClient* pRenderClient = NULL;
  pRenderClientGlobal = NULL;
  hr = pAudioClientGlobal->GetService(
				IID_IAudioRenderClient,
				(void**)&pRenderClientGlobal
				);
  if (FAILED(hr)) { return; }
  
  // 9: start audio client
  hr = pAudioClientGlobal->Start();
  if (FAILED(hr)) { return; }

  // (Check pwfx format)
  bool isFormatFloat = (pwfx->wFormatTag == WAVE_FORMAT_EXTENSIBLE) ||
                   (pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT);
  if(!isFormatFloat) { return; }

  sampleRateGlobal = pwfx->nSamplesPerSec;  
}

// (Widow Resize)
internal void Win64ResizeDIBSection(win64_offscreen_buffer *Buffer, int Width, int Height){
  if (Buffer->Memory)
    {
      VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;
  
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  // NOTE: when biHeight field is negative, serves as clue to Windows to treat bitmap as top-down rather than bottom-up: first 3 bytes of image are colors for top-left pixel
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;

  int BitmapMemorySize = (Width * Height) * Buffer->BytesPerPixel;
  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

  // TODO: probably clear this to black
  Buffer->Pitch = Buffer->BytesPerPixel * Width;
  
}

// (OpenGL Windows Initialization)
internal void Win64InitOpenGL(HWND Window, HDC WindowDC){

  // HDC WindowDC = GetDC(Window); 
  
  PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
  DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
  DesiredPixelFormat.nVersion = 1;
  DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER; // (remove doublebuffer?)
  DesiredPixelFormat.cColorBits = 32;
  DesiredPixelFormat.cAlphaBits = 8;
  DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
 
  int32 SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);

  PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
  DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex, sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
  
  SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
  
  HGLRC OpenGLRC = wglCreateContext(WindowDC);
  if(wglMakeCurrent(WindowDC, OpenGLRC)){
    if(gladLoadGL()){
      // Success
      OutputDebugStringA("InitOpenGL() Success: Set Context, Loaded GL from GLAD\n");
    }
    else{
    }
  }
  else{OutputDebugStringA("ERROR::OpenGL Initialization");}
  
  // (Print OpenGL version, vendor info)
  /* 
  const GLubyte* version = glGetString(GL_VERSION);
  const GLubyte* vendor = glGetString(GL_VENDOR);
  char debugInfo[256];
  sprintf(debugInfo, "OpenGL Version: %s\nVendor: %s\n", version, vendor);
  OutputDebugStringA(debugInfo);
  */
  
  // ReleaseDC(Window, WindowDC);
}

// (OpenGL Texturing Init.)
internal void InitGlobalGLRendering(){

  {/* 1: Shader Loading / Init */}
  {
    // (Base shader declarations)
    GlobalGLRenderer.BaseShader = new Shader("../driver/shader.vert", "../driver/shader.frag");
    // GlobalGLRenderer.BaseShader->Use();
    // GlobalGLRenderer.BaseShader->SetInt("gameTexture", 0);

    // (Rain shader declarations)
    GlobalGLRenderer.RainShader = new Shader("../driver/rain.vert", "../driver/rain.frag");
    // GlobalGLRenderer.RainShader->Use();
    // GlobalGLRenderer.RainShader->SetInt("rainTexture", 0)
  }

  {/* 2: Base Scene Setup */ }
  {
    real32 BaseVertices[] = {
      // positions        // texture coords
      -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
      1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
      1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
      -1.0f, -1.0f, 0.0f,  0.0f, 0.0f
    };
        
    uint32 BaseIndices[] = {
      0, 1, 2,
      2, 3, 0    
    };

    // Generate and bind base VAO/VBO/EBO
    glGenVertexArrays(1, &GlobalGLRenderer.FrameVAO);
    glGenBuffers(1, &GlobalGLRenderer.FrameVBO);
    glGenBuffers(1, &GlobalGLRenderer.FrameEBO);
        
    glBindVertexArray(GlobalGLRenderer.FrameVAO);
        
    glBindBuffer(GL_ARRAY_BUFFER, GlobalGLRenderer.FrameVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(BaseVertices), BaseVertices, GL_STATIC_DRAW);
        
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GlobalGLRenderer.FrameEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(BaseIndices), BaseIndices, GL_STATIC_DRAW);
        
    // (Position attribute)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // (Texture coords attribute)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
  }
  
  {/* 3: Rain Scene Setup */}
  {
    real32 RainVertices[] = {
      // positions     // texture coords
      -0.5f, -0.5f,   0.0f, 0.0f,
      0.5f, -0.5f,   1.0f, 0.0f,
      0.5f,  0.5f,   1.0f, 1.0f,
      -0.5f,  0.5f,   0.0f, 1.0f
    };

    // Generate and bind rain VAO/VBO
    glGenVertexArrays(1, &GlobalGLRenderer.RainVAO);
    glGenBuffers(1, &GlobalGLRenderer.RainVBO);
    glGenBuffers(1, &GlobalGLRenderer.RainInstanceVBO);
        
    glBindVertexArray(GlobalGLRenderer.RainVAO);
        
    glBindBuffer(GL_ARRAY_BUFFER, GlobalGLRenderer.RainVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(RainVertices), RainVertices, GL_STATIC_DRAW);

    // (Positions)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(real32), (void*)0);
    glEnableVertexAttribArray(0);
    // (Texture coords)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(real32), (void*)(2 * sizeof(real32)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, GlobalGLRenderer.RainInstanceVBO);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(real32), (void*)0);  // Position
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(real32), (void*)(2 * sizeof(real32))); // Velocity
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(2, 1);  // Instance rate for position
    glVertexAttribDivisor(3, 1);  // Instance rate for velocity	
  }

  {/* 4: Texture Setup */}
  {
    // (A: Main scene texture)
    glGenTextures(1, &GlobalGLRenderer.MainTexture);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.MainTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // (min filter)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // (mag filter)

    GlobalGLRenderer.Pixels = (uint32*)VirtualAlloc(0,
						    sizeof(uint32) * InternalWidth * InternalHeight,
						    MEM_RESERVE|MEM_COMMIT,
						    PAGE_READWRITE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, InternalWidth, InternalHeight, 0,
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Pixels);
    
    // (B: Rain texture)
    glGenTextures(1, &GlobalGLRenderer.RainTexture);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.RainTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    /*
      Their ImplementatioN:
      const int TEX_SIZE = 32;
      std::vector<unsigned char> texData(TEX_SIZE * TEX_SIZE, 0);
      for(int y = 0; y < TEX_SIZE; y++) {
      for(int x = 0; x < TEX_SIZE; x++) {
      float dx = (x - TEX_SIZE/2.0f) / (TEX_SIZE/4.0f);
      float dy = (y - TEX_SIZE/2.0f) / (TEX_SIZE/2.0f);
      float dist = dx*dx + dy*dy;
      texData[y*TEX_SIZE + x] = (dist <= 1.0f) ? 255 : 0;
      }
      }
      glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TEX_SIZE, TEX_SIZE, 0,
      GL_RED, GL_UNSIGNED_BYTE, texData.data());      
     */

  }
  
  GlobalGLRenderer.BaseShader->Use();
  GlobalGLRenderer.BaseShader->SetInt("gameTexture", 0);

  GlobalGLRenderer.RainShader->Use();
  GlobalGLRenderer.RainShader->SetInt("rainTexture", 0);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  
  // (Manual blitting for internal texture)
  // for(int i = 0; i < InternalHeight; ++i){for(int j = 0; j < InternalWidth; ++j){ GlobalGLRenderer.Pixels[IX(i,j)] = Alpha|Blue; }}
  // for(int i = 0; i < 100; ++i){ GlobalGLRenderer.Pixels[IX(i, 100)] = Alpha|Red|Blue;}

  /// (Enable blending for particle effects. I'd be careful of how this affects the main render)
  
}

// (OpenGL-Based Screen Blitting)
internal void Win64DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight, win64_offscreen_buffer *Buffer){

  real32 TargetAspectRatio = (real32)InternalWidth / (real32)InternalHeight;
  real32 WindowAspectRatio = (real32)WindowWidth / (real32)WindowHeight;

  int32 ViewportX = 0;
  int32 ViewportY = 0;
  int32 ViewportWidth = WindowWidth;
  int32 ViewportHeight = WindowHeight;

  
  if(WindowAspectRatio > TargetAspectRatio){
    ViewportWidth = (int32)(WindowWidth / TargetAspectRatio);
    ViewportX = (WindowWidth - ViewportHeight) / 2;
  }
  else{
    ViewportHeight = (int32)(WindowHeight / TargetAspectRatio);
    ViewportY = (WindowHeight - ViewportHeight) / 2;
  }
  
  glClearColor(1.0f, 0.0f, 1.0f, 0.0f); // Pink
  glClear(GL_COLOR_BUFFER_BIT);

  glViewport(ViewportX, ViewportY, ViewportWidth, ViewportHeight);
  // CheckGLError("After viewport");

  GlobalGLRenderer.BaseShader->Use();
  // CheckGLError("After shader use");

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.MainTexture);

  // CheckGLError("After bind texture");
  
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, InternalWidth, InternalHeight,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Pixels);

  // CheckGLError("After texsubimage");
  
  glBindVertexArray(GlobalGLRenderer.FrameVAO);

  // CheckGLError("After bind vao");
  
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

  // CheckGLError("After draw");
  
  SwapBuffers(DeviceContext);

}

// (Window Procedure)
LRESULT Win64MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam){
  LRESULT Result = 0;

  switch(Message){

  case WM_SYSKEYDOWN:
    {
      if(WParam == VK_F4 && (LParam & (1 << 29))){
	GlobalRunning = false;
      }
    } break;
    
  case WM_CLOSE:
    {
      GlobalRunning = false;
    } break;
    
  case WM_DESTROY:
    {
      GlobalRunning = false;
    } break;
    
  case WM_PAINT:
    {
      PAINTSTRUCT Paint;
      HDC DeviceContext = BeginPaint(Window, &Paint);

      win64_window_dimension Dimension = GetWindowDimension(Window);
      Win64DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);

      EndPaint(Window, &Paint); // (returns bool)
 
    } break;
    
  default:
    {
      Result = DefWindowProcA(Window, Message, WParam, LParam);
    } break;
    
  }
  
  return Result;
}

  

/*

TODO: Implement:
* Win64MainWindowCallback
* Win64ResizeDIBSection
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

  Win64ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
  
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
	  Win64InitOpenGL(Window, DeviceContext);
	  InitGlobalGLRendering();
	  
	  // Sound initializations
	  Win64InitWASAPI(Window);
	  HRESULT hr;
	  int32 SoundBufferSize = 48000 * sizeof(int16) * 2;
	  int16* Samples = (int16*)VirtualAlloc(0, SoundBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	  /* 
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
	    
	    // (Win64 audio padding)
	    
	    UINT32 currentPaddingFrames;
	    hr = pAudioClientGlobal->GetCurrentPadding(&currentPaddingFrames);
	    if(FAILED(hr)){ OutputDebugStringA("Failed to get current audio frame padding"); }
	    UINT32 currentAvailableFrames = bufferFrameCountGlobal - currentPaddingFrames;
	    
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
	      // GameUpdateAndRender(&GameMemory, NewInput, &GraphicalBuffer, &SoundBuffer);

	      // Writing Audio to Win64
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
	      // GameUpdateAndRender(&GameMemory, NewInput, &GraphicalBuffer, NULL);
	    }
	    
	    win64_window_dimension Dimension = GetWindowDimension(Window);
	    Win64DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
	 
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





