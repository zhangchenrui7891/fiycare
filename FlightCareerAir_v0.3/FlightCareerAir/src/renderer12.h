#pragma once
#include "graphics_common.h"
#include <d3d12.h>
#include <dxgi1_6.h>
struct Renderer12 final : IRenderer {
 HWND window{}; UINT width=960,height=540; UINT frame=0;UINT64 serial=0;HANDLE event{};
 ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<IDXGISwapChain3> swap;
 ComPtr<ID3D12DescriptorHeap> heap;ComPtr<ID3D12Resource> targets[2],depth,vertices,readback;
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 captureBytes=0;
 ComPtr<ID3D12DescriptorHeap> depthHeap;ComPtr<ID3D12CommandAllocator> allocator;
 ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Fence> fence;
 ComPtr<ID3D12RootSignature> root;ComPtr<ID3D12PipelineState> pipeline,hudPipeline;UINT stride=0;Vertex* mapped=nullptr;
 static constexpr size_t capacity=160000;
 void wait(){check(queue->Signal(fence.Get(),++serial));if(fence->GetCompletedValue()<serial){check(fence->SetEventOnCompletion(serial,event));if(WaitForSingleObject(event,5000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU fence timed out");}}
 const char* name()const override{return "DX12";}
 void init(HWND h,bool software)override{
  window=h;ComPtr<IDXGIFactory4> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
  ComPtr<IDXGIAdapter1> adapter;for(UINT i=0;!software&&factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;i++){
   DXGI_ADAPTER_DESC1 d{};check(adapter->GetDesc1(&d));if(!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){std::wstring desc(d.Description);logLine(std::string("DX12 adapter: ")+std::string(desc.begin(),desc.end()));break;}adapter.Reset();
  }
  if(!device&&!software)throw std::runtime_error("DX12 hardware unavailable. Check Boot Camp graphics driver and dxdiag. Software test only: launch with --warp.");
  if(!device){ComPtr<IDXGIAdapter> warp;check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));}
  D3D12_COMMAND_QUEUE_DESC q{};check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
  DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=width;sd.Height=height;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
  ComPtr<IDXGISwapChain1> s;check(factory->CreateSwapChainForHwnd(queue.Get(),h,&sd,nullptr,nullptr,&s));check(s.As(&swap));check(factory->MakeWindowAssociation(h,DXGI_MWA_NO_ALT_ENTER));
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=2;check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));stride=device->GetDescriptorHandleIncrementSize(hd.Type);
  auto rt=heap->GetCPUDescriptorHandleForHeapStart();for(int i=0;i<2;i++){check(swap->GetBuffer(i,IID_PPV_ARGS(&targets[i])));device->CreateRenderTargetView(targets[i].Get(),nullptr,rt);rt.ptr+=stride;}
  hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;hd.NumDescriptors=1;check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&depthHeap)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC dd{};dd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;dd.Width=width;dd.Height=height;dd.DepthOrArraySize=1;dd.MipLevels=1;dd.Format=DXGI_FORMAT_D32_FLOAT;dd.SampleDesc.Count=1;dd.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  D3D12_CLEAR_VALUE dc{};dc.Format=dd.Format;dc.DepthStencil.Depth=1;
  check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&dd,D3D12_RESOURCE_STATE_DEPTH_WRITE,&dc,IID_PPV_ARGS(&depth)));device->CreateDepthStencilView(depth.Get(),nullptr,depthHeap->GetCPUDescriptorHandleForHeapStart());
  check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
  check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));check(list->Close());
  check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));event=CreateEvent(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("Fence event creation failed");
  D3D12_ROOT_PARAMETER param{};param.ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;param.Constants.Num32BitValues=17;param.ShaderVisibility=D3D12_SHADER_VISIBILITY_VERTEX;
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=1;rd.pParameters=&param;rd.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  ComPtr<ID3DBlob> blob,error;check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));

  ComPtr<ID3DBlob> vs,ps;vs=compileShader("vs","vs_5_0");ps=compileShader("ps","ps_5_0");
  D3D12_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pd.PS={ps->GetBufferPointer(),ps->GetBufferSize()};pd.InputLayout={input,2};pd.SampleMask=UINT_MAX;
  pd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pd.RasterizerState.DepthClipEnable=TRUE;
  auto& blend=pd.BlendState.RenderTarget[0];blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_ZERO;blend.BlendOp=D3D12_BLEND_OP_ADD;blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
  pd.DepthStencilState.DepthEnable=TRUE;pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pd.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_LESS;pd.DepthStencilState.StencilReadMask=255;pd.DepthStencilState.StencilWriteMask=255;
  pd.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};pd.DepthStencilState.BackFace=pd.DepthStencilState.FrontFace;
  pd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pd.NumRenderTargets=1;pd.RTVFormats[0]=sd.Format;pd.DSVFormat=dd.Format;pd.SampleDesc.Count=1;
  check(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pipeline)));
  pd.DepthStencilState.DepthEnable=FALSE;pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;check(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&hudPipeline)));
  hp.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC vd{};vd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;vd.Width=capacity*sizeof(Vertex);vd.Height=1;vd.DepthOrArraySize=1;vd.MipLevels=1;vd.SampleDesc.Count=1;vd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&vd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices)));D3D12_RANGE range{0,0};check(vertices->Map(0,&range,reinterpret_cast<void**>(&mapped)));
 }
 void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& matrix,float daylight,const std::filesystem::path& photo={})override{
  std::vector<Vertex> v=scene;v.insert(v.end(),hud.begin(),hud.end());
  if(v.size()>capacity)throw std::runtime_error("Vertex capacity exceeded");
  memcpy(mapped,v.data(),v.size()*sizeof(Vertex));check(allocator->Reset());check(list->Reset(allocator.Get(),pipeline.Get()));frame=swap->GetCurrentBackBufferIndex();
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=targets[frame].Get();b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;b.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;list->ResourceBarrier(1,&b);
  auto rt=heap->GetCPUDescriptorHandleForHeapStart();rt.ptr+=frame*stride;auto ds=depthHeap->GetCPUDescriptorHandleForHeapStart();float clear[]={0.06f+0.32f*daylight,0.08f+0.5f*daylight,0.14f+0.65f*daylight,1};list->ClearRenderTargetView(rt,clear,0,nullptr);list->ClearDepthStencilView(ds,D3D12_CLEAR_FLAG_DEPTH,1,0,0,nullptr);list->OMSetRenderTargets(1,&rt,FALSE,&ds);
  D3D12_VIEWPORT viewport{0,0,(float)width,(float)height,0,1};D3D12_RECT rect{0,0,(LONG)width,(LONG)height};list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&rect);list->SetGraphicsRootSignature(root.Get());XMFLOAT4X4 m;XMStoreFloat4x4(&m,matrix);list->SetGraphicsRoot32BitConstants(0,16,&m,0);list->SetGraphicsRoot32BitConstants(0,1,&daylight,16);
  D3D12_VERTEX_BUFFER_VIEW vb{vertices->GetGPUVirtualAddress(),(UINT)(v.size()*sizeof(Vertex)),sizeof(Vertex)};list->IASetVertexBuffers(0,1,&vb);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);list->DrawInstanced((UINT)scene.size(),1,0,0);
  list->SetPipelineState(hudPipeline.Get());XMStoreFloat4x4(&m,XMMatrixIdentity());list->SetGraphicsRoot32BitConstants(0,16,&m,0);float full=1;list->SetGraphicsRoot32BitConstants(0,1,&full,16);list->DrawInstanced((UINT)hud.size(),1,(UINT)scene.size(),0);
  if(!photo.empty()){
   if(!readback){auto desc=targets[frame]->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&captureBytes);
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=captureBytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));}
   b.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;list->ResourceBarrier(1,&b);
   D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=targets[frame].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
   b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;
  }else{b.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;}
  list->ResourceBarrier(1,&b);check(list->Close());ID3D12CommandList* commands[]={list.Get()};queue->ExecuteCommandLists(1,commands);check(swap->Present(1,0));wait();
  if(!photo.empty()){
   unsigned char* pixels=nullptr;D3D12_RANGE range{0,(SIZE_T)captureBytes};check(readback->Map(0,&range,reinterpret_cast<void**>(&pixels)));
   try{writeBitmap(photo,width,height,pixels+footprint.Offset,footprint.Footprint.RowPitch);}catch(...){readback->Unmap(0,nullptr);throw;}
   D3D12_RANGE written{0,0};readback->Unmap(0,&written);
  }
 }
 ~Renderer12()override{if(queue&&fence&&event){queue->Signal(fence.Get(),++serial);if(fence->GetCompletedValue()<serial&&SUCCEEDED(fence->SetEventOnCompletion(serial,event)))WaitForSingleObject(event,2000);}if(event)CloseHandle(event);}
};
