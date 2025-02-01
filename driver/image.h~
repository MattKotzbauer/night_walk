#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if !defined(IMAGE_H)

// base ver

internal void LoadGameMap(game_map* GameMap, char* Filename){
  int Width, Height, Channels;
  
  unsigned char* PixelBuffer = stbi_load(Filename, &Width, &Height, &Channels, 4);
  if(PixelBuffer){
    if(Height == InternalHeight && Width == FullWidth){
      for(int i = 0; i < Height; ++i){
	for(int j = 0; j < Width; ++j){
	  int SrcIndex = (i * Width + j) * 4;
	  int DstIndex = IX(i,j);

	  uint32 CurrentColor = 
	    (PixelBuffer[SrcIndex + 0] << 24) | // Red
	    (PixelBuffer[SrcIndex + 1] << 16) | // Green
	    (PixelBuffer[SrcIndex + 2] << 8)  | // Blue
	    (PixelBuffer[SrcIndex + 3]);
	  
	  GameMap->Pixels[DstIndex] = CurrentColor;
	}}
    }
    stbi_image_free(PixelBuffer);
  }
}

internal void LoadSpriteMap(sprite_map* SpriteMap, char* Filename){
  int Width, Height, Channels;
 
  unsigned char* PixelBuffer = stbi_load(Filename, &Width, &Height, &Channels, 4);
  if(PixelBuffer){
    for(int i = 0; i < Height; ++i){
      for(int j = 0; j < Width; ++j){
	int SrcIndex = (i * Width + j) * 4;
	int DstIndex = i*(SpriteMapWidth) + j;
	
	uint32 CurrentColor = 
	  (PixelBuffer[SrcIndex + 0] << 24) | // Red
	  (PixelBuffer[SrcIndex + 1] << 16) | // Green
	  (PixelBuffer[SrcIndex + 2] << 8)  | // Blue 
	  (PixelBuffer[SrcIndex + 3]);
	
	
	SpriteMap->Pixels[DstIndex] = CurrentColor;
	
      }
    }
    stbi_image_free(PixelBuffer);
  }
}

internal void LoadNormalMap(game_map* GameMap, char* Filename){
  int Width, Height, Channels;
  
  unsigned char* ValueBuffer = stbi_load(Filename, &Width, &Height, &Channels, 4);
  if(ValueBuffer){
    if(Height == InternalHeight && Width == FullWidth){
      for(int i = 0; i < Height; ++i){
	for(int j = 0; j < Width; ++j){
	  int SrcIndex = (i * Width + j) * 4;
	  int DstIndex = IX(i,j);

	  uint32 CurrentAngle =
	    (ValueBuffer[SrcIndex + 0]) + // Red
	    (ValueBuffer[SrcIndex + 1]); // Green
	  
	  GameMap->Angles[DstIndex] = CurrentAngle;
	}}
    }
    stbi_image_free(ValueBuffer);
  }
  
  // (test if it's alpha first)
 
}


#define IMAGE_H
#endif
