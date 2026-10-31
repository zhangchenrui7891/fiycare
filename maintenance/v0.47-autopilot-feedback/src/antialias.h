#pragma once
inline bool antiAliasEnabled=true;
inline const char* postCode=R"(
cbuffer Post:register(b0){float2 texel;float enabled;float unused;}
Texture2D Scene:register(t0);SamplerState LinearClamp:register(s0);
struct P{float4 position:SV_POSITION;float2 uv:TEXCOORD0;};
P postVS(uint id:SV_VertexID){P o;float2 uv=float2((id<<1)&2,id&2);o.uv=uv;o.position=float4(uv*float2(2,-2)+float2(-1,1),0,1);return o;}
float4 postPS(P i):SV_TARGET{
 float3 m=Scene.SampleLevel(LinearClamp,i.uv,0).rgb;if(enabled<.5)return float4(m,1);
 float3 nw=Scene.SampleLevel(LinearClamp,i.uv+texel*float2(-1,-1),0).rgb;
 float3 ne=Scene.SampleLevel(LinearClamp,i.uv+texel*float2(1,-1),0).rgb;
 float3 sw=Scene.SampleLevel(LinearClamp,i.uv+texel*float2(-1,1),0).rgb;
 float3 se=Scene.SampleLevel(LinearClamp,i.uv+texel*float2(1,1),0).rgb;
 float3 l=float3(.299,.587,.114);float lm=dot(m,l),lnw=dot(nw,l),lne=dot(ne,l),lsw=dot(sw,l),lse=dot(se,l);
 float low=min(lm,min(min(lnw,lne),min(lsw,lse))),high=max(lm,max(max(lnw,lne),max(lsw,lse)));
 if(high-low<max(.0312,high*.125))return float4(m,1);
 float2 dir=float2(-((lnw+lne)-(lsw+lse)),(lnw+lsw)-(lne+lse));
 float reduce=max((lnw+lne+lsw+lse)*(.25*.125),1.0/128);
 dir=clamp(dir/(min(abs(dir.x),abs(dir.y))+reduce),-8,8)*texel;
 float3 a=.5*(Scene.SampleLevel(LinearClamp,i.uv+dir*(-1.0/6),0).rgb+Scene.SampleLevel(LinearClamp,i.uv+dir*(1.0/6),0).rgb);
 float3 b=a*.5+.25*(Scene.SampleLevel(LinearClamp,i.uv-dir*.5,0).rgb+Scene.SampleLevel(LinearClamp,i.uv+dir*.5,0).rgb);
 float lb=dot(b,l);return float4(lb<low||lb>high?a:b,1);
}
)";
inline ComPtr<ID3DBlob> compilePost(const char* entry,const char* model){return compileCachedHlsl(postCode,entry,model);}
