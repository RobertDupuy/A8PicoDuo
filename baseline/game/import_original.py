#!/usr/bin/env python3
"""Extract level geometry and timeline transforms from FFDec XML of original SWF.
No ActionScript, audio, or Flash runtime is embedded in the cartridge.
Usage: python3 game/import_original.py /path/to/original-flash.xml
"""
import xml.etree.ElementTree as E, sys, json, math, hashlib
from pathlib import Path
import numpy as np
P=Path(__file__).resolve().parent
root=E.parse(sys.argv[1]).getroot()
assert root.get('frameRate')=='30.0'
defs={int(t.get('spriteId',t.get('shapeId','-1'))):t for t in root.find('tags') if t.get('type') in ['DefineSpriteTag','DefineShapeTag','DefineShape2Tag','DefineShape3Tag','DefineShape4Tag']}
def mat(t):
 m=t.find('matrix')
 if m is None:return [1,0,0,1,0,0]
 return [float(m.get(k,d)) for k,d in [('scaleX','1'),('rotateSkew0','0'),('rotateSkew1','0'),('scaleY','1'),('translateX','0'),('translateY','0')]]
def mul(a,b):
 return [a[0]*b[0]+a[2]*b[1],a[1]*b[0]+a[3]*b[1],a[0]*b[2]+a[2]*b[3],a[1]*b[2]+a[3]*b[3],a[0]*b[4]+a[2]*b[5]+a[4],a[1]*b[4]+a[3]*b[5]+a[5]]
def xy(m,x,y):return [(m[0]*x+m[2]*y+m[4])/20,(m[1]*x+m[3]*y+m[5])/20]
def frames(tags):
 state={};out=[]
 for t in tags:
  typ=t.get('type')
  if typ.startswith('PlaceObject'):
   dep=int(t.get('depth'));obj=state.get(dep,{}).copy() if t.get('placeFlagMove')=='true' else {}
   if t.get('characterId'):obj['id']=int(t.get('characterId'))
   if t.find('matrix') is not None:obj['matrix']=mat(t)
   if t.get('name'):obj['name']=t.get('name')
   state[dep]=obj
  elif typ.startswith('RemoveObject'):state.pop(int(t.get('depth')),None)
  elif typ=='ShowFrameTag':out.append({k:v.copy() for k,v in state.items()})
 return out
cache={}
def sf(i):
 if i not in cache:cache[i]=frames(defs[i].find('subTags'))
 return cache[i]
def shapes(i,m):
 t=defs[i]
 if t.get('type')=='DefineSpriteTag':
  for o in sf(i)[0].values():yield from shapes(o['id'],mul(m,o['matrix']))
 else:yield t,m
# Convert each fill to a directed edge list. Curves are flattened only for geometry
# (all original maze walls are straight). Opposite fill sides cancel by parity.
def fills(shape,m):
 groups={};x=y=0;f0=f1=0
 for r in shape.find('shapes/shapeRecords'):
  typ=r.get('type')
  if typ=='StyleChangeRecord':
   if r.get('stateMoveTo')=='true':x=int(r.get('moveDeltaX'));y=int(r.get('moveDeltaY'))
   if r.get('stateFillStyle0')=='true':f0=int(r.get('fillStyle0'))
   if r.get('stateFillStyle1')=='true':f1=int(r.get('fillStyle1'))
  elif typ=='StraightEdgeRecord':
   xx=x+int(r.get('deltaX','0'));yy=y+int(r.get('deltaY','0'))
   for f in [f0,f1]:
    if f:groups.setdefault(f,[]).append(xy(m,x,y)+xy(m,xx,yy))
   x,y=xx,yy
  elif typ=='CurvedEdgeRecord':
   cx=x+int(r.get('controlDeltaX'));cy=y+int(r.get('controlDeltaY'))
   xx=cx+int(r.get('anchorDeltaX'));yy=cy+int(r.get('anchorDeltaY'))
   px,py=x,y
   for j in range(1,33):
    t=j/32;qx=(1-t)**2*x+2*t*(1-t)*cx+t*t*xx;qy=(1-t)**2*y+2*t*(1-t)*cy+t*t*yy
    for f in [f0,f1]:
     if f:groups.setdefault(f,[]).append(xy(m,px,py)+xy(m,qx,qy))
    px,py=qx,qy
   x,y=xx,yy
 return list(groups.values())
def bounds(i,m):
 pts=[]
 for shape,mm in shapes(i,m):
  b=shape.find('shapeBounds'); pts.extend(xy(mm,x,y) for x in [int(b.get('Xmin')),int(b.get('Xmax'))] for y in [int(b.get('Ymin')),int(b.get('Ymax'))])
 return [min(p[0] for p in pts),min(p[1] for p in pts),max(p[0] for p in pts),max(p[1] for p in pts)]
# Every enemy container is a declarative Flash timeline, not an AS program.
# Store independent instance tracks, preserving parent/child transforms and periods.
nodes=[];tracks=[];childids=[];used=set();track_cache={};node_cache={}
def node(i,track):
 key=(i,tuple(tuple(m) for m in track))
 if key in node_cache:return node_cache[key]
 used.add(i); n=len(nodes);nodes.append(None);node_cache[key]=n
 tk=key[1]
 if tk not in track_cache:track_cache[tk]=len(tracks);tracks.extend(track)
 off=track_cache[tk]
 if i in (81,266,354): # base blue circle, radius 6.5 original pixels
  nodes[n]=[off,len(track),0,0];return n
 t=defs[i]
 assert t.get('type')=='DefineSpriteTag',('enemy leaf',i,t.get('type'))
 assert not any('Action' in x.get('type') or x.get('placeFlagHasClipActions')=='true' for x in t.find('subTags'))
 fs=sf(i);deps=list(fs[0]);kids=[]
 assert all(list(f)==deps for f in fs),('changing instances',i)
 for d in deps:
  assert len(set(f[d]['id'] for f in fs))==1
  tr=[f[d]['matrix'] for f in fs]
  if all(m==tr[0] for m in tr):tr=tr[:1]
  kids.append(node(fs[0][d]['id'],tr))
 c=len(childids);childids.extend(kids);nodes[n]=[off,len(track),c,len(kids)];return n
allframes=frames(root.find('tags'));levels=[]
expected=[0,1,1,3,0,4,4,3,1,0,2,1,0,0,0,4,0,67,0,7,3,0,36,0,0,4,1,0,2,4]
for l in range(30):
 os=allframes[70+l*4];byname={o.get('name'):o for o in os.values() if o.get('name')}
 w=byname['walls'];wg=[]
 for s,m in shapes(w['id'],w['matrix']):wg+=fills(s,m)
 checks=[]
 for name,o in byname.items():
  if name.startswith('check'):
   b=bounds(o['id'],o['matrix']);checks.append(dict(name=name,rect=b,spawn=xy(o['matrix'],0,0)))
 checks.sort(key=lambda x:int(x['name'][5:]))
 coins=[xy(o['matrix'],0,0) for dep,o in os.items() if o['id']==115 and dep>next(d for d,v in os.items() if v.get('name')=='walls')]
 
 assert len(coins)==expected[l], (l+1,len(coins),expected[l])
 en=byname['enemies'];ni=node(en['id'],[en['matrix']])
 # Independent raster sampling of the original vector wall fills.
 X,Y=np.meshgrid((np.arange(160)+.5)*550/160,25+(np.arange(112)+.5)*350/112)
 mask=np.zeros(X.shape,dtype=bool)
 for g in wg:
  inside=np.zeros(X.shape,dtype=bool)
  for x1,y1,x2,y2 in g:
   if y1!=y2:inside^=((y1>Y)!=(y2>Y)) & (X < (x2-x1)*(Y-y1)/(y2-y1)+x1)
  mask|=inside
 bg=mask.astype('uint8')
 for ch in checks:
  x1,y1,x2,y2=ch['rect'];bg[(X>=x1)&(X<x2)&(Y>=y1)&(Y<y2)&~mask]=3
 vals=bg.flatten().tolist();rle=[]
 for v in vals:
  if rle and rle[-1][1]==v and rle[-1][0]<255:rle[-1][0]+=1
  else:rle.append([1,v])
 # Original-resolution collision bitmap, losslessly RLE encoded. Union of fills.
 high=np.zeros((400,550),dtype=np.uint8)
 for yy in range(400):
  for g in wg:
   crosses=sorted(x1+(x2-x1)*(yy+.5-y1)/(y2-y1) for x1,y1,x2,y2 in g if (y1>yy+.5)!=(y2>yy+.5))
   assert len(crosses)%2==0
   for a,b in zip(crosses[::2],crosses[1::2]): high[yy,max(0,math.ceil(a-.5)):max(0,min(550,math.ceil(b-.5)))]=1
 packed=np.packbits(high.reshape(-1)).tolist();crle=[]
 for v in packed:
  if crle and crle[-1][1]==v and crle[-1][0]<255:crle[-1][0]+=1
  else:crle.append([1,v])
 levels.append(dict(required_coins=expected[l],collision_rle=crle,level=l+1,frame=71+l*4,walls=wg,checkpoints=checks,coins=coins,enemy_root=ni,background_rle=rle))
data=dict(source='WHGOriginal.swf',fps=30,width=550,height=400,crop=[0,25,550,350],screen=[160,112],levels=levels,nodes=nodes,tracks=tracks,children=childids)
(P/'reference/original-levels.json').write_text(json.dumps(data,separators=(',',':'))+'\n')
print('Verified 30 original levels; coins:',expected)
print('Nodes',len(nodes),'matrix records',len(tracks),'children',len(childids),'source XML SHA256',hashlib.sha256(Path(sys.argv[1]).read_bytes()).hexdigest())
