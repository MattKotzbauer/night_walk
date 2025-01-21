#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if !defined(IMAGE_H)


internal void LoadImageToGameMap(game_map* GameMap, char* Filename){
  int Width, Height, Channels;
  
  unsigned char* PixelBuffer = stbi_load(Filename, &Width, &Height, &Channels, 4);
  if(PixelBuffer){
    if(Height == InternalHeight && Width == FullWidth){
      for(int X = 0; X < Width; ++X){
	for(int Y = 0; Y < Height; ++Y){
	  int SrcIndex = (Y * Width + X) * 4;
	  int DstIndex = IX(Y,X); // (Reversing arguments because IX is made for (i,j) notation)
	  
	  
	  uint32 CurrentColor = 
	    (PixelBuffer[SrcIndex + 0] << 24)
	    | (PixelBuffer[SrcIndex + 1] << 16)
	    | (PixelBuffer[SrcIndex + 2] << 8)
	    | (PixelBuffer[SrcIndex + 3]);
	  
	  GameMap->Pixels[DstIndex] = CurrentColor;
	}}
    }
    stbi_image_free(PixelBuffer);
  }
}


#define IMAGE_H
#endif
