"""Convert embedded GLB meshes into native color/UV triangles. Requires numpy and Pillow.
Run: python tools/convert_assets.py --source-dir /path/to/glbs --satellite /path/to/image.png
Original files are retained unchanged. No runtime GLB parser or external engine is required.
"""
import argparse,struct,json,io,pathlib,hashlib
import numpy as np
from PIL import Image

def load_glb(path):
 b=path.read_bytes();magic,version,total=struct.unpack_from('<III',b)
 if magic!=0x46546c67 or version!=2 or total!=len(b):raise ValueError('Not a valid GLB2')
 chunks={};off=12
 while off<len(b):
  size,kind=struct.unpack_from('<II',b,off);chunks[kind]=b[off+8:off+8+size];off+=8+size
 return json.loads(chunks[0x4e4f534a]),chunks[0x004e4942]
def accessor(j,b,index):
 a=j['accessors'][index];v=j['bufferViews'][a['bufferView']]
 if 'sparse' in a:raise ValueError('Sparse accessors not supported')
 dtype={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']];count={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4}[a['type']]
 dt=np.dtype(dtype);off=v.get('byteOffset',0)+a.get('byteOffset',0);stride=v.get('byteStride',dt.itemsize*count)
 arr=np.ndarray((a['count'],count),dtype=dt,buffer=b,offset=off,strides=(stride,dt.itemsize)).copy()
 return arr[:,0] if count==1 else arr
def transform(node):
 if 'matrix' in node:return np.array(node['matrix']).reshape((4,4),order='F')
 x,y,z,w=node.get('rotation',[0,0,0,1])
 rot=np.array([[1-2*y*y-2*z*z,2*x*y-2*z*w,2*x*z+2*y*w],[2*x*y+2*z*w,1-2*x*x-2*z*z,2*y*z-2*x*w],[2*x*z-2*y*w,2*y*z+2*x*w,1-2*x*x-2*y*y]])
 m=np.eye(4);m[:3,:3]=rot@np.diag(node.get('scale',[1,1,1]));m[:3,3]=node.get('translation',[0,0,0]);return m
def mesh_parts(j,b):
 result=[]
 def walk(i,parent):
  n=j['nodes'][i];m=parent@transform(n)
  if 'mesh' in n:
   for p in j['meshes'][n['mesh']]['primitives']:
    if p.get('mode',4)!=4:raise ValueError('Only triangles supported')
    positions=accessor(j,b,p['attributes']['POSITION']);positions=(np.c_[positions,np.ones(len(positions))]@m.T)[:,:3]
    normals=accessor(j,b,p['attributes']['NORMAL'])@np.linalg.inv(m[:3,:3])
    normals/=np.maximum(np.linalg.norm(normals,axis=1)[:,None],1e-9)
    uv=accessor(j,b,p['attributes']['TEXCOORD_0'])
    indices=accessor(j,b,p['indices']).astype(np.int64) if 'indices' in p else np.arange(len(positions))
    result.append((positions,normals,uv,indices,p.get('material',0),1 if 'LandingGear' in n.get('name','') else 0))
  for c in n.get('children',[]):walk(c,m)
 for i in j['scenes'][j.get('scene',0)]['nodes']:walk(i,np.eye(4))
 return result
def extract_image(j,b,texture):
 image=j['images'][j['textures'][texture]['source']];v=j['bufferViews'][image['bufferView']];start=v.get('byteOffset',0)
 return Image.open(io.BytesIO(b[start:start+v['byteLength']])).convert('RGBA')
def build(source,satellite,out):
 out.mkdir(parents=True,exist_ok=True)
 atlas=Image.new('RGBA',(2048,2048),(255,255,255,255))
 atlas.paste(Image.open(satellite).convert('RGBA').resize((1024,1024),Image.Resampling.LANCZOS),(0,0))
 records=[]
 configs=[('a330-300_plane.glb','a330.fcmesh',63.66,4.8,{0:(1024,0,1024)}),('airbus_a350-900.glb','a350.fcmesh',66.8,7.5,{3:(0,1024,1024),0:(1024,1024,512),1:(1536,1024,512),2:(1536,1024,512)})]
 for name,dest,length,gearheight,rectangles in configs:
  j,b=load_glb(source/name);parts=mesh_parts(j,b);allpos=np.concatenate([p[0] for p in parts]);low=allpos.min(0);high=allpos.max(0);scale=length/(high[2]-low[2]);center=(low+high)*.5
  print(name,'original bounds',low,high,'scale',scale)
  for mat,rect in rectangles.items():
   material=j['materials'][mat];texture=material.get('pbrMetallicRoughness',{}).get('baseColorTexture',{}).get('index')
   image=extract_image(j,b,texture) if texture is not None else Image.new('RGBA',(4,4),(255,255,255,255))
   x,y,side=rect;atlas.paste(image.resize((side,side),Image.Resampling.LANCZOS),(x,y))
  vertices=[];gear_flags=[]
  for positions,normals,uv,indices,material,gear in parts:
   # A330 source points +Z; A350 source points -Z. Native forward is +Z.
   positions=(positions-np.array([center[0],low[1],center[2]]))*scale
   if dest=='a350.fcmesh':positions[:,[0,2]]*=-1;normals[:,[0,2]]*=-1
   positions[:,1]-=gearheight
   if dest=='a330.fcmesh':positions[:,1]+=2 # source has no landing gear; allow a simplified native gear
   x,y,side=rectangles[material]
   uv=np.clip(uv,0,1);uv[:,0]=(x+1+uv[:,0]*(side-2))/2048;uv[:,1]=(y+1+uv[:,1]*(side-2))/2048
   factor=np.array(j['materials'][material].get('pbrMetallicRoughness',{}).get('baseColorFactor',[1,1,1,1])[:3])
   brightness=np.ones(len(normals))
   colors=brightness[:,None]*factor
   v=np.c_[positions,colors,uv,np.ones(len(positions)),normals].astype('<f4')[indices]
   if not np.isfinite(v).all():raise ValueError('Non-finite vertex')
   vertices.append(v);gear_flags.extend([gear]*(len(indices)//3))
  data=np.concatenate(vertices);flags=np.array(gear_flags,dtype='u1')
  with (out/dest).open('wb') as f:f.write(struct.pack('<8sIII',b'FCMESH06',len(data),12,len(flags)));f.write(data.tobytes());f.write(flags.tobytes())
  records.append({'input':name,'input_sha256':hashlib.sha256((source/name).read_bytes()).hexdigest(),'output':dest,'triangles':len(flags),'scale':scale,'length_m':length,'ground_offset_m':gearheight,'body_lift_m':2 if dest=='a330.fcmesh' else 0,'forward_axis':'+Z','attribution':j.get('asset',{}).get('extras',{})})
 atlas.save(out/'atlas.png')
 rgba=np.asarray(atlas,dtype=np.uint8)
 with (out/'atlas.fctex').open('wb') as f:f.write(struct.pack('<8sII',b'FCTEX005',2048,2048));f.write(rgba.tobytes())
 (out/'model_manifest.json').write_text(json.dumps(records,ensure_ascii=False,indent=2),encoding='utf-8')
 print('Wrote atlas and native model assets to',out)
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('--source-dir',type=pathlib.Path,required=True);p.add_argument('--satellite',type=pathlib.Path,required=True);p.add_argument('--output',type=pathlib.Path,default=pathlib.Path('assets'));a=p.parse_args();build(a.source_dir,a.satellite,a.output)
