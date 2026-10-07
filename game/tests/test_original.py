#!/usr/bin/env python3
"""Independent double-precision timeline oracle + gameplay/protocol regressions."""
from pathlib import Path
import ctypes as C,json,math,random
R=Path(__file__).resolve().parents[2];d=json.loads((R/'game/reference/original-levels.json').read_text());g=C.CDLL(str(R/'build/game.so'))
config=json.loads((R/'game/display.json').read_text())
u8=C.c_uint8;out=(u8*512)();command=(u8*8)(config['protocol_version'],60,0,0,0,0,0,0)
def cmd(button=0,stick=0,elapsed=2):
 command[2]=elapsed;command[3]=stick;command[4]=button
 assert g.hg_command(command,out);return bytes(out)
def start(l):
 g.hg_test_reset();command[5]=l;cmd(elapsed=0);command[5]=0
 assert out[4]==l and out[5]==0
 cmd(button=1);cmd();assert out[5]==1

def mul(a,b):return [a[0]*b[0]+a[2]*b[1],a[1]*b[0]+a[3]*b[1],a[0]*b[2]+a[2]*b[3],a[1]*b[2]+a[3]*b[3],a[0]*b[4]+a[2]*b[5]+a[4],a[1]*b[4]+a[3]*b[5]+a[5]]
def oracle(i,t,p=(1,0,0,1,0,0)):
 a,n,c,k=d['nodes'][i];m=mul(p,d['tracks'][a+t%n])
 if not k:return [(m[4]/20,m[5]/20)]
 return [v for j in d['children'][c:c+k] for v in oracle(j,t,m)]
maxerror=0;checks=0
for l,ld in enumerate(d['levels']):
 g.hg_test_level(l)
 for t in [0,1,2,30,65,71,72,96,97,210,211,287,288,359,360,600,997,4096,9999]:
  g.hg_test_tick(t);xy=oracle(ld['enemy_root'],t);assert len(xy)==g.hg_test_hazards()
  for i,(x,y) in enumerate(xy):
   err=max(abs(x-g.hg_test_enemy_x(i)/65536),abs(y-g.hg_test_enemy_y(i)/65536));maxerror=max(maxerror,err);assert err<.03,(l,t,i,err)
  assert g.hg_test_glyphs()<128;checks+=len(xy)
 # All spawn locations are navigable, including intermediate checkpoints.
 for cp in ld['checkpoints'][:-1]:
  x,y=cp['spawn'];assert not g.hg_test_wall(round(x),round(y)),(l,cp)
 start(l)
 # Finish condition requires the original coin quota, then the final safe zone.
 rect=ld['checkpoints'][-1]['rect'];goal=[(rect[0]+rect[2])/2,(rect[1]+rect[3])/2];g.hg_test_position(round(goal[0]),round(goal[1]))
 if ld['required_coins']:
  cmd();assert out[5]==1,(l,'goal accepted without coins')
  g.hg_test_clearcoins()
 cmd();assert out[5]==(3 if l==29 else 2),(l,'goal refused')
 # Transition sequence returns to the start screen of the next level.
 cmd(1);assert out[4]==(l+1 if l<29 else 0) and out[5]==0
# Real collision, death delay, coin reset, wall bounds, pause and button edges.
start(0);g.hg_test_tick(10);g.hg_test_position(round(g.hg_test_enemy_x(0)/65536),round(g.hg_test_enemy_y(0)/65536));cmd();assert out[15]>0
for _ in range(27):cmd()
assert g.hg_test_deaths()==1
# Reduced player: a 15-original-pixel offset from an isolated enemy is now a
# near miss (the old 9-pixel half-width collided); 13 pixels still collides.
g.hg_test_level(0);g.hg_test_tick(0)
x=round(g.hg_test_enemy_x(0)/65536);y=round(g.hg_test_enemy_y(0)/65536)
g.hg_test_position(x+15,y);assert not g.hg_test_collision()
g.hg_test_position(x+13,y);assert g.hg_test_collision()
start(1);coin=d['levels'][1]['coins'][0];g.hg_test_position(round(coin[0]),round(coin[1]));cmd();assert g.hg_test_collected()==1
cmd(1);assert out[5]==5;t=out[18]+256*out[19]
for _ in range(20):cmd()
assert out[18]+256*out[19]==t
cmd(1);assert out[5]==1
cmd();cmd(8);assert out[16]==1
cmd();cmd(8);assert out[16]==0
cmd();cmd(2);assert out[5]==0 and g.hg_test_collected()==0
# PAL and NTSC advance precisely 30 simulation ticks per elapsed second.
for fps in [50,60]:
 command[1]=fps;start(0);a=out[18]+256*out[19]
 for _ in range(fps//2):cmd()
 assert (out[18]+256*out[19])-a==30
# Reject malformed commands and invalid snapshot page, without accessing storage.
for offset,value in [(0,1),(1,0),(2,13),(3,16),(4,16),(5,30),(6,1),(7,1)]:
 good=command[offset];command[offset]=value;assert not g.hg_command(command,out);command[offset]=good
assert not g.hg_page(4,out)
report=dict(levels=30,enemy_positions_compared=checks,max_coordinate_error_original_pixels=maxerror,simulation_hz=30,checks='coin quota, all goals/transitions, all checkpoint spawns, death, coin collection, pause, mute, restart, PAL/NTSC rate, malformed packets')
(R/'validation/original-tests.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
