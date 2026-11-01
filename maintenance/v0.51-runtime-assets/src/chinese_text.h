#pragma once
#include "graphics_common.h"
#include <map>
#include <array>
struct PixelRun{float x,y,w,h;};
inline void screenQuad(std::vector<Vertex>& out,float x,float y,float w,float h,XMFLOAT3 color){
 const float l=x/480-1,r=(x+w)/480-1,t=1-y/270,b=1-(y+h)/270;
 const Vertex a{l,t,0,color.x,color.y,color.z},c{r,t,0,color.x,color.y,color.z},d{l,b,0,color.x,color.y,color.z},e{r,b,0,color.x,color.y,color.z};
 out.insert(out.end(),{a,d,c,c,d,e});
}
struct ChineseFont {
 struct Glyph{int advance=16;std::vector<PixelRun> runs;};
 HDC dc{};HFONT font{};HGDIOBJ old{};TEXTMETRICW metrics{};std::map<wchar_t,Glyph> cache;
 ChineseFont(){
  dc=CreateCompatibleDC(nullptr);if(!dc)throw std::runtime_error("无法创建中文字体绘制环境");
  font=CreateFontW(-16,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_TT_ONLY_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Microsoft YaHei");
  if(!font){DeleteDC(dc);dc=nullptr;throw std::runtime_error("无法创建中文字体");}
  old=SelectObject(dc,font);GetTextMetricsW(dc,&metrics);
 }
 ~ChineseFont(){if(dc){SelectObject(dc,old);DeleteObject(font);DeleteDC(dc);}}
 ChineseFont(const ChineseFont&)=delete;ChineseFont& operator=(const ChineseFont&)=delete;
 const Glyph& get(wchar_t c){
  auto found=cache.find(c);if(found!=cache.end())return found->second;
  Glyph g;SIZE size{};GetTextExtentPoint32W(dc,&c,1,&size);g.advance=std::max(4,(int)size.cx);
  if(c==L' ')return cache.emplace(c,std::move(g)).first->second;
  MAT2 identity{};identity.eM11.value=identity.eM22.value=1;GLYPHMETRICS gm{};
  DWORD bytes=GetGlyphOutlineW(dc,c,GGO_GRAY8_BITMAP,&gm,0,nullptr,&identity);
  if(bytes!=GDI_ERROR&&bytes>0){
   std::vector<unsigned char> data(bytes);
   if(GetGlyphOutlineW(dc,c,GGO_GRAY8_BITMAP,&gm,bytes,data.data(),&identity)!=GDI_ERROR){
    const UINT stride=(gm.gmBlackBoxX+3)&~3u;g.advance=std::max(4,(int)gm.gmCellIncX);
    for(UINT row=0;row<gm.gmBlackBoxY;row++){
     UINT col=0;while(col<gm.gmBlackBoxX){while(col<gm.gmBlackBoxX&&data[row*stride+col]<24)col++;UINT start=col;while(col<gm.gmBlackBoxX&&data[row*stride+col]>=24)col++;
      if(col>start)g.runs.push_back({(float)(gm.gmptGlyphOrigin.x+(int)start),(float)(metrics.tmAscent-gm.gmptGlyphOrigin.y+(int)row),(float)(col-start),1});
     }
    }
   }
  }
  // Visible replacement if a requested character is unavailable in the selected system font.
  if(g.runs.empty()&&c!=L' '){g.runs={{1,2,12,1},{1,14,12,1},{1,2,1,13},{12,2,1,13}};g.advance=16;}
  return cache.emplace(c,std::move(g)).first->second;
 }
 void draw(std::vector<Vertex>& out,const std::wstring& text,float x,float y,XMFLOAT3 color={.9f,.95f,.85f},float maxWidth=920){
  const float origin=x;
  for(wchar_t c:text){
   if(c==L'\n'){x=origin;y+=21;continue;}
   const auto& glyph=get(c);if(x+glyph.advance>origin+maxWidth){x=origin;y+=21;}
   for(const auto& run:glyph.runs)screenQuad(out,x+run.x,y+run.y,run.w,run.h,color);
   x+=glyph.advance;
  }
 }
};
inline ChineseFont& chineseFont(){static ChineseFont font;return font;}
inline std::wstring wideUtf8(const std::string& text){
 if(text.empty())return {};
 int count=MultiByteToWideChar(CP_UTF8,0,text.data(),(int)text.size(),nullptr,0);
 if(count<=0)return L"详细信息见日志";
 std::wstring result(count,L'\0');MultiByteToWideChar(CP_UTF8,0,text.data(),(int)text.size(),result.data(),count);return result;
}
inline void hudText(std::vector<Vertex>& out,const std::wstring& text){
 screenQuad(out,5,5,950,215,{.025f,.04f,.065f});chineseFont().draw(out,text,12,10);
}
