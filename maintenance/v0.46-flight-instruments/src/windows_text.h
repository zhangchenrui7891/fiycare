#pragma once
#include <windows.h>
#include <climits>
#include <stdexcept>
#include <string>
#include <string_view>

// Device names are UTF-16; logs are UTF-8, never byte-truncated wchar_t.
inline std::string windowsUtf8(std::wstring_view text){
 if(text.empty())return {};
 if(text.size()>static_cast<size_t>(INT_MAX))throw std::length_error("UTF-16 text exceeds Windows API range");
 const int length=static_cast<int>(text.size());
 const int bytes=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),length,nullptr,0,nullptr,nullptr);
 if(!bytes)return "[invalid UTF-16]";
 std::string result(static_cast<size_t>(bytes),'\0');
 if(WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.data(),length,result.data(),bytes,nullptr,nullptr)!=bytes)
  return "[UTF-8 conversion failed]";
 return result;
}
