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


// (Used in particle structs)
// (Resolution Width, Height)
global_variable const uint32 InternalWidth = 320;
global_variable const uint32 InternalHeight = 180;

// (Internal Display)
// (Internal translation: column-major)
#define IX(i,j) ((i)+((j)*InternalHeight))
// (OpenGL translation: row-major)
#define OX(i,j) (((i)*InternalWidth)+(j))
#define ArrayCount(arr) (sizeof(arr) / sizeof(arr[0]))

// TODO: refactor for differently-sized sprites
global_variable const uint32 SpriteWidth = 15;
global_variable const uint32 SpriteHeight = 20;
global_variable const uint32 SpritePitch = 10; // (Probably won't have to use)
global_variable const uint32 SpriteMapWidth = 75;
global_variable const uint32 SpriteMapHeight = 20;

// [(i % SpritePitch) * SpriteWidth, (i / SpritePitch) * SpriteHeight]
// #define SpritePosition(i) OX((),())
// #define SpritePosition(i) [(((i)%SpritePitch) * SpriteWidth), (((i)/SpritePitch)*SpriteHeight)]

// (Full Width for game map: used in image.h)
global_variable const uint32 FullWidth = 720; // TODO: change to final full width of game / determine from full map
// (Used for error checking in raindrop struct functions)
internal void CheckGLError(char* label);

struct game_map{
  uint32* Pixels;
  uint32* Angles; // (Angles provided by normal map)
  int32 Width;
  // (Height = InternalHeight)
  int32 PriorXOffset;
  int32 XOffset;
  // int32 PlayerOffset; // (dictates where we draw sprite)
};
global_variable game_map GlobalGameMap;


struct sprite_map{
  uint32* Pixels;
  // int32 MapWidth;
  // int32 MapHeight;
};
global_variable sprite_map GlobalSpriteMap;


// STRUCTS
#include "shader.h"
// (Window structs)
/*
struct player_anim{
int32 StartFrame; // (first frame of animation)
int32 EndFrame; // (last frame of animatioN)
int32 Direction; // (is it reversed?)
}
 */
struct player_state{
  int32 AnimState = 0; // 0 = idle, 1 = walk

  // (Walk animation metadata)
  int32 WalkStart = 1;
  int32 WalkEnd = 5;
  int32 WalkDirection = 1;

  bool32 PlayerReversed = false;  
};
global_variable player_state GlobalPlayerState;

struct GLBuffer {
  // (Base rendering resources)
  GLuint MainTexture;
  GLuint FrameVAO;
  GLuint FrameVBO;
  GLuint FrameEBO;
  Shader* BaseShader;
  uint32* Pixels;

  // (Normal map resources)
  GLuint AngleTexture;
  uint32* Angles;
  
  // (Rain rendering)
  GLuint RainVAO;
  GLuint RainVBO; // (single-raindrop vertex data)
  GLuint RainInstanceVBO; // (data for instances)
  GLuint RainTexture;
  Shader* RainShader;
};
global_variable GLBuffer GlobalGLRenderer;
// (I like putting image.h here, shader.h should also be fine?)

struct light_source{
  real32 PosX, PosY;
  real32 R, G, B;
  real32 Intensity;
  real32 Radius;
  bool32 Active;
};
struct lighting_system{
  static const int MAX_LIGHTS = 8;
  light_source Lights[MAX_LIGHTS];
  int ActiveLightCount;

  void AddLight(real32 PosX, real32 PosY, real32 R, real32 G,
	       real32 B, real32 Intensity, real32 Radius){
    if(ActiveLightCount >= MAX_LIGHTS){ return; }
    int idx = ActiveLightCount++;

    // (Copy aspects)
    Lights[idx].PosX = PosX;
    Lights[idx].PosY = PosY;
    Lights[idx].R = R;
    Lights[idx].G = G;
    Lights[idx].B = B;
    Lights[idx].Intensity = Intensity;
    Lights[idx].Radius = Radius;
    Lights[idx].Active = true;
  }
  
  void UpdateLightUniforms(Shader* LightShader){
    // LightShader->Use();

    LightShader->SetInt("numActiveLights", ActiveLightCount);
    
    for(int i = 0; i < ActiveLightCount; ++i){
      if(!Lights[i].Active){ continue; }

      char buf[64];
      // OutputDebugStringA("\n\n");
      sprintf(buf, "lights[%d].position", i);
      LightShader->SetVec2(buf, Lights[i].PosX, Lights[i].PosY);
      sprintf(buf, "lights[%d].color", i); 
      LightShader->SetVec3(buf, Lights[i].R, Lights[i].G, Lights[i].B);
      sprintf(buf, "lights[%d].intensity", i);
      LightShader->SetFloat(buf, Lights[i].Intensity);      
      sprintf(buf, "lights[%d].radius", i);
      LightShader->SetFloat(buf, Lights[i].Radius);
    }

    LightShader->SetFloat("ambientStrength", 0.2f);
    
  }
};
global_variable lighting_system GlobalLightingSystem;


#include "image.h"

// (Pixel dimensions of current window)
struct win64_window_dimension
{
  int Width;
  int Height;
};
// (Win64 window metadata)
struct win64_offscreen_buffer
{
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
 };
// (Internal game metadata for window blitting)
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

  const real32 RainVertices[16] = {
    // positions     // texture coords
    -0.5f, -0.5f,   0.0f, 0.0f,
    0.5f, -0.5f,   1.0f, 0.0f,
    0.5f,  0.5f,   1.0f, 1.0f,
    -0.5f,  0.5f,   0.0f, 1.0f
  };

  void InitGL(){
    
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

    glGenTextures(1, &GlobalGLRenderer.RainTexture);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.RainTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    const int32 TEX_SIZE = 32;
    unsigned char texData[TEX_SIZE * TEX_SIZE];

    // (Base texture)
    /* 
       for(int y = 0; y < TEX_SIZE; y++) {
       for(int x = 0; x < TEX_SIZE; x++) {
       texData[y*TEX_SIZE + x] = 255;
       }
       }
    */
    // (Ellipse texture:)
    for(int y = 0; y < TEX_SIZE; y++) {
      for(int x = 0; x < TEX_SIZE; x++) {
	float dx = (x - TEX_SIZE/2.0f) / (TEX_SIZE/4.0f);
	float dy = (y - TEX_SIZE/2.0f) / (TEX_SIZE/2.0f);
	float dist = dx*dx + dy*dy;
	texData[y*TEX_SIZE + x] = (dist <= 1.0f) ? 255 : 0;
      }}
	
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, TEX_SIZE, TEX_SIZE, 0,
		 GL_RED, GL_UNSIGNED_BYTE, texData);
    
}
  
  void Init(){
    //ActiveCount = MAX_RAINDROPS / 2;
    ActiveCount = 900;
    for(int i = 0; i < ActiveCount; ++i){
      Reset(GameRaindrops[i]);
    }
    UpdateInstanceData();
  }

  void UpdateInstanceData(){
    real32 InstanceDataBuffer[MAX_RAINDROPS*4];

    for(int i = 0; i < ActiveCount; ++i){
      const game_raindrop& CurrentDrop = GameRaindrops[i];
      if(!CurrentDrop.Active) continue;

      InstanceDataBuffer[i * 4] = CurrentDrop.PosX;
      InstanceDataBuffer[i * 4 + 1] = CurrentDrop.PosY;
      InstanceDataBuffer[i * 4 + 2] = CurrentDrop.Velocity * sin(CurrentDrop.Angle);
      InstanceDataBuffer[i * 4 + 3] = -CurrentDrop.Velocity * cos(CurrentDrop.Angle);
    }

    glBindBuffer(GL_ARRAY_BUFFER, GlobalGLRenderer.RainInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
		 sizeof(real32) * ActiveCount * 4,
		 InstanceDataBuffer,
		 GL_DYNAMIC_DRAW);

    
  }
  
  void Update(){
    for(int i = 0; i < ActiveCount; ++i){
      game_raindrop& Drop = GameRaindrops[i];
      if(!Drop.Active) continue;
      
      // (Move pixel along velocity if defined)
      Drop.PosX += Drop.Velocity * sin(Drop.Angle);;
      Drop.PosY += Drop.Velocity * cos(Drop.Angle);
      // (Angle, Velocity, Size remain constant)
    
      // (Check for ground collision)
      if(Drop.PosY < 0 || Drop.PosX < 0){
	// (resetting logic)
	Reset(Drop);
      }
    }
    UpdateInstanceData();
  }

  void Reset(game_raindrop& Drop){
    // (When drop is either (A) unitialized or (B) reaches bottom of screen)

    // (set random posX (any pixel within viewport))
    real32 x = (real32)(rand() % (InternalHeight + InternalWidth + 20));
    Drop.PosX = x > (InternalWidth + 10) ? (InternalWidth + 10) : x;
    Drop.PosY = x > (InternalWidth + 10) ?  x - (InternalWidth + 10) : (InternalHeight + 10);
    
    // Drop.PosX = (real32)(rand() % InternalWidth);
    // Drop.PosY = (real32)(InternalHeight - 10); // TODO: do we need to randomize? e.g. (real32)(InternalHeight + (rand() % 50));
    
    // (set random angle: (4 - 4.5 radians))
    // Drop.Angle = 4.0f + (0.5f * ((real32)rand() / RAND_MAX));
    Drop.Angle = 3.5f + (0.5f * ((real32)rand() / RAND_MAX));
    
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

// (Colors)
global_variable const uint32 Red = (0xFF << 24);
global_variable const uint32 Green = (0xFF << 16);
global_variable const uint32 Blue = (0xFF << 8);
global_variable const uint32 Alpha = 0XFF;

// Framerate Control
global_variable const int TARGET_FPS = 30;
global_variable const real32 TARGET_SECONDS_PER_FRAME = (1.0f / (real32)TARGET_FPS);

// (Scroll speed)
global_variable const real32 SCROLL_SPEED = 1.0f;
global_variable real32 ScrollOffset = 0;

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

internal real32 Win64GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End, int64 PerfCountFrequency){
  real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)PerfCountFrequency);
  return Result;
}

internal void ProcessGameInput(game_input* NewInput, game_input* OldInput){

  NewInput->Left.EndedDown = (GetAsyncKeyState(VK_LEFT) & 0x8000) != 0;
  NewInput->Right.EndedDown = (GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0;

  // OutputDebugStringA(NewInput->Left.EndedDown ? "Left key DOWN\n" : "Left key UP\n");
  // NewInput->Right.EndedDown = true;
  
  if(NewInput->Left.EndedDown){
    // ScrollOffset = max(0, ScrollOffset - SCROLL_SPEED)
    ScrollOffset = (ScrollOffset - SCROLL_SPEED > 0) ? ScrollOffset - SCROLL_SPEED : 0;
    GlobalGameMap.XOffset = (int32)ScrollOffset;
    if(!GlobalPlayerState.PlayerReversed){GlobalPlayerState.PlayerReversed = true;}
  }
  if(NewInput->Right.EndedDown){
    // ScrollOffset = min(ScrollOffset + SCROLL_SPEED, GlobalGameMap.Width)
    ScrollOffset = (ScrollOffset + SCROLL_SPEED < GlobalGameMap.Width) ? ScrollOffset + SCROLL_SPEED : GlobalGameMap.Width;
    GlobalGameMap.XOffset = (int32)ScrollOffset;
    if(GlobalPlayerState.PlayerReversed){GlobalPlayerState.PlayerReversed = false;}
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

// (Load game map into GlobalGLRenderer.Pixels)
internal void LoadInternalMap(int PriorXOffset){
  // 1: Game Map (Column-Major): 
  for(int i = 0; i < InternalHeight; ++i){
    for(int j = 0; j < InternalWidth; ++j){
      int MapJ = j + GlobalGameMap.XOffset;
      if(MapJ >= 0 && MapJ < GlobalGameMap.Width){
	int SrcIndex = IX(i,MapJ);
	// int DstIndex = ((InternalHeight - i) * InternalWidth) + (j);
	int DstIndex = OX((InternalHeight - i), j);
	GlobalGLRenderer.Pixels[DstIndex] = GlobalGameMap.Pixels[SrcIndex];
	GlobalGLRenderer.Angles[DstIndex] = GlobalGameMap.Angles[SrcIndex];
    }}}
  
  // 2: Player Sprite (Row-Major):
  int PlayerBottomOffset = 27;
  int PlayerLeftOffset = 20;

  // int PlayerHeightTEMP = 15;
  // int PlayerWidthTEMP = 10;

  int PlayerHeightTEMP = 20;
  int PlayerWidthTEMP = 15;

  // (dictates frame of animation)
  int SpriteIndex = 4;

  int SpriteStartY = (SpriteIndex / SpritePitch) * SpriteHeight;
  int SpriteStartX = (SpriteIndex % SpritePitch) * SpriteWidth;
  // int32 PlayerBase = SpritePosition(0); // (should return 0)

  
  for(int i = 0; i < SpriteHeight; ++i){
    for(int j = 0; j < SpriteWidth; ++j){
      // TODO: (fix logic on this)
      // int DstIndex = OX(i,j);      int SrcIndex = (() * Sprite )
      
      int SrcIndex = ((SpriteStartY + i) * SpriteMapWidth) + (SpriteStartX + j);
      if(GlobalSpriteMap.Pixels[SrcIndex] & Alpha != 0){
	// int DstIndex = OX((InternalHeight - (PlayerBottomOffset + i)), (PlayerLeftOffset + j));
	// int DstIndex = OX((PlayerBottomOffset + i), (PlayerLeftOffset + j));
	// int DstIndex = OX(InternalHeight - (PlayerBottomOffset - SpriteHeight) - i, (PlayerLeftOffset + j));
	int DstIndex;
	if(GlobalPlayerState.PlayerReversed){
	  DstIndex = OX(((PlayerBottomOffset + SpriteHeight) - i), (PlayerLeftOffset + j));
	}
	else{
	  DstIndex = OX(((PlayerBottomOffset + SpriteHeight) - i), (PlayerLeftOffset + j));
	}
	
	// TODO: test opacity value of pixel
	GlobalGLRenderer.Pixels[DstIndex] = GlobalSpriteMap.Pixels[SrcIndex];
	
      }
    }
  }
  
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
    // (Base shader)
    GlobalGLRenderer.BaseShader = new Shader("../driver/shader.vert", "../driver/shader.frag");
    // (Rain shader)
    GlobalGLRenderer.RainShader = new Shader("../driver/rain.vert", "../driver/rain.frag");

    // CURR TEST:
    GlobalLightingSystem.ActiveLightCount = 0;    
    GlobalLightingSystem.AddLight(InternalWidth / 2.0f, InternalHeight / 2.0f,
				  1.0f, .8f, .6f, 1.0f, 100.0f
				  );
        GlobalLightingSystem.AddLight(InternalWidth / 4.0f, InternalHeight / 3.0f,
				  1.0f, .8f, .6f, 1.0f, 200.0f
				  );
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
    GlobalRainSystem.InitGL();
    GlobalRainSystem.Init();
  }

  {/* 4: Texture Setup */}
  {
    // (A: Main scene texture)
    glGenTextures(1, &GlobalGLRenderer.MainTexture);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.MainTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // (min filter)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // (mag filter)

    // (B: Angle textures)
    glGenTextures(1, &GlobalGLRenderer.AngleTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.AngleTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    
    GlobalGLRenderer.Pixels = (uint32*)VirtualAlloc(0,
						    sizeof(uint32) * InternalWidth * InternalHeight,
						    MEM_RESERVE|MEM_COMMIT,
						    PAGE_READWRITE);
    GlobalGLRenderer.Angles = (uint32*)VirtualAlloc(0,
						    sizeof(uint32) * InternalWidth * InternalHeight,
						    MEM_RESERVE|MEM_COMMIT,
						    PAGE_READWRITE);

    
    // (Initialize textures)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.MainTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, InternalWidth, InternalHeight, 0,
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Pixels);


    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.AngleTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, InternalWidth, InternalHeight, 0,
                 GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Angles);

    
  }


  // TODO: does this work as an end?
  GlobalGLRenderer.BaseShader->Use();

  GlobalGLRenderer.BaseShader->SetFloat("screenWidth", (float)InternalWidth);
  GlobalGLRenderer.BaseShader->SetFloat("screenHeight", (float)InternalHeight);
  
  GlobalGLRenderer.BaseShader->SetInt("gameTexture", 0);
  GlobalGLRenderer.BaseShader->SetInt("angleTexture", 1);
  
  GlobalGLRenderer.RainShader->Use();
  GlobalGLRenderer.RainShader->SetInt("rainTexture", 0);
  
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // GlobalRainSystem.Init();

  // blue blit (TODO: replace with game screen blit)
  // for(int i = 0; i < InternalHeight; ++i){for(int j = 0; j < InternalWidth; ++j){ GlobalGLRenderer.Pixels[IX(i,j)] = Alpha|Blue; }}
  // for(int i = 0; i < 100; ++i){ GlobalGLRenderer.Pixels[IX(i, 100)] = Alpha|Red|Blue;}
  
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
  
  {/* Base Texture Pass */}
  {
    GlobalGLRenderer.BaseShader->Use();

    // THIS IS THE LINE
    GlobalLightingSystem.UpdateLightUniforms(GlobalGLRenderer.BaseShader);

    // (A: Bind main texture)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.MainTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, InternalWidth, InternalHeight,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Pixels);
    // (B: Bind angle texture)
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.AngleTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, InternalWidth, InternalHeight,
                    GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, GlobalGLRenderer.Angles);
    

    glBindVertexArray(GlobalGLRenderer.FrameVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    
  }
  {/* Rain Pass */}
  {
    GlobalGLRenderer.RainShader->Use();
    CheckGLError("After shader use");
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, GlobalGLRenderer.RainTexture);
    CheckGLError("After rain texture bind");
    glBindVertexArray(GlobalGLRenderer.RainVAO);
    CheckGLError("After rain VAO bind");
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, GlobalRainSystem.ActiveCount);
    CheckGLError("After rain draw");
  }

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
* Win64ResizeDIBSection
* game_input (struct) (means of storing game input)
* game_offscreen_buffer (struct) (means of storing )
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

  bool32 SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);

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

	  // (Game map init.)
	  int32 GameMapWidth = 720;
	  int32 GameMapSize = InternalHeight * GameMapWidth * sizeof(uint32);
	  GlobalGameMap.Pixels = (uint32*)VirtualAlloc(0, GameMapSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	  GlobalGameMap.Angles = (uint32*)VirtualAlloc(0, GameMapSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	  // (Sprite map init.)
	  int32 SpriteMapSize = SpriteMapWidth * SpriteMapHeight * sizeof(uint32);
	  GlobalSpriteMap.Pixels = (uint32*)VirtualAlloc(0, SpriteMapSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	  // (Image loading / game map population)
	  GlobalGameMap.Width = GameMapWidth;
	  GlobalGameMap.XOffset = 0;
	  GlobalGameMap.PriorXOffset = 0;
	  LoadImageToGameMap(&GlobalGameMap, "../media/Scene1.png");
	  LoadSpriteMap(&GlobalSpriteMap, "../media/Anim1.png");

	  // LoadInternalMap();
	  
	  game_input Input[2] = {};
	  game_input* NewInput = &Input[0];
	  game_input* OldInput = &Input[1];
	  
	  // (Performance tracking initializations)
	  LARGE_INTEGER LastCounter;
	  QueryPerformanceCounter(&LastCounter);
	  int64 LastCycleCount =  __rdtsc();
	  
	  GlobalRunning = true;	  
	  while(GlobalRunning){

	    // (Base map load)
	    // if(GlobalGameMap.XOffset != GlobalGameMap.PriorXOffset){
	      // LoadInternalMap();
	      // GlobalGameMap.PriorXOffset = GlobalGameMap.XOffset;
	    // }
	    LoadInternalMap(GlobalGameMap.PriorXOffset);
	    GlobalGameMap.PriorXOffset = GlobalGameMap.XOffset;
	 
	    MSG Message;
	    
 	    // win64_offscreen_buffer Buffer;
	    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)){
	      if(Message.message == WM_QUIT){
		GlobalRunning = false;
	      }
	      TranslateMessage(&Message);
	      DispatchMessage(&Message); 
	    }

	    // Input handling
	    ProcessGameInput(NewInput, OldInput);

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

	    GlobalRainSystem.Update();
	    win64_window_dimension Dimension = GetWindowDimension(Window);
	    Win64DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, &GlobalBackBuffer);
	 
	    //++GlobalXOffset;

	    // Time Tracking (QueryPerformanceCounter, rdtsc): 
	    LARGE_INTEGER EndCounter;
	    QueryPerformanceCounter(&EndCounter);	    
	    
	    // Sleep until end of frame
	    real32 SecondsElapsed = Win64GetSecondsElapsed(LastCounter, EndCounter, PerfCountFrequency);
	    while(SecondsElapsed < TARGET_SECONDS_PER_FRAME){
	      if(SleepIsGranular){
		DWORD SleepMS = (DWORD)(1000.0f * (TARGET_SECONDS_PER_FRAME - SecondsElapsed));
		if(SleepMS > 0){
		  Sleep(SleepMS);
		}
	      }
	      QueryPerformanceCounter(&EndCounter);
	      SecondsElapsed = Win64GetSecondsElapsed(LastCounter, EndCounter, PerfCountFrequency);
	    }

	    int64 EndCycleCount = __rdtsc();
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

	  if(SleepIsGranular){ timeEndPeriod(1); }

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
  
  return 0;
}





