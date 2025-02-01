#if !defined(SHADER_H)

struct ProcessedFile{
  void* Contents;
  uint32 ContentsSize; 
};

internal ProcessedFile ReadEntireFile(char* Filename){
  ProcessedFile Result = {};
  HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
  if(FileHandle != INVALID_HANDLE_VALUE){
    LARGE_INTEGER FileSize;
    if(GetFileSizeEx(FileHandle, &FileSize)){
      uint32 FileSize32 = (uint32)FileSize.QuadPart;
      Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
      if(Result.Contents){
	DWORD BytesRead;
	if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)){
	  Result.ContentsSize = FileSize32;
	}
	else{
	  if(Result.Contents){
	    VirtualFree(Result.Contents, 0, MEM_RELEASE);
	    Result.Contents = 0;
	  }
	}
      }
    }
    CloseHandle(FileHandle);
  }
  return Result;
}

  
struct Shader{
  uint32 ID;

  Shader(char* VertexPath, char* FragmentPath){
    ProcessedFile VertexShaderFile = ReadEntireFile(VertexPath);
    ProcessedFile FragmentShaderFile = ReadEntireFile(FragmentPath);
    if(!VertexShaderFile.Contents || !FragmentShaderFile.Contents) {
      OutputDebugStringA("SHADER ERROR: File Read Failed\n");
      return;
    }
    // (Initialize code to empty memory)
    char* VertexCode = (char*)VirtualAlloc(0, VertexShaderFile.ContentsSize + 1,
                                             MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    char* FragmentCode = (char*)VirtualAlloc(0, FragmentShaderFile.ContentsSize + 1,
					     MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    if(VertexCode && FragmentCode){
      // (Copy in memory and set terminating chars)
      CopyMemory(VertexCode, VertexShaderFile.Contents, VertexShaderFile.ContentsSize);
      CopyMemory(FragmentCode, FragmentShaderFile.Contents, FragmentShaderFile.ContentsSize);
      VertexCode[VertexShaderFile.ContentsSize] = '\0';
      FragmentCode[FragmentShaderFile.ContentsSize] = '\0';

      // (Debugging vars)
      int32 Success;
      char InfoLog[1024];
      
      // (Vertex shader compilation)
      uint32 Vertex = glCreateShader(GL_VERTEX_SHADER);
      glShaderSource(Vertex, 1, &VertexCode, NULL);
      glCompileShader(Vertex);

      // (Vertex shader debugging)
      glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Success);
      if(!Success) {
	glGetShaderInfoLog(Vertex, 1024, NULL, InfoLog);
	OutputDebugStringA("SHADER ERROR: Vertex Shader Compilation\n");
	OutputDebugStringA(InfoLog);
      }
      
      // (Fragment shader compilation)
      uint32 Fragment = glCreateShader(GL_FRAGMENT_SHADER);
      glShaderSource(Fragment, 1, &FragmentCode, NULL);
      glCompileShader(Fragment);

      glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Success);
      if(!Success) {
	glGetShaderInfoLog(Fragment, 1024, NULL, InfoLog);
	OutputDebugStringA("SHADER ERROR: Fragment Shader Compilation\n");
	OutputDebugStringA(InfoLog);
      }

      ID = glCreateProgram();
      glAttachShader(ID, Vertex);
      glAttachShader(ID, Fragment);
      glLinkProgram(ID);

      glGetProgramiv(ID, GL_LINK_STATUS, &Success);
      if(!Success) {
	glGetProgramInfoLog(ID, 1024, NULL, InfoLog);
	OutputDebugStringA("SHADER ERROR: Vertex + Shader Linking\n");
	OutputDebugStringA(InfoLog);
      }

      glDeleteShader(Vertex);
      glDeleteShader(Fragment);
      
    }
    // (Free memory)
    if(VertexCode){VirtualFree(VertexCode, 0, MEM_RELEASE);}
    if(FragmentCode){VirtualFree(FragmentCode, 0, MEM_RELEASE);}
    if(VertexShaderFile.Contents){VirtualFree(VertexShaderFile.Contents, 0, MEM_RELEASE);}
    if(FragmentShaderFile.Contents){VirtualFree(FragmentShaderFile.Contents, 0, MEM_RELEASE);}
    
  }

  // Their suggested helpers:
  void Use() { 
    glUseProgram(ID); 
  }
  
  void SetBool(const char* Name, bool Value) {
    glUniform1i(glGetUniformLocation(ID, Name), (int)Value);
  }

  void SetInt(const char* Name, int Value) {
    glUniform1i(glGetUniformLocation(ID, Name), Value);
  }

  void SetFloat(const char* Name, float Value) {
    glUniform1f(glGetUniformLocation(ID, Name), Value);
  }

  void SetVec2(const char* Name, float x, float y) {
    glUniform2f(glGetUniformLocation(ID, Name), x, y);
  }

  void SetVec3(const char* Name, float x, float y, float z) {
    glUniform3f(glGetUniformLocation(ID, Name), x, y, z);
  }

  
  
}; 

#define SHADER_H
#endif
