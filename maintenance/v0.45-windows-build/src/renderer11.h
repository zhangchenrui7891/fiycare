#pragma once
#include "graphics_common.h"
#include <d3d11.h>
#include "vertex_capacity.h"
#include <dxgi1_2.h>
struct Renderer11 final : IRenderer {
 size_t capacity=320000;
 UINT width=960,height=540;
 ComPtr<ID3D11Device> device;
 ComPtr<ID3D11DeviceContext> context;
 ComPtr<IDXGISwapChain> swap;
 ComPtr<ID3D11RenderTargetView> target,sceneTarget;ComPtr<ID3D11Texture2D> sceneTexture;ComPtr<ID3D11ShaderResourceView> sceneView;ComPtr<ID3D11VertexShader> postVS;ComPtr<ID3D11PixelShader> postPS;ComPtr<ID3D11Buffer> postConstants;
 ComPtr<ID3D11DepthStencilView> depth;
 ComPtr<ID3D11Buffer> vertices,constants;
 ComPtr<ID3D11VertexShader> vs;
 ComPtr<ID3D11PixelShader> ps;
 ComPtr<ID3D11InputLayout> layout;
 ComPtr<ID3D11RasterizerState> raster;
 ComPtr<ID3D11DepthStencilState> worldDepth,hudDepth;
 ComPtr<ID3D11Texture2D> staging,atlas;ComPtr<ID3D11ShaderResourceView> atlasView;ComPtr<ID3D11SamplerState> sampler,postSampler;
 const char* name()const override{return "DX11";}
 void init(HWND window,bool software)override{
  DXGI_SWAP_CHAIN_DESC sd{};sd.BufferDesc.Width=width;sd.BufferDesc.Height=height;sd.BufferDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.OutputWindow=window;sd.Windowed=TRUE;sd.SwapEffect=DXGI_SWAP_EFFECT_DISCARD;
  D3D_FEATURE_LEVEL levels[]={D3D_FEATURE_LEVEL_11_0};D3D_FEATURE_LEVEL obtained{};
  check(D3D11CreateDeviceAndSwapChain(nullptr,software?D3D_DRIVER_TYPE_WARP:D3D_DRIVER_TYPE_HARDWARE,nullptr,0,levels,1,D3D11_SDK_VERSION,&sd,&swap,&device,&obtained,&context),"D3D11CreateDeviceAndSwapChain");
  ComPtr<IDXGIDevice> dxgi;check(device.As(&dxgi));ComPtr<IDXGIAdapter> adapter;check(dxgi->GetAdapter(&adapter));DXGI_ADAPTER_DESC ad{};check(adapter->GetDesc(&ad));
  std::wstring desc(ad.Description);logLine(std::string("DX11 adapter: ")+windowsUtf8(desc));
  ComPtr<IDXGIFactory> factory;check(adapter->GetParent(IID_PPV_ARGS(&factory)));check(factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER));
  ComPtr<ID3D11Texture2D> back;check(swap->GetBuffer(0,IID_PPV_ARGS(&back)));check(device->CreateRenderTargetView(back.Get(),nullptr,&target));
  D3D11_TEXTURE2D_DESC td{};td.Width=width;td.Height=height;td.MipLevels=1;td.ArraySize=1;td.Format=DXGI_FORMAT_D32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;
  ComPtr<ID3D11Texture2D> db;check(device->CreateTexture2D(&td,nullptr,&db));check(device->CreateDepthStencilView(db.Get(),nullptr,&depth));
  td.Format=DXGI_FORMAT_R8G8B8A8_UNORM;td.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;check(device->CreateTexture2D(&td,nullptr,&sceneTexture));check(device->CreateRenderTargetView(sceneTexture.Get(),nullptr,&sceneTarget));check(device->CreateShaderResourceView(sceneTexture.Get(),nullptr,&sceneView));
  auto pv=compilePost("postVS","vs_5_0"),pp=compilePost("postPS","ps_5_0");check(device->CreateVertexShader(pv->GetBufferPointer(),pv->GetBufferSize(),nullptr,&postVS));check(device->CreatePixelShader(pp->GetBufferPointer(),pp->GetBufferSize(),nullptr,&postPS));D3D11_BUFFER_DESC pb{};pb.ByteWidth=16;pb.Usage=D3D11_USAGE_DYNAMIC;pb.BindFlags=D3D11_BIND_CONSTANT_BUFFER;pb.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;check(device->CreateBuffer(&pb,nullptr,&postConstants));
  auto v=compileShader("vs","vs_5_0"),p=compileShader("ps","ps_5_0");
  check(device->CreateVertexShader(v->GetBufferPointer(),v->GetBufferSize(),nullptr,&vs));check(device->CreatePixelShader(p->GetBufferPointer(),p->GetBufferSize(),nullptr,&ps));
  D3D11_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32_FLOAT,0,32,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,36,D3D11_INPUT_PER_VERTEX_DATA,0}};
  check(device->CreateInputLayout(input,5,v->GetBufferPointer(),v->GetBufferSize(),&layout));
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=static_cast<UINT>(capacity*sizeof(Vertex));bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
  check(device->CreateBuffer(&bd,nullptr,&vertices));bd.ByteWidth=sizeof(CameraData);bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;check(device->CreateBuffer(&bd,nullptr,&constants));
  D3D11_RASTERIZER_DESC rd{};rd.FillMode=D3D11_FILL_SOLID;rd.CullMode=D3D11_CULL_NONE;rd.DepthClipEnable=TRUE;check(device->CreateRasterizerState(&rd,&raster));
  D3D11_DEPTH_STENCIL_DESC dd{};dd.DepthEnable=TRUE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;dd.DepthFunc=D3D11_COMPARISON_GREATER;
  dd.FrontFace={D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_STENCIL_OP_KEEP,D3D11_COMPARISON_ALWAYS};dd.BackFace=dd.FrontFace;
  const auto textures=buildMaterialTextures(loadAtlas(assetFolder()/L"atlas.fctex"));
  D3D11_TEXTURE2D_DESC at{};at.Width=textures.subresources[0].width;at.Height=textures.subresources[0].height;at.MipLevels=textures.mipCount;at.ArraySize=5;at.Format=DXGI_FORMAT_R8G8B8A8_UNORM;at.SampleDesc.Count=1;at.Usage=D3D11_USAGE_IMMUTABLE;at.BindFlags=D3D11_BIND_SHADER_RESOURCE;
  std::vector<D3D11_SUBRESOURCE_DATA> data(textures.subresources.size());
  for(size_t i=0;i<data.size();i++){data[i].pSysMem=textures.subresources[i].rgba.data();data[i].SysMemPitch=textures.subresources[i].width*4;}
  check(device->CreateTexture2D(&at,data.data(),&atlas));check(device->CreateShaderResourceView(atlas.Get(),nullptr,&atlasView));
  D3D11_SAMPLER_DESC sm{};sm.Filter=D3D11_FILTER_ANISOTROPIC;sm.AddressU=sm.AddressV=sm.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;sm.MaxAnisotropy=16;sm.ComparisonFunc=D3D11_COMPARISON_NEVER;sm.MaxLOD=D3D11_FLOAT32_MAX;check(device->CreateSamplerState(&sm,&sampler));
  sm.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;sm.MaxAnisotropy=1;check(device->CreateSamplerState(&sm,&postSampler));
  logLine("Material textures: 5 isolated slices, "+std::to_string(textures.mipCount)+" mip levels, 16x anisotropic filtering");
  check(device->CreateDepthStencilState(&dd,&worldDepth));dd.DepthEnable=FALSE;dd.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ZERO;check(device->CreateDepthStencilState(&dd,&hudDepth));
 }
 void ensureVertexBuffer(size_t required){
  if(required<=capacity)return;
  const size_t grown=growVertexCapacity(capacity,required,sizeof(Vertex));
  D3D11_BUFFER_DESC bd{};bd.ByteWidth=static_cast<UINT>(grown*sizeof(Vertex));bd.Usage=D3D11_USAGE_DYNAMIC;bd.BindFlags=D3D11_BIND_VERTEX_BUFFER;bd.CPUAccessFlags=D3D11_CPU_ACCESS_WRITE;
  ComPtr<ID3D11Buffer> larger;check(device->CreateBuffer(&bd,nullptr,&larger),"Grow DX11 vertex buffer");
  vertices=std::move(larger);capacity=grown;logLine("DX11 vertex capacity grew to "+std::to_string(capacity));
 }
 void camera(const XMMATRIX& matrix,float mode){
  auto c=cameraData(matrix,mode);D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(constants.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));std::memcpy(mapped.pData,&c,sizeof(c));context->Unmap(constants.Get(),0);
  ID3D11Buffer* cb=constants.Get();context->VSSetConstantBuffers(0,1,&cb);context->PSSetConstantBuffers(0,1,&cb);
 }
 void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& view,float light,const std::filesystem::path& photo={},const std::vector<Vertex>& cabin={},const XMMATRIX& cabinView=XMMatrixIdentity())override{
  const presentation::VertexBatch<Vertex> batch(skyVertices,scene,cabin,hud);
  ensureVertexBuffer(batch.layout.total);
  D3D11_MAPPED_SUBRESOURCE mapped{};check(context->Map(vertices.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));
  batch.copyTo(mapped.pData,capacity);context->Unmap(vertices.Get(),0);
  float clear[]={.06f+.32f*light,.08f+.5f*light,.14f+.65f*light,1};context->ClearRenderTargetView(sceneTarget.Get(),clear);context->ClearDepthStencilView(depth.Get(),D3D11_CLEAR_DEPTH,presentation::depthClear,0);
  ID3D11ShaderResourceView* empty=nullptr;context->PSSetShaderResources(0,1,&empty);auto* rt=sceneTarget.Get();context->OMSetRenderTargets(1,&rt,depth.Get());
  D3D11_VIEWPORT vp{0,0,(float)width,(float)height,0,1};context->RSSetViewports(1,&vp);context->RSSetState(raster.Get());
  context->IASetInputLayout(layout.Get());context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  UINT stride=sizeof(Vertex),offset=0;auto* vb=vertices.Get();context->IASetVertexBuffers(0,1,&vb,&stride,&offset);
  context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);auto* srv=atlasView.Get();auto* sm=sampler.Get();context->PSSetShaderResources(0,1,&srv);context->PSSetSamplers(0,1,&sm);
  context->OMSetDepthStencilState(hudDepth.Get(),0);camera(XMMatrixIdentity(),2);context->Draw(6,0);
  context->OMSetDepthStencilState(worldDepth.Get(),0);camera(view,0);context->Draw((UINT)scene.size(),(UINT)batch.layout.scene);
  if(!cabin.empty()){context->ClearDepthStencilView(depth.Get(),D3D11_CLEAR_DEPTH,presentation::depthClear,0);camera(cabinView,3);context->Draw((UINT)cabin.size(),(UINT)batch.layout.cabin);}
  rt=target.Get();context->OMSetRenderTargets(1,&rt,nullptr);context->OMSetDepthStencilState(hudDepth.Get(),0);context->IASetInputLayout(nullptr);context->VSSetShader(postVS.Get(),nullptr,0);context->PSSetShader(postPS.Get(),nullptr,0);srv=sceneView.Get();context->PSSetShaderResources(0,1,&srv);auto* postSm=postSampler.Get();context->PSSetSamplers(0,1,&postSm);
  float post[]={1.f/width,1.f/height,antiAliasEnabled?1.f:0.f,0};check(context->Map(postConstants.Get(),0,D3D11_MAP_WRITE_DISCARD,0,&mapped));std::memcpy(mapped.pData,post,sizeof(post));context->Unmap(postConstants.Get(),0);auto* pcb=postConstants.Get();context->PSSetConstantBuffers(0,1,&pcb);context->Draw(3,0);
  context->PSSetShaderResources(0,1,&empty);context->IASetInputLayout(layout.Get());context->VSSetShader(vs.Get(),nullptr,0);context->PSSetShader(ps.Get(),nullptr,0);srv=atlasView.Get();context->PSSetShaderResources(0,1,&srv);context->PSSetSamplers(0,1,&sm);
  context->OMSetDepthStencilState(hudDepth.Get(),0);camera(XMMatrixIdentity(),1);context->Draw((UINT)hud.size(),(UINT)batch.layout.hud);
  if(!photo.empty()){
   ComPtr<ID3D11Texture2D> back;check(swap->GetBuffer(0,IID_PPV_ARGS(&back)));
   if(!staging){D3D11_TEXTURE2D_DESC td{};back->GetDesc(&td);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;td.MiscFlags=0;check(device->CreateTexture2D(&td,nullptr,&staging));}
   context->CopyResource(staging.Get(),back.Get());check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped));
   try{writeBitmap(photo,width,height,static_cast<unsigned char*>(mapped.pData),mapped.RowPitch);}catch(...){context->Unmap(staging.Get(),0);throw;}
   context->Unmap(staging.Get(),0);
  }
  check(swap->Present(0,0),"DX11 Present");
 }
};
