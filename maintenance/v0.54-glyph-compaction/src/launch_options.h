#pragma once
#include <algorithm>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace launch {
// Already-tokenized arguments, excluding the executable name.
class Options {
 std::vector<std::wstring> values;
public:
 explicit Options(std::vector<std::wstring> arguments={}):values(std::move(arguments)){}
 bool has(std::wstring_view option)const{
  return std::any_of(values.begin(),values.end(),[option](const auto& value){return value==option;});
 }
 Options with(std::wstring option)const{
  Options result=*this;
  if(!result.has(option))result.values.push_back(std::move(option));
  return result;
 }
};
// Native resolution is the low-overhead default; supersampling is opt-in.
// An explicit native override wins independent of argument order.
inline unsigned initialRenderScale(const Options& options){
 return options.has(L"--supersample")&&!options.has(L"--native")?2u:1u;
}
struct RenderExtent {unsigned width,height;};
// Shared by renderer creation and CSV metadata; scale is the applied scale,
// not a menu request that has yet to recreate the renderer.
inline RenderExtent renderExtent(unsigned width,unsigned height,unsigned scale,bool low){
 return low?RenderExtent{640,360}:RenderExtent{std::max(1u,width)*scale,std::max(1u,height)*scale};
}
}
