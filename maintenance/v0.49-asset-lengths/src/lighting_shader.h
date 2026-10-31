#pragma once
inline const char* shaderCode=R"(
cbuffer Camera:register(b0){row_major float4x4 mvp;float4 eye;float4 sun;float4 sunColor;float4 zenith;float4 horizon;float4 options;float4 cameraRight;float4 cameraUp;float4 cameraForward;float4 plane;float4 shape;}
Texture2DArray Atlas:register(t0);SamplerState AtlasSampler:register(s0);
struct V{float3 p:POSITION;float3 c:COLOR;float2 uv:TEXCOORD0;float textured:TEXCOORD1;float3 n:NORMAL;};
struct O{float4 p:SV_POSITION;float3 c:COLOR;float2 uv:TEXCOORD0;float textured:TEXCOORD1;float3 world:TEXCOORD2;float3 n:NORMAL;};
O vs(V v){O o;// World pass uses a rotation-only view; subtract before rotating to avoid
// cancellation of large world translations. Cabin/HUD/sky remain local.
float3 position=options.z<.5?v.p-eye.xyz:v.p;o.p=mul(float4(position,1),mvp);o.c=v.c;o.uv=v.uv;o.textured=v.textured;o.world=v.p;o.n=v.n;return o;}
float3 tone(float3 x){x=max(0,x*options.x);x=saturate((x*(2.51*x+.03))/(x*(2.43*x+.59)+.14));return pow(x,1.0/2.2);}
float hash(float2 p){return frac(sin(dot(p,float2(127.1,311.7)))*43758.5453);}
float noise(float2 p){float2 i=floor(p),f=frac(p);f=f*f*(3-2*f);return lerp(lerp(hash(i),hash(i+float2(1,0)),f.x),lerp(hash(i+float2(0,1)),hash(i+1),f.x),f.y);}
float3 sky(float3 d){
 float elevation=saturate(d.y);float3 c=lerp(horizon.rgb,zenith.rgb,pow(elevation,.45));float alignment=saturate(dot(d,sun.xyz));
 c+=sunColor.rgb*(.13*pow(alignment,24)+2.5*pow(alignment,6000));
 float2 q=d.xz/max(.12,d.y);float cloud=noise(q*1.4)*.65+noise(q*3.8)*.35;
 float cover=smoothstep(.64,.88,cloud)*smoothstep(.02,.2,d.y)*.32*sunColor.w;
 return lerp(c,float3(.75,.79,.83)*(.2+.8*sunColor.w),cover);
}
float boxOcclusion(float3 p,float3 center,float3 extent){
 float3 d=sun.xyz+float3(.00001,.00001,.00001);float3 a=(center-extent-p)/d,b=(center+extent-p)/d;
 float3 lo=min(a,b),hi=max(a,b);float enter=max(max(lo.x,lo.y),lo.z),leave=min(min(hi.x,hi.y),hi.z);
 return leave>max(enter,.05)?1:0;
}
float airportOcclusion(float3 p){
 if(abs(p.x-350)>700||abs(p.z+250)>1000)return 0;
 float blocked=0;for(int k=-1;k<=1;k++){float3 q=p+float3(k*.6,0,-k*.6);blocked+=max(boxOcclusion(q,float3(350,8,-250),float3(30,8,260)),boxOcclusion(q,float3(390,15,170),float3(8,15,8)));}return blocked/3;
}
float groundShadow(float3 p){
 if(options.w<.5||sun.y<.08||p.y>2)return 1;
 float building=1-.65*airportOcclusion(p)*sunColor.w;if(plane.y>250)return building;
 float2 delta=p.xz+sun.xz*((plane.y-p.y)/max(.08,sun.y))-plane.xz;
 float s=sin(plane.w),c=cos(plane.w);float2 local=float2(delta.x*c-delta.y*s,delta.x*s+delta.y*c);
 float softness=1+max(0,plane.y-shape.w)*.018;
 float body=length(local/float2(shape.x*softness,shape.y*softness));
 float wings=length(local/float2(shape.z*softness,max(1.5,shape.y*.13)*softness));
 float occlusion=1-smoothstep(.65,1.25,min(body,wings));
 return min(building,1-.63*occlusion*saturate(1-plane.y/250)*sunColor.w);
}
float4 materialSample(float2 uv){
 float2 origin;float scale,layer;
 if(uv.y<.5){origin=float2(uv.x<.5?0:.5,0);scale=2;layer=uv.x<.5?0:1;}
 else if(uv.x<.5){origin=float2(0,.5);scale=2;layer=2;}
 else{origin=float2(uv.x<.75?.5:.75,.5);scale=4;layer=uv.x<.75?3:4;}
 // Explicit gradients avoid undefined implicit derivatives at material branches.
 return Atlas.SampleGrad(AtlasSampler,float3(saturate((uv-origin)*scale),layer),ddx(uv)*scale,ddy(uv)*scale);
}
float4 ps(O i):SV_TARGET{
 // Cabin markers: 3 = lit interior surface, 4 = dimmable emissive display.
 // sun.w / horizon.w are packed controls, independent of outdoor exposure/fog.
 if(i.textured>3.5)return float4(i.c*lerp(.08,1,saturate(sun.w)),1);
 if(i.textured>2.5){
  float3 normal=i.n;if(dot(normal,normal)<.1)normal=cross(ddy(i.world),ddx(i.world));
  normal*=rsqrt(max(dot(normal,normal),.000001));
  if(dot(normal,eye.xyz-i.world)<0)normal=-normal;
  float hemisphere=saturate(normal.y*.5+.5);
  float3 ambient=float3(.06,.075,.10)*(1+4*sunColor.w);
  float3 flood=float3(.75,.52,.29)*horizon.w*(.45+.55*hemisphere);
  return float4(tone(pow(saturate(i.c),2.2)*(ambient+flood)),1);
 }
 if(i.textured>1.5)return float4(i.c,1);
 if(options.z>.5&&options.z<1.5)return float4(i.c,1);
 if(options.z>1.5){float3 ray=normalize(cameraForward.xyz+i.world.x*cameraRight.xyz+i.world.y*cameraUp.xyz);return float4(tone(sky(ray)),1);}
 float3 tex=materialSample(i.uv).rgb;
 float3 base=pow(saturate(i.c*lerp(float3(1,1,1),tex,saturate(i.textured))),2.2);
 float3 n=i.n; if(dot(n,n)<.1)n=cross(ddy(i.world),ddx(i.world));n=normalize(n);
 float3 view=normalize(eye.xyz-i.world);if(dot(n,view)<0)n=-n;
 float sunlight=saturate(dot(n,sun.xyz)),hemisphere=saturate(n.y*.5+.5);
 float3 ambient=lerp(float3(.035,.029,.024),float3(.19,.24,.32),hemisphere)*(.12+.88*sunColor.w);
 float shadow=groundShadow(i.world);float3 color=base*(ambient+sunColor.rgb*sunlight*shadow);
 float aircraft=i.textured>.5&&(i.uv.x>.5||i.uv.y>.5)?1:0;
 float3 halfVector=sun.xyz+view;float3 halfDir=halfVector*rsqrt(max(dot(halfVector,halfVector),.000001));float fresnel=.035+.12*pow(1-saturate(dot(n,view)),5);
 color+=aircraft*sunColor.rgb*pow(saturate(dot(n,halfDir)),64)*fresnel*shadow;
 float distance=length(i.world-eye.xyz),fog=1-exp(-distance*options.y);
 color=lerp(color,sky(normalize(i.world-eye.xyz)),saturate(fog));
 return float4(tone(color),1);
}
)";
