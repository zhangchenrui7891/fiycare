#pragma once
#include <windows.h>
#include <wrl.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <vector>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <climits>
#include <memory>
#include <string>
using Microsoft::WRL::ComPtr;
using namespace DirectX;
inline std::ofstream diagnostics;
inline void logLine(const std::string& text){if(diagnostics){diagnostics<<text<<'\n';diagnostics.flush();}}
inline void check(HRESULT hr,const char* operation="DirectX call"){
 if(FAILED(hr)){std::ostringstream message;message<<operation<<" failed: HRESULT 0x"<<std::hex<<static_cast<unsigned long>(hr);logLine(message.str());throw std::runtime_error(message.str());}
}
#include "shader_cache.h"
inline compilation::ShaderCache<ComPtr<ID3DBlob>>& shaderBytecodeCache(){
 static compilation::ShaderCache<ComPtr<ID3DBlob>> cache;return cache;
}
inline ComPtr<ID3DBlob> compileCachedHlsl(const char* source,const char* entry,const char* profile,
                                        UINT flags1=D3DCOMPILE_ENABLE_STRICTNESS,UINT flags2=0){
 if(!source||!entry||!profile)throw std::invalid_argument("Null shader compilation request");
 const compilation::ShaderRequest request{source,entry,profile,flags1,flags2};
 return shaderBytecodeCache().get(request,[&](){
  ComPtr<ID3DBlob> bytecode,error;
  HRESULT hr=D3DCompile(request.source.data(),request.source.size(),nullptr,nullptr,nullptr,
                       request.entry.c_str(),request.profile.c_str(),request.flags1,request.flags2,&bytecode,&error);
  if(FAILED(hr)&&error){std::string message(static_cast<char*>(error->GetBufferPointer()),error->GetBufferSize());logLine(message);throw std::runtime_error(message);}
  check(hr,"D3DCompile");
  if(!bytecode||!bytecode->GetBufferSize())throw std::runtime_error("D3DCompile returned empty bytecode");
  return bytecode;
 });
}
inline void logShaderBytecodeCache(){
 const auto& cache=shaderBytecodeCache();const auto& stats=cache.stats();
 logLine("Shader CPU bytecode cache: requests="+std::to_string(stats.requests)+
         " compilations="+std::to_string(stats.compilations)+" hits="+std::to_string(stats.hits)+
         " failures="+std::to_string(stats.failures)+" evictions="+std::to_string(stats.evictions)+
         " entries="+std::to_string(cache.size())+" (process-local; GPU resources still recreated)");
}
#include "assets.h"
#include "texture_quality.h"
inline std::filesystem::path assetFolder(){wchar_t path[32768]{};auto n=GetModuleFileNameW(nullptr,path,32768);if(!n||n>=32768)throw std::runtime_error("Executable path unavailable");return std::filesystem::path(path).parent_path()/L"assets";}
#include "lighting.h"
#include "render_stability.h"
#include "frame_geometry.h"
#include "lighting_shader.h"
#include "antialias.h"
struct alignas(16) CameraData{XMFLOAT4X4 matrix;LightingFrame frame;};
static_assert(sizeof(CameraData)==240,"HLSL constant layout");
inline CameraData cameraData(const XMMATRIX& matrix,float mode){CameraData d{};XMStoreFloat4x4(&d.matrix,matrix);d.frame=lightingFrame;d.frame.options[2]=mode;if(mode==3)d.frame.eye={0,0,0,1};return d;}
inline XMMATRIX reversedPerspective(float fov,float aspect,float nearDistance,float farDistance){
 const auto depth=presentation::DepthProjection::reversed(nearDistance,farDistance);
 const float y=1.f/std::tan(fov*.5f);
 return XMMatrixSet(y/aspect,0,0,0, 0,y,0,0, 0,0,depth.scale,1, 0,0,depth.offset,0);
}
inline const Vertex skyVertices[6]={{-1,-1,0,1,1,1},{-1,1,0,1,1,1},{1,1,0,1,1,1},{-1,-1,0,1,1,1},{1,1,0,1,1,1},{1,-1,0,1,1,1}};
inline ComPtr<ID3DBlob> compileShader(const char* entry,const char* model){
 return compileCachedHlsl(shaderCode,entry,model);
}
inline void writeBitmap(const std::filesystem::path& path,UINT width,UINT height,const unsigned char* rgba,size_t pitch){
 BITMAPFILEHEADER file{};BITMAPINFOHEADER info{};
 info.biSize=sizeof(info);info.biWidth=(LONG)width;info.biHeight=-(LONG)height;info.biPlanes=1;info.biBitCount=32;info.biCompression=BI_RGB;info.biSizeImage=width*height*4;
 file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(info);file.bfSize=file.bfOffBits+info.biSizeImage;
 std::ofstream out(path,std::ios::binary);out.write(reinterpret_cast<char*>(&file),sizeof(file));out.write(reinterpret_cast<char*>(&info),sizeof(info));
 std::vector<unsigned char> row(width*4);
 for(UINT y=0;y<height;y++){
  const auto* src=rgba+y*pitch;
  for(UINT x=0;x<width;x++){row[x*4]=src[x*4+2];row[x*4+1]=src[x*4+1];row[x*4+2]=src[x*4];row[x*4+3]=255;}
  out.write(reinterpret_cast<char*>(row.data()),static_cast<std::streamsize>(row.size()));
 }
 out.flush();if(!out)throw std::runtime_error("Screenshot write failed");
}
struct IRenderer {
 virtual ~IRenderer()=default;
 virtual const char* name()const=0;
 virtual void init(HWND window,bool software)=0;
 virtual void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& view,float light,const std::filesystem::path& photo={},const std::vector<Vertex>& cabin={},const XMMATRIX& cabinView=XMMatrixIdentity())=0;
};
