#include "tests/reference/assets_v048_loaders.h"
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cstdlib>
#include <iostream>
#include <new>
#include <string>

namespace allocation {
bool intercept=false;
std::size_t largeRequests=0,largest=0;
constexpr std::size_t limit=1024*1024;
}
void* operator new(std::size_t bytes){
 if(allocation::intercept){
  allocation::largest=std::max(allocation::largest,bytes);
  if(bytes>allocation::limit){++allocation::largeRequests;throw std::bad_alloc();}
 }
 if(void* p=std::malloc(bytes?bytes:1))return p;
 throw std::bad_alloc();
}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}

void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
void u32(std::ostream& out,std::uint32_t n){
 const unsigned char b[]={static_cast<unsigned char>(n),static_cast<unsigned char>(n>>8),
  static_cast<unsigned char>(n>>16),static_cast<unsigned char>(n>>24)};
 out.write(reinterpret_cast<const char*>(b),4);
}
struct Temporary {
 std::filesystem::path path=std::filesystem::temp_directory_path()/
  ("fcair_lengths_"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 Temporary(){std::filesystem::create_directories(path);}
 ~Temporary(){std::error_code error;std::filesystem::remove_all(path,error);}
};
void write(const std::filesystem::path& path,const std::string& data){
 std::ofstream out(path,std::ios::binary);out.write(data.data(),std::streamsize(data.size()));
 require(bool(out),"Test file written");
}
template<class Reader> void rejectedWithoutLargeAllocation(Reader read){
 allocation::largeRequests=allocation::largest=0;allocation::intercept=true;
 bool rejected=false,large=false;
 try{read();}catch(const std::bad_alloc&){large=true;}catch(const std::runtime_error&){rejected=true;}
 catch(...){allocation::intercept=false;throw;}
 allocation::intercept=false;
 require(rejected&&!large&&allocation::largeRequests==0,"Malformed payload must fail before a large allocation request");
}
template<class Reader> void legacyLargeRequest(Reader read,std::size_t expected){
 allocation::largeRequests=allocation::largest=0;allocation::intercept=true;bool large=false;
 try{read();}catch(const std::bad_alloc&){large=true;}catch(...){allocation::intercept=false;throw;}
 allocation::intercept=false;
 require(large&&allocation::largeRequests==1&&allocation::largest>=expected&&allocation::largest<=expected+4096,"Frozen old reader reproduces advertised allocation request");
}
std::string model(bool colored){
 std::ostringstream out;out.write(colored?"FCMESH07":"FCMESH06",8);u32(out,3);u32(out,12);u32(out,1);
 const Vertex vertices[]={{0,0,0,.2f,.4f,.8f,0,0,colored?0.f:1.f,0,1,0},
  {1,0,0,.2f,.4f,.8f,1,0,colored?0.f:1.f,0,1,0},
  {0,0,1,.2f,.4f,.8f,0,1,colored?0.f:1.f,0,1,0}};
 out.write(reinterpret_cast<const char*>(vertices),sizeof(vertices));out.put(0);return out.str();
}
int main(){
 try{
  Temporary files;const auto path=files.path/std::filesystem::u8path(u8"资源 长度.bin");
  std::ostringstream atlas;atlas.write("FCTEX005",8);u32(atlas,2);u32(atlas,2);
  for(int i=0;i<16;++i)atlas.put(char(i));
  const auto validAtlas=atlas.str();write(path,validAtlas);
  const auto actual=loadAtlas(path),old=legacyLoadAtlas(path);
  require(actual.width==2&&actual.height==2&&actual.rgba==old.rgba&&actual.rgba.size()==16,"Valid atlas bytes preserved");
  std::size_t rejected=0;
  for(std::size_t length=0;length<validAtlas.size();++length){
   write(path,validAtlas.substr(0,length));rejectedWithoutLargeAllocation([&]{loadAtlas(path);});++rejected;
  }
  write(path,validAtlas+"x");rejectedWithoutLargeAllocation([&]{loadAtlas(path);});++rejected;
  std::ostringstream bigAtlas;bigAtlas.write("FCTEX005",8);u32(bigAtlas,4096);u32(bigAtlas,4096);
  write(path,bigAtlas.str());
  legacyLargeRequest([&]{legacyLoadAtlas(path);},4096u*4096u*4u);
  rejectedWithoutLargeAllocation([&]{loadAtlas(path);});++rejected;
  for(bool colored:{false,true}){
   const auto bytes=model(colored);write(path,bytes);
   const auto now=loadModel(path),before=legacyLoadModel(path);
   require(now.vertices.size()==3&&now.gear==before.gear&&
    std::memcmp(now.vertices.data(),before.vertices.data(),3*sizeof(Vertex))==0,"Both valid native mesh versions preserve all vertex/tag bytes");
   for(std::size_t length=0;length<bytes.size();++length){
    write(path,bytes.substr(0,length));rejectedWithoutLargeAllocation([&]{loadModel(path);});++rejected;
   }
   write(path,bytes+"x");rejectedWithoutLargeAllocation([&]{loadModel(path);});++rejected;
   std::ostringstream big;big.write(colored?"FCMESH07":"FCMESH06",8);u32(big,3000000);u32(big,12);u32(big,1000000);
   write(path,big.str());
   legacyLargeRequest([&]{legacyLoadModel(path);},3000000u*sizeof(Vertex));
   rejectedWithoutLargeAllocation([&]{loadModel(path);});++rejected;
  }
  std::cout<<"PASS "<<rejected<<" truncated/trailing payload cases; valid atlas/FCMESH06/07 bytes preserved. "
    "Old header-only atlas/mesh request at least 67108864/144000000 bytes (allocator padding allowed), intercepted before allocation. "
    "New readers reject with zero requests above 1MiB. Not process RAM, Windows startup, GPU or FPS measurement.\n";
  return 0;
 }catch(const std::exception& e){allocation::intercept=false;std::cerr<<e.what()<<'\n';return 1;}
}
