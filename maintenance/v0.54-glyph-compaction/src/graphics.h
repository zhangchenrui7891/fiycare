#pragma once
#include "renderer12.h"
#include "renderer11.h"
#include "launch_options.h"
inline UINT renderScale=1;
inline std::unique_ptr<IRenderer> createRenderer(HWND window,const launch::Options& args){
 RECT client{};GetClientRect(window,&client);
 const UINT renderWidth=UINT(std::max(1L,client.right)),renderHeight=UINT(std::max(1L,client.bottom));
 const auto extent=launch::renderExtent(renderWidth,renderHeight,renderScale,args.has(L"--low"));
 bool force11=args.has(L"--dx11");
 bool force12=args.has(L"--dx12");
 bool software=args.has(L"--warp");
 if(!force11){
  try{auto r=std::make_unique<Renderer12>();r->width=extent.width;r->height=extent.height;r->init(window,software);logLine("Selected DX12");logShaderBytecodeCache();return r;}
  catch(const std::exception& e){logLine(std::string("DX12 initialization: ")+e.what());if(force12)throw;}
 }
 auto r=std::make_unique<Renderer11>();r->width=extent.width;r->height=extent.height;r->init(window,software);logLine(software?"Selected DX11 software (diagnostic only)":"Selected DX11 hardware compatibility mode");logShaderBytecodeCache();return r;
}
