#include "src/windows_text.h"
#include <iostream>
void require(bool value,const char* why){if(!value)throw std::runtime_error(why);}
int main(){
 try{
  require(windowsUtf8(L"").empty(),"Empty text");
  require(windowsUtf8(L"Intel HD Graphics 6000")=="Intel HD Graphics 6000","ASCII unchanged");
  require(windowsUtf8(L"中文显卡") == u8"中文显卡","Chinese device name is UTF-8");
  require(windowsUtf8(L"GPU \U0001F6EB") == u8"GPU \U0001F6EB","Supplementary Unicode");
  require(windowsUtf8(std::wstring_view(L"A\0中",3))==std::string("A\0",2)+u8"中","Explicit length preserves embedded NUL");
  require(windowsUtf8(std::wstring(1,static_cast<wchar_t>(0xD800)))=="[invalid UTF-16]","Invalid high surrogate gives explicit fallback");
  require(windowsUtf8(std::wstring(1,static_cast<wchar_t>(0xDC00)))=="[invalid UTF-16]","Invalid low surrogate gives explicit fallback");
  std::cout<<"PASS 7 actual Windows UTF-16/UTF-8 cases: ASCII, Chinese, non-BMP, NUL and invalid surrogate fallback. No GPU test.\n";
  return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
