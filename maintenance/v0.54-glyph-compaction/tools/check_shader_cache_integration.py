"""Compile the production HLSL wrappers with a CPU compiler/COM stand-in.

Uses real source strings and both renderers' actual compile-call order, but
does NOT validate HLSL syntax, Windows ABI or GPU resource creation.
"""
from pathlib import Path
import re
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
common=(root/'src/graphics_common.h').read_text()
start=common.index('inline compilation::ShaderCache<ComPtr<ID3DBlob>>& shaderBytecodeCache()')
compiler=common[start:common.index('#include "assets.h"',start)]
start=common.index('inline ComPtr<ID3DBlob> compileShader(')
scene_wrapper=common[start:common.index('inline void writeBitmap(',start)]
calls={}
for backend in (11,12):
    text=(root/f'src/renderer{backend}.h').read_text()
    matches=re.findall(r'\b(compileShader|compilePost)\("([^"]+)","([^"]+)"\)',text)
    if len(matches)!=4:
        raise RuntimeError(f'Expected four actual DX{backend} shader requests, got {matches}')
    calls[backend]=''.join(f'{function}("{entry}","{profile}");' for function,entry,profile in matches)

prefix=r'''
#include "shader_cache.h"
#include <memory>
#include <vector>
#include <iostream>
#include <cstring>
using UINT=unsigned;
using HRESULT=int;
constexpr UINT D3DCOMPILE_ENABLE_STRICTNESS=1;
bool FAILED(HRESULT result){return result<0;}
struct ID3DBlob {
 std::string bytes;
 explicit ID3DBlob(std::string value):bytes(std::move(value)){}
 void* GetBufferPointer(){return bytes.data();}
 std::size_t GetBufferSize() const{return bytes.size();}
};
template<class T> using ComPtr=std::shared_ptr<T>;
int compilerCalls=0,failNext=0,emptyNext=0;
struct CompileInput {std::string source,entry,profile;UINT flags1,flags2;};
std::vector<CompileInput> compilerInputs;
std::vector<std::string> logs;
void logLine(const std::string& text){logs.push_back(text);}
void check(HRESULT result,const char* reason){if(FAILED(result))throw std::runtime_error(reason);}
HRESULT D3DCompile(const void* data,std::size_t size,const char* sourceName,const void* defines,
                  const void* includes,const char* entry,const char* profile,UINT flags1,UINT flags2,
                  ComPtr<ID3DBlob>* code,ComPtr<ID3DBlob>* error){
 if(sourceName||defines||includes)throw std::runtime_error("Unexpected compiler context");
 ++compilerCalls;
 const std::string source(static_cast<const char*>(data),size);
 compilerInputs.push_back({source,entry,profile,flags1,flags2});
 if(failNext){--failNext;*error=std::make_shared<ID3DBlob>("injected compile error");return -1;}
 if(emptyNext){--emptyNext;*code=std::make_shared<ID3DBlob>("");return 0;}
 *code=std::make_shared<ID3DBlob>(source+entry+profile+std::to_string(flags1)+std::to_string(flags2));return 0;
}
void require(bool ok,const char* reason){if(!ok)throw std::runtime_error(reason);}
'''
test=r'''
int main(){
 for(int rebuild=0;rebuild<10;rebuild++){renderer11Requests();renderer12Requests();}
 auto stats=shaderBytecodeCache().stats();
 require(compilerCalls==4&&stats.requests==80&&stats.hits==76&&stats.compilations==4,
         "Actual backend request order must share four CPU bytecode blobs");
 auto before=compileShader("vs","vs_5_0");
 logShaderBytecodeCache();
 require(logs.back().find("compilations=4")!=std::string::npos&&logs.back().find("hits=77")!=std::string::npos,
         "Production diagnostic counters match actual requests");
 const int baseline=compilerCalls;
 const std::string changed=std::string(shaderCode)+"\n// changed source";
 compileCachedHlsl(changed.c_str(),"vs","vs_5_0");
 compileCachedHlsl(changed.c_str(),"vs","vs_5_0");
 require(compilerCalls==baseline+1,"Changed HLSL compiles once, next request hits");
 compileCachedHlsl(shaderCode,"vs","vs_5_0",7,9);
 require(compilerInputs.back().flags1==7&&compilerInputs.back().flags2==9,"Flags reach compiler and cache identity");
 failNext=1;bool failed=false;
 try{compileCachedHlsl("failure fixture","main","ps_5_0");}catch(const std::exception&){failed=true;}
 require(failed&&logs.back()=="injected compile error","Compiler diagnostics propagate and are logged");
 const int failuresBeforeRetry=compilerCalls;
 auto recovered=compileCachedHlsl("failure fixture","main","ps_5_0");
 require(recovered&&compilerCalls==failuresBeforeRetry+1,"Actual wrapper retries failed compilation");
 emptyNext=1;failed=false;
 try{compileCachedHlsl("empty fixture","main","ps_5_0");}catch(const std::exception&){failed=true;}
 require(failed,"Zero-length compiler blob is rejected");
 compileCachedHlsl("empty fixture","main","ps_5_0");
 require(shaderBytecodeCache().stats().failures==2,"Failed and empty outputs remain visible");
 const auto retained=before->bytes;
 shaderBytecodeCache().clear();
 require(before->bytes==retained,"Bytecode owners survive cache clear, like renderer recreation");
 require(compileShader("vs","vs_5_0")!=before,"Explicit invalidation permits a new blob");
 std::cout<<"Actual DX11/DX12 shader-call sequences x10 each: 4 compiler calls, 76 hits.\n"
          <<"Production wrappers: source/flags invalidation, failed/empty retry, diagnostics and ownership passed.\n"
          <<"CPU stand-ins only; Windows HLSL compiler and GPU were NOT executed.\n";
}
'''
source=prefix+compiler+'\n#include "lighting_shader.h"\n#include "antialias.h"\n'+scene_wrapper
source+='\nvoid renderer11Requests(){'+calls[11]+'}\nvoid renderer12Requests(){'+calls[12]+'}\n'+test
with tempfile.TemporaryDirectory() as folder:
    cpp=Path(folder)/'integration.cpp'
    cpp.write_text(source)
    exe=Path(folder)/'integration'
    subprocess.run(['g++','-std=c++17','-O2','-Wall','-Wextra','-Werror','-pedantic',
                    '-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
