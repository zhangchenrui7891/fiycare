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
struct Vertex{float x,y,z,r,g,b;};
inline const char* shaderCode=R"(
cbuffer Camera:register(b0){row_major float4x4 mvp;float light;}
struct V{float3 p:POSITION;float3 c:COLOR;};
struct O{float4 p:SV_POSITION;float3 c:COLOR;};
O vs(V v){O o;o.p=mul(float4(v.p,1),mvp);o.c=v.c*light;return o;}
float4 ps(O i):SV_TARGET{return float4(i.c,1);}
)";
inline ComPtr<ID3DBlob> compileShader(const char* entry,const char* model){
 ComPtr<ID3DBlob> bytecode,error;
 HRESULT hr=D3DCompile(shaderCode,std::strlen(shaderCode),nullptr,nullptr,nullptr,entry,model,D3DCOMPILE_ENABLE_STRICTNESS,0,&bytecode,&error);
 if(FAILED(hr)&&error){std::string message(static_cast<char*>(error->GetBufferPointer()),error->GetBufferSize());logLine(message);throw std::runtime_error(message);}
 check(hr,"D3DCompile");return bytecode;
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
 virtual void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& view,float light,const std::filesystem::path& photo={})=0;
};
