#include "src/chinese_text.h"
#include <set>
#include <iostream>
void check(bool ok){if(!ok)throw std::runtime_error("GDI glyph coverage mismatch");}
int main(){
 try{
 ChineseFont font;size_t before=0,after=0,checked=0;
 for(wchar_t c:std::wstring(L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz中文飞行高度速度导航账号起落架+-%./")){
  MAT2 identity{};identity.eM11.value=identity.eM22.value=1;GLYPHMETRICS gm{};
  DWORD bytes=GetGlyphOutlineW(font.dc,c,GGO_GRAY8_BITMAP,&gm,0,nullptr,&identity);
  check(bytes!=GDI_ERROR&&bytes>0);
  std::vector<unsigned char> data(bytes);
  check(GetGlyphOutlineW(font.dc,c,GGO_GRAY8_BITMAP,&gm,bytes,data.data(),&identity)!=GDI_ERROR);
  std::set<std::pair<int,int>> original,compacted;
  const UINT stride=(gm.gmBlackBoxX+3)&~3u;
  for(UINT y=0;y<gm.gmBlackBoxY;++y){
   bool span=false;
   for(UINT x=0;x<gm.gmBlackBoxX;++x){
    bool filled=data[y*stride+x]>=24;
    if(filled&&!span)++before;
    span=filled;
    if(filled)original.emplace(gm.gmptGlyphOrigin.x+int(x),font.metrics.tmAscent-gm.gmptGlyphOrigin.y+int(y));
   }
  }
  const auto& glyph=font.get(c);after+=glyph.runs.size();
  check(glyph.advance==std::max(4,int(gm.gmCellIncX)));
  for(const auto& r:glyph.runs)for(int y=int(r.y);y<int(r.y+r.h);++y)
   for(int x=int(r.x);x<int(r.x+r.w);++x)check(compacted.emplace(x,y).second);
  check(original==compacted);++checked;
 }
 check(after<=before);
 std::cout<<"GDI glyphs "<<checked<<", exact coverage and advance preserved; rectangles "<<before<<" -> "<<after<<". No GPU/FPS evidence.\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
