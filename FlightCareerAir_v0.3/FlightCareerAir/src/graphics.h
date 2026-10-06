#pragma once
#include "renderer12.h"
#include "renderer11.h"
inline std::unique_ptr<IRenderer> createRenderer(HWND window,const std::wstring& args){
 bool force11=args.find(L"--dx11")!=std::wstring::npos;
 bool force12=args.find(L"--dx12")!=std::wstring::npos;
 bool software=args.find(L"--warp")!=std::wstring::npos;
 if(!force11){
  try{auto r=std::make_unique<Renderer12>();if(args.find(L"--low")!=std::wstring::npos){r->width=640;r->height=360;}r->init(window,software);logLine("Selected DX12");return r;}
  catch(const std::exception& e){logLine(std::string("DX12 initialization: ")+e.what());if(force12)throw;}
 }
 auto r=std::make_unique<Renderer11>();if(args.find(L"--low")!=std::wstring::npos){r->width=640;r->height=360;}r->init(window,software);logLine(software?"Selected DX11 software (diagnostic only)":"Selected DX11 hardware compatibility mode");return r;
}
