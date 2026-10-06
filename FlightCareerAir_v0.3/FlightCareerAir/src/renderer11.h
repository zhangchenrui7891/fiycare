#pragma once
#include "graphics_common.h"
#include <d3d11.h>
#include <dxgi1_2.h>
struct Renderer11 final : IRenderer {
 static constexpr UINT capacity=160000;
 UINT width=960,height=540;
 ComPtr<ID3D11Device> device;
 ComPtr<ID3D11DeviceContext> context;
 ComPtr<IDXGISwapChain> swap;
 ComPtr<ID3D11RenderTargetView> target;
 ComPtr<ID3D11DepthStencilView> depth;
 ComPtr<ID3D11Buffer> vertices,constants;
 ComPtr<ID3D11VertexShader> vs;
 ComPtr<ID3D11PixelShader> ps;
 ComPtr<ID3D11InputLayout> layout;
 ComPtr<ID3D11RasterizerState> raster;
 ComPtr<ID3D11DepthStencilState> worldDepth,hudDepth;
 ComPtr<ID3D11Texture2D> staging;
 const char* name()const override{return "DX11";}
 void init(HWND window,bool software)override{
  DXGI_SWAP_CHAIN_DESC sd{};sd.BufferDesc.Width=width;sd.BufferDesc.Height=height;sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.OutputWindow=window;sd.Windowed=TRUE;sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
  D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0};D3D_FEATURE_LEVEL obtained{};
  check(D3D11CreateDeviceAndSwapChain(nullptr,software?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,1,D3D11_SDK_VERSION,&sd,&swap,&device,&obtained,&context),"D3D11CreateDeviceAndSwapChain");
  ComPtr<IDXGIDevice> dxgi;check(device.As(&dxgi));ComPtr<IDXGIAdapter> adapter;check(dxgi->GetAdapter(&adapter));DXGI_ADAPTER_DESC ad{};check(adapter->GetDesc(&ad));
  std::wstring desc(ad.Description);logLine(std::string("DX11 adapter: ")+std::string(desc.begin(),desc.end()));
  ComPtr<IDXGIFactory> factory;check(adapter->GetParent(IID_PPV_ARGS(&factory)));check(factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER));
  ComPtr<ID3D11Texture2D> back;check(swap->GetBuffer(0,IID_PPV_ARGS(&back)));check(device->CreateRenderTargetView(back.Get(),nullptr,&target));
  D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_D32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
  ComPtr<ID3D11Texture2D> db;check(device->CreateTexture2D(&td,nullptr,&db));check(device->CreateDepthStencilView(db.Get(),nullptr,&depth));
  auto v=compileShader("vs","vs_5_0"),p=compileShader("ps","ps_5_0");
  check(device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs));check(device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps));
  D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}};
  check(device->CreateInputLayout(input,2,v->GetBufferPointer(),v->GetBufferSize(),&layout));
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=capacity*sizeof(Vertex);bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
  check(device->CreateBuffer(&bd,nullptr,&vertices));bd.ByteWidth=80;bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device->CreateBuffer(&bd,nullptr,&constants));
  D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;check(device->CreateRasterizerState(&rd,&raster));
  D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_LESS;
  dd.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS};dd.BackFace=dd.FrontFace;
  check(device->CreateDepthStencilState(&dd,&worldDepth));dd.DepthEnable=FALSE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;check(device->CreateDepthStencilState(&dd,&hudDepth));
 }
 void camera(const XMMATRIX& matrix,float light){
  struct alignas(16) Camera{XMFLOAT4X4 m;float light;float pad[3];};Camera c{};XMStoreFloat4x4(&c.m,matrix);c.light=light;
  D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(constants.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));std::memcpy(mapped.pData,&c,sizeof(c));context->Unmap(constants.Get(),0);
  ID3D11Buffer* cb=constants.Get();context->VSSetConstantBuffers(0,1,&cb);
 }
 void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& view,float light,const std::filesystem::path& photo={})override{
  if(scene.size()+hud.size()>capacity)throw std::runtime_error("Vertex capacity exceeded");
  D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));
  std::memcpy(mapped.pData,scene.data(),scene.size()*sizeof(Vertex));std::memcpy(static_cast<char*>(mapped.pData)+scene.size()*sizeof(Vertex),hud.data(),hud.size()*sizeof(Vertex));context->Unmap(vertices.Get(),0);
  float clear[]={.06f+.32f*light,.08f+.5f*light,.14f+.65f*light,1};context->ClearRenderTargetView(target.Get(),clear);context->ClearDepthStencilView(depth.Get(),D3D11_CLEAR_DEPTH,1,0);
  auto* rt=target.Get();context->OMSetRenderTargets(1,&rt,depth.Get());
  D3D11_VIEWPORT vp{0,0,(float)width,(float)height,0,1};context->RSSetViewports(1,&vp);context->RSSetState(raster.Get());
  context->IASetInputLayout(layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  UINT stride=sizeof(Vertex),offset=0;auto* vb=vertices.Get();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
  context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);
  context->OMSetDepthStencilState(worldDepth.Get(),0);camera(view,light);context->Draw((UINT)scene.size(),0);
  context->OMSetDepthStencilState(hudDepth.Get(),0);camera(XMMatrixIdentity(),1);context->Draw((UINT)hud.size(),(UINT)scene.size());
  if(!photo.empty()){
   ComPtr<ID3D11Texture2D> back;check(swap->GetBuffer(0,IID_PPV_ARGS(&back)));
   if(!staging){D3D11_TEXTURE2D_DESC td{};back->GetDesc(&td);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;td.MiscFlags=0;check(device->CreateTexture2D(&td,nullptr,&staging));}
   context->CopyResource(staging.Get(),back.Get());check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
   try{writeBitmap(photo,width,height,static_cast<unsigned char*>(mapped.pData),mapped.RowPitch);}catch(...){context->Unmap(staging.Get(),0);throw;}
   context->Unmap(staging.Get(),0);
  }
  check(swap->Present(1,0),"DX11 Present");
 }
};
