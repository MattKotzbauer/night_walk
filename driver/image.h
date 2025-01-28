#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#if !defined(IMAGE_H)

// debug ver
/* 
internal void LoadImageToGameMap(game_map* GameMap, char* Filename){
    int Width, Height, Channels;
    unsigned char* PixelBuffer = stbi_load(Filename, &Width, &Height, &Channels, 4);
    
    if(PixelBuffer){
        // Debug: Print first few pixels
        OutputDebugStringA("First 8 pixels:\n");
        for(int i = 0; i < 8; i++) {
            char DebugBuffer[256];
            sprintf(DebugBuffer, "Pixel %d: %02x %02x %02x %02x\n", 
                    i,
                    PixelBuffer[i*4 + 0],
                    PixelBuffer[i*4 + 1], 
                    PixelBuffer[i*4 + 2],
                    PixelBuffer[i*4 + 3]);
		    OutputDebugStringA(DebugBuffer);
        }
        
        // Also print expected values for first pixel as uint32
        uint32 FirstPixel = 
            (PixelBuffer[3])        | // Alpha
            (PixelBuffer[0] << 24) | // Red
            (PixelBuffer[1] << 16) | // Green
            (PixelBuffer[2] << 8);   // Blue
        
        char DebugBuffer[256];
        sprintf(DebugBuffer, "First pixel as uint32: %08x\n", FirstPixel);
        OutputDebugStringA(DebugBuffer);
    }
}
*/


// base ver

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
	    (PixelBuffer[SrcIndex + 3])        | // Alpha (ff) in lowest byte
	    (PixelBuffer[SrcIndex + 0] << 24) | // Red (22) in highest byte
	    (PixelBuffer[SrcIndex + 1] << 16) | // Green (20) 
	    (PixelBuffer[SrcIndex + 2] << 8);   // Blue (34)
	  
	  /* uint32 CurrentColor = 
	    (PixelBuffer[SrcIndex + 0] << 24)
	    | (PixelBuffer[SrcIndex + 1] << 16)
	    | (PixelBuffer[SrcIndex + 2] << 8)
	    | (PixelBuffer[SrcIndex + 3]); */
	  
	  GameMap->Pixels[DstIndex] = CurrentColor;
	}}
    }
    stbi_image_free(PixelBuffer);
  }
}


#define IMAGE_H
#endif
