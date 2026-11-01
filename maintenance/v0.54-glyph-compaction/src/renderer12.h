#pragma once
#include "graphics_common.h"
#include <d3d12.h>
#include "vertex_capacity.h"
#include <dxgi1_6.h>
struct Renderer12 final : IRenderer {
 HWND window{}; UINT width=960,height=540; UINT frame=0;UINT64 serial=0;HANDLE event{};
 ComPtr<ID3D12Device> device;ComPtr<ID3D12CommandQueue> queue;ComPtr<IDXGISwapChain3> swap;
 ComPtr<ID3D12DescriptorHeap> heap;ComPtr<ID3D12Resource> targets[2],depth,vertices,readback;
 D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{};UINT64 captureBytes=0;
 ComPtr<ID3D12DescriptorHeap> atlasHeap;ComPtr<ID3D12Resource> atlas;
 ComPtr<ID3D12DescriptorHeap> depthHeap;ComPtr<ID3D12CommandAllocator> allocator;
 ComPtr<ID3D12GraphicsCommandList> list;ComPtr<ID3D12Fence> fence;
 ComPtr<ID3D12RootSignature> root,postRoot;ComPtr<ID3D12Resource> sceneTexture;ComPtr<ID3D12PipelineState> postPipeline;ComPtr<ID3D12PipelineState> pipeline,hudPipeline;UINT stride=0;Vertex* mapped=nullptr;
 size_t capacity=320000;
 void wait(){check(queue->Signal(fence.Get(),++serial));if(fence->GetCompletedValue()<serial){check(fence->SetEventOnCompletion(serial,event));if(WaitForSingleObject(event,5000)!=WAIT_OBJECT_0)throw std::runtime_error("GPU fence timed out");}}
 const char* name()const override{return "DX12";}
 void init(HWND h,bool software)override{
  window=h;ComPtr<IDXGIFactory4> factory;check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
  ComPtr<IDXGIAdapter1> adapter;for(UINT i=0;!software&&factory->EnumAdapters1(i,&adapter)!=DXGI_ERROR_NOT_FOUND;i++){
   DXGI_ADAPTER_DESC1 d{};check(adapter->GetDesc1(&d));if(!(d.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)&&SUCCEEDED(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)))){std::wstring desc(d.Description);logLine(std::string("DX12 adapter: ")+windowsUtf8(desc));break;}adapter.Reset();
  }
  if(!device&&!software)throw std::runtime_error("DX12 hardware unavailable. Check Boot Camp graphics driver and dxdiag. Software test only: launch with --warp.");
  if(!device){ComPtr<IDXGIAdapter> warp;check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));check(D3D12CreateDevice(warp.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)));}
  D3D12_COMMAND_QUEUE_DESC q{};check(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)));
  DXGI_SWAP_CHAIN_DESC1 sd{};sd.Width=width;sd.Height=height;sd.Format=DXGI_FORMAT_R8G8B8A8_UNORM;sd.SampleDesc.Count=1;sd.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sd.BufferCount=2;sd.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
  ComPtr<IDXGISwapChain1> s;check(factory->CreateSwapChainForHwnd(queue.Get(),h,&sd,nullptr,nullptr,&s));check(s.As(&swap));check(factory->MakeWindowAssociation(h,DXGI_MWA_NO_ALT_ENTER));
  D3D12_DESCRIPTOR_HEAP_DESC hd{};hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_RTV;hd.NumDescriptors=3;check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&heap)));stride=device->GetDescriptorHandleIncrementSize(hd.Type);
  auto rt=heap->GetCPUDescriptorHandleForHeapStart();for(int i=0;i<2;i++){check(swap->GetBuffer(i,IID_PPV_ARGS(&targets[i])));device->CreateRenderTargetView(targets[i].Get(),nullptr,rt);rt.ptr+=stride;}
  hd.Type=D3D12_DESCRIPTOR_HEAP_TYPE_DSV;hd.NumDescriptors=1;check(device->CreateDescriptorHeap(&hd,IID_PPV_ARGS(&depthHeap)));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_DEFAULT;
  D3D12_RESOURCE_DESC dd{};dd.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;dd.Width=width;dd.Height=height;dd.DepthOrArraySize=1;dd.MipLevels=1;dd.Format=DXGI_FORMAT_D32_FLOAT;dd.SampleDesc.Count=1;dd.Flags=D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
  D3D12_CLEAR_VALUE dc{};dc.Format=dd.Format;dc.DepthStencil.Depth=presentation::depthClear;
  check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&dd,D3D12_RESOURCE_STATE_DEPTH_WRITE,&dc,IID_PPV_ARGS(&depth)));device->CreateDepthStencilView(depth.Get(),nullptr,depthHeap->GetCPUDescriptorHandleForHeapStart());
  auto colorDesc=dd;colorDesc.Format=sd.Format;colorDesc.Flags=D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;D3D12_CLEAR_VALUE cc{};cc.Format=sd.Format;check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&colorDesc,D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,&cc,IID_PPV_ARGS(&sceneTexture)));auto sceneRT=heap->GetCPUDescriptorHandleForHeapStart();sceneRT.ptr+=2*stride;device->CreateRenderTargetView(sceneTexture.Get(),nullptr,sceneRT);
  check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)));
  check(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),nullptr,IID_PPV_ARGS(&list)));check(list->Close());
  check(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)));event=CreateEvent(nullptr,FALSE,FALSE,nullptr);if(!event)throw std::runtime_error("Fence event creation failed");
  D3D12_ROOT_PARAMETER params[2]{};params[0].ParameterType=D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;params[0].Constants.Num32BitValues=60;params[0].ShaderVisibility=D3D12_SHADER_VISIBILITY_ALL;
  D3D12_DESCRIPTOR_RANGE srvRange{};srvRange.RangeType=D3D12_DESCRIPTOR_RANGE_TYPE_SRV;srvRange.NumDescriptors=1;srvRange.OffsetInDescriptorsFromTableStart=D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
  params[1].ParameterType=D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;params[1].DescriptorTable.NumDescriptorRanges=1;params[1].DescriptorTable.pDescriptorRanges=&srvRange;params[1].ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
  D3D12_STATIC_SAMPLER_DESC sm{};sm.Filter=D3D12_FILTER_ANISOTROPIC;sm.AddressU=sm.AddressV=sm.AddressW=D3D12_TEXTURE_ADDRESS_MODE_CLAMP;sm.MaxAnisotropy=16;sm.ComparisonFunc=D3D12_COMPARISON_FUNC_NEVER;sm.MaxLOD=D3D12_FLOAT32_MAX;sm.ShaderVisibility=D3D12_SHADER_VISIBILITY_PIXEL;
  D3D12_ROOT_SIGNATURE_DESC rd{};rd.NumParameters=2;rd.pParameters=params;rd.NumStaticSamplers=1;rd.pStaticSamplers=&sm;rd.Flags=D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
  ComPtr<ID3DBlob> blob,error;check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&root)));

  ComPtr<ID3DBlob> vs,ps;vs=compileShader("vs","vs_5_0");ps=compileShader("ps","ps_5_0");
  D3D12_INPUT_ELEMENT_DESC input[]={{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"COLOR",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"TEXCOORD",1,DXGI_FORMAT_R32_FLOAT,0,32,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,36,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pd{};pd.pRootSignature=root.Get();pd.VS={vs->GetBufferPointer(),vs->GetBufferSize()};pd.PS={ps->GetBufferPointer(),ps->GetBufferSize()};pd.InputLayout={input,5};pd.SampleMask=UINT_MAX;
  pd.RasterizerState.FillMode=D3D12_FILL_MODE_SOLID;pd.RasterizerState.CullMode=D3D12_CULL_MODE_NONE;pd.RasterizerState.DepthClipEnable=TRUE;
  auto& blend=pd.BlendState.RenderTarget[0];blend.SrcBlend=D3D12_BLEND_ONE;blend.DestBlend=D3D12_BLEND_ZERO;blend.BlendOp=D3D12_BLEND_OP_ADD;blend.SrcBlendAlpha=D3D12_BLEND_ONE;blend.DestBlendAlpha=D3D12_BLEND_ZERO;blend.BlendOpAlpha=D3D12_BLEND_OP_ADD;blend.LogicOp=D3D12_LOGIC_OP_NOOP;blend.RenderTargetWriteMask=D3D12_COLOR_WRITE_ENABLE_ALL;
  pd.DepthStencilState.DepthEnable=TRUE;pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ALL;pd.DepthStencilState.DepthFunc=D3D12_COMPARISON_FUNC_GREATER;pd.DepthStencilState.StencilReadMask=255;pd.DepthStencilState.StencilWriteMask=255;
  pd.DepthStencilState.FrontFace={D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_STENCIL_OP_KEEP,D3D12_COMPARISON_FUNC_ALWAYS};pd.DepthStencilState.BackFace=pd.DepthStencilState.FrontFace;
  pd.PrimitiveTopologyType=D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;pd.NumRenderTargets=1;pd.RTVFormats[0]=sd.Format;pd.DSVFormat=dd.Format;pd.SampleDesc.Count=1;
  check(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&pipeline)));
  pd.DepthStencilState.DepthEnable=FALSE;pd.DepthStencilState.DepthWriteMask=D3D12_DEPTH_WRITE_MASK_ZERO;check(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&hudPipeline)));
  sm.Filter=D3D12_FILTER_MIN_MAG_MIP_LINEAR;sm.MaxAnisotropy=1;params[0].Constants.Num32BitValues=4;check(D3D12SerializeRootSignature(&rd,D3D_ROOT_SIGNATURE_VERSION_1,&blob,&error));check(device->CreateRootSignature(0,blob->GetBufferPointer(),blob->GetBufferSize(),IID_PPV_ARGS(&postRoot)));auto pv=compilePost("postVS","vs_5_0"),pp=compilePost("postPS","ps_5_0");pd.pRootSignature=postRoot.Get();pd.VS={pv->GetBufferPointer(),pv->GetBufferSize()};pd.PS={pp->GetBufferPointer(),pp->GetBufferSize()};pd.InputLayout={nullptr,0};pd.DSVFormat=DXGI_FORMAT_UNKNOWN;check(device->CreateGraphicsPipelineState(&pd,IID_PPV_ARGS(&postPipeline)));
  const auto textures=buildMaterialTextures(loadAtlas(assetFolder()/L"atlas.fctex"));
  const UINT subresourceCount=static_cast<UINT>(textures.subresources.size());
  D3D12_RESOURCE_DESC at{};at.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;at.Width=textures.subresources[0].width;at.Height=textures.subresources[0].height;at.DepthOrArraySize=5;at.MipLevels=static_cast<UINT16>(textures.mipCount);at.Format=DXGI_FORMAT_R8G8B8A8_UNORM;at.SampleDesc.Count=1;
  check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&at,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&atlas)));
  std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> footprints(subresourceCount);UINT64 bytes=0;device->GetCopyableFootprints(&at,0,subresourceCount,0,footprints.data(),nullptr,nullptr,&bytes);
  D3D12_HEAP_PROPERTIES uploadHeap{};uploadHeap.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC buf{};buf.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buf.Width=bytes;buf.Height=1;buf.DepthOrArraySize=1;buf.MipLevels=1;buf.SampleDesc.Count=1;buf.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> upload;check(device->CreateCommittedResource(&uploadHeap,D3D12_HEAP_FLAG_NONE,&buf,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&upload)));unsigned char* pixels=nullptr;D3D12_RANGE nothing{0,0};check(upload->Map(0,&nothing,reinterpret_cast<void**>(&pixels)));
  for(UINT i=0;i<subresourceCount;i++){const auto& image=textures.subresources[i];const auto& footprint=footprints[i];for(UINT y=0;y<image.height;y++)std::memcpy(pixels+footprint.Offset+size_t(y)*footprint.Footprint.RowPitch,image.rgba.data()+size_t(y)*image.width*4,size_t(image.width)*4);}upload->Unmap(0,nullptr);
  check(allocator->Reset());check(list->Reset(allocator.Get(),nullptr));
  for(UINT i=0;i<subresourceCount;i++){D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=upload.Get();src.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src.PlacedFootprint=footprints[i];dst.pResource=atlas.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.SubresourceIndex=i;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);}
  D3D12_RESOURCE_BARRIER ab{};ab.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;ab.Transition.pResource=atlas.Get();ab.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;ab.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_DEST;ab.Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&ab);check(list->Close());ID3D12CommandList* cmd[]={list.Get()};queue->ExecuteCommandLists(1,cmd);wait();
  D3D12_DESCRIPTOR_HEAP_DESC ah{};ah.Type=D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;ah.NumDescriptors=2;ah.Flags=D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;check(device->CreateDescriptorHeap(&ah,IID_PPV_ARGS(&atlasHeap)));D3D12_SHADER_RESOURCE_VIEW_DESC av{};av.Format=at.Format;av.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2DARRAY;av.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;av.Texture2DArray.MipLevels=textures.mipCount;av.Texture2DArray.ArraySize=5;device->CreateShaderResourceView(atlas.Get(),&av,atlasHeap->GetCPUDescriptorHandleForHeapStart());
  auto sceneSRV=atlasHeap->GetCPUDescriptorHandleForHeapStart();sceneSRV.ptr+=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);D3D12_SHADER_RESOURCE_VIEW_DESC sceneView{};sceneView.Format=at.Format;sceneView.ViewDimension=D3D12_SRV_DIMENSION_TEXTURE2D;sceneView.Shader4ComponentMapping=D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;sceneView.Texture2D.MipLevels=1;device->CreateShaderResourceView(sceneTexture.Get(),&sceneView,sceneSRV);
  logLine("Material textures: 5 isolated slices, "+std::to_string(textures.mipCount)+" mip levels, 16x anisotropic filtering");
  hp.Type=D3D12_HEAP_TYPE_UPLOAD;D3D12_RESOURCE_DESC vd{};vd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;vd.Width=capacity*sizeof(Vertex);vd.Height=1;vd.DepthOrArraySize=1;vd.MipLevels=1;vd.SampleDesc.Count=1;vd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&vd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&vertices)));D3D12_RANGE range{0,0};check(vertices->Map(0,&range,reinterpret_cast<void**>(&mapped)));
 }
 void ensureVertexBuffer(size_t required){
  if(required<=capacity)return;
  const size_t grown=growVertexCapacity(capacity,required,sizeof(Vertex));
  D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_UPLOAD;
  D3D12_RESOURCE_DESC vd{};vd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;vd.Width=grown*sizeof(Vertex);vd.Height=1;vd.DepthOrArraySize=1;vd.MipLevels=1;vd.SampleDesc.Count=1;vd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  ComPtr<ID3D12Resource> larger;check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&vd,D3D12_RESOURCE_STATE_GENERIC_READ,nullptr,IID_PPV_ARGS(&larger)),"Grow DX12 vertex buffer");
  Vertex* next=nullptr;D3D12_RANGE range{0,0};check(larger->Map(0,&range,reinterpret_cast<void**>(&next)));
  wait();vertices->Unmap(0,nullptr);vertices=std::move(larger);mapped=next;capacity=grown;logLine("DX12 vertex capacity grew to "+std::to_string(capacity));
 }
 void draw(const std::vector<Vertex>& scene,const std::vector<Vertex>& hud,const XMMATRIX& matrix,float daylight,const std::filesystem::path& photo={},const std::vector<Vertex>& cabin={},const XMMATRIX& cabinView=XMMatrixIdentity())override{
  const presentation::VertexBatch<Vertex> batch(skyVertices,scene,cabin,hud);
  ensureVertexBuffer(batch.layout.total);
  batch.copyTo(mapped,capacity);check(allocator->Reset());check(list->Reset(allocator.Get(),pipeline.Get()));frame=swap->GetCurrentBackBufferIndex();
  D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=targets[frame].Get();b.Transition.Subresource=D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;b.Transition.StateBefore=D3D12_RESOURCE_STATE_PRESENT;b.Transition.StateAfter=D3D12_RESOURCE_STATE_RENDER_TARGET;list->ResourceBarrier(1,&b);
  auto swapRT=heap->GetCPUDescriptorHandleForHeapStart();swapRT.ptr+=frame*stride;auto rt=heap->GetCPUDescriptorHandleForHeapStart();rt.ptr+=2*stride;auto sceneBarrier=b;sceneBarrier.Transition.pResource=sceneTexture.Get();sceneBarrier.Transition.StateBefore=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&sceneBarrier);auto ds=depthHeap->GetCPUDescriptorHandleForHeapStart();float clear[]={0.06f+0.32f*daylight,0.08f+0.5f*daylight,0.14f+0.65f*daylight,1};list->ClearRenderTargetView(rt,clear,0,nullptr);list->ClearDepthStencilView(ds,D3D12_CLEAR_FLAG_DEPTH,presentation::depthClear,0,0,nullptr);list->OMSetRenderTargets(1,&rt,FALSE,&ds);
  D3D12_VIEWPORT viewport{0,0,(float)width,(float)height,0,1};D3D12_RECT rect{0,0,(LONG)width,(LONG)height};list->RSSetViewports(1,&viewport);list->RSSetScissorRects(1,&rect);list->SetGraphicsRootSignature(root.Get());ID3D12DescriptorHeap* heaps[]={atlasHeap.Get()};list->SetDescriptorHeaps(1,heaps);list->SetGraphicsRootDescriptorTable(1,atlasHeap->GetGPUDescriptorHandleForHeapStart());auto camera=cameraData(XMMatrixIdentity(),2);list->SetGraphicsRoot32BitConstants(0,60,&camera,0);
  D3D12_VERTEX_BUFFER_VIEW vb{vertices->GetGPUVirtualAddress(),(UINT)batch.bytes(),sizeof(Vertex)};list->IASetVertexBuffers(0,1,&vb);list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  list->SetPipelineState(hudPipeline.Get());list->DrawInstanced(6,1,0,0);
  list->SetPipelineState(pipeline.Get());camera=cameraData(matrix,0);list->SetGraphicsRoot32BitConstants(0,60,&camera,0);list->DrawInstanced((UINT)scene.size(),1,(UINT)batch.layout.scene,0);
  if(!cabin.empty()){list->ClearDepthStencilView(ds,D3D12_CLEAR_FLAG_DEPTH,presentation::depthClear,0,0,nullptr);camera=cameraData(cabinView,3);list->SetGraphicsRoot32BitConstants(0,60,&camera,0);list->DrawInstanced((UINT)cabin.size(),1,(UINT)batch.layout.cabin,0);}
  sceneBarrier.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;sceneBarrier.Transition.StateAfter=D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;list->ResourceBarrier(1,&sceneBarrier);list->OMSetRenderTargets(1,&swapRT,FALSE,nullptr);list->SetGraphicsRootSignature(postRoot.Get());list->SetPipelineState(postPipeline.Get());auto postSRV=atlasHeap->GetGPUDescriptorHandleForHeapStart();postSRV.ptr+=device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);list->SetGraphicsRootDescriptorTable(1,postSRV);float post[]={1.f/width,1.f/height,antiAliasEnabled?1.f:0.f,0};list->SetGraphicsRoot32BitConstants(0,4,post,0);list->DrawInstanced(3,1,0,0);
  list->OMSetRenderTargets(1,&swapRT,FALSE,&ds);list->SetGraphicsRootSignature(root.Get());list->SetGraphicsRootDescriptorTable(1,atlasHeap->GetGPUDescriptorHandleForHeapStart());list->SetPipelineState(hudPipeline.Get());camera=cameraData(XMMatrixIdentity(),1);list->SetGraphicsRoot32BitConstants(0,60,&camera,0);list->DrawInstanced((UINT)hud.size(),1,(UINT)batch.layout.hud,0);
  if(!photo.empty()){
   if(!readback){auto desc=targets[frame]->GetDesc();device->GetCopyableFootprints(&desc,0,1,0,&footprint,nullptr,nullptr,&captureBytes);
    D3D12_HEAP_PROPERTIES hp{};hp.Type=D3D12_HEAP_TYPE_READBACK;D3D12_RESOURCE_DESC rd{};rd.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;rd.Width=captureBytes;rd.Height=1;rd.DepthOrArraySize=1;rd.MipLevels=1;rd.SampleDesc.Count=1;rd.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    check(device->CreateCommittedResource(&hp,D3D12_HEAP_FLAG_NONE,&rd,D3D12_RESOURCE_STATE_COPY_DEST,nullptr,IID_PPV_ARGS(&readback)));}
   b.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;b.Transition.StateAfter=D3D12_RESOURCE_STATE_COPY_SOURCE;list->ResourceBarrier(1,&b);
   D3D12_TEXTURE_COPY_LOCATION src{},dst{};src.pResource=targets[frame].Get();src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;dst.pResource=readback.Get();dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=footprint;list->CopyTextureRegion(&dst,0,0,0,&src,nullptr);
   b.Transition.StateBefore=D3D12_RESOURCE_STATE_COPY_SOURCE;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;
  }else{b.Transition.StateBefore=D3D12_RESOURCE_STATE_RENDER_TARGET;b.Transition.StateAfter=D3D12_RESOURCE_STATE_PRESENT;}
  list->ResourceBarrier(1,&b);check(list->Close());ID3D12CommandList* commands[]={list.Get()};queue->ExecuteCommandLists(1,commands);check(swap->Present(0,0));wait();
  if(!photo.empty()){
   unsigned char* pixels=nullptr;D3D12_RANGE range{0,(SIZE_T)captureBytes};check(readback->Map(0,&range,reinterpret_cast<void**>(&pixels)));
   try{writeBitmap(photo,width,height,pixels+footprint.Offset,footprint.Footprint.RowPitch);}catch(...){readback->Unmap(0,nullptr);throw;}
   D3D12_RANGE written{0,0};readback->Unmap(0,&written);
  }
 }
 ~Renderer12()override{if(queue&&fence&&event){queue->Signal(fence.Get(),++serial);if(fence->GetCompletedValue()<serial&&SUCCEEDED(fence->SetEventOnCompletion(serial,event)))WaitForSingleObject(event,2000);}if(event)CloseHandle(event);}
};
