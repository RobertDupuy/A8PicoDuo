#!/usr/bin/env python3
"""Compile audited Flash timeline data into immutable U2 tables."""
from pathlib import Path
import json,runpy,math
P=Path(__file__).resolve().parent;G=P/'generated';G.mkdir(exist_ok=True)
d=json.loads((P/'reference/original-levels.json').read_text())
cfg=json.loads((P/'display.json').read_text())
W,H=cfg['screen_width'],cfg['screen_height'];left,ww=cfg['world_left'],cfg['world_width']
assert W==160 and H%8==0 and cfg['top_blank_scanlines']==24
assert cfg['hud_scanlines']+H*2<=192
constants={
 'SCREEN_WIDTH':W,'SCREEN_HEIGHT':H,'WORLD_WIDTH':ww,'WORLD_LEFT':left,
 'SOURCE_WIDTH':cfg['source_width'],'SOURCE_HEIGHT':cfg['source_height'],'SOURCE_TOP':cfg['source_top'],
 'PLAYER_WIDTH':cfg['player_width'],'PLAYER_HEIGHT':cfg['player_height'],
 'PLAYER_HALF_Q':round(cfg['player_half_original_pixels']*65536),
 'PLAYER_HALF_16':round(cfg['player_half_original_pixels']*16),
 'PLAYER_SCANLINES':cfg['player_height']*2,
 'PLAYER_PATTERN':(255<<(8-cfg['player_width']))&255,
 'WORLD_FIRST_SCANLINE':cfg['first_antic_scanline']+cfg['top_blank_scanlines']+cfg['hud_scanlines'],
 'TOP_BLANK':cfg['top_blank_scanlines'],'CHAR_ROWS':H//8,
 'VIDEO_SECTORS':(1024+(W//4)*(H//8)+127)//128,
 'PROTOCOL_VERSION':cfg['protocol_version']}
(G/'display.h').write_text('/* Generated from game/display.json. */\n'+''.join(f'#define HG_{k} {v}\n' for k,v in constants.items()))
(G/'display.inc').write_text('; Generated from game/display.json.\n'+''.join(f'HG_{k} = {v}\n' for k,v in constants.items()))
def background(level):
 # Sample the preserved original vectors into a TV-safe, centered viewport.
 # Original collision mask and obstacle coordinates remain at original resolution.
 pixels=[1]*(W*H)
 for y in range(H):
  yy=cfg['source_top']+(y+.5)*cfg['source_height']/H
  pixels[y*W+left:y*W+left+ww]=[0]*ww
  for group in level['walls']:
   crosses=sorted(x1+(x2-x1)*(yy-y1)/(y2-y1) for x1,y1,x2,y2 in group if (y1>yy)!=(y2>yy))
   assert len(crosses)%2==0
   for a,b in zip(crosses[::2],crosses[1::2]):
    x1=max(left,min(left+ww,math.ceil(left+a*ww/cfg['source_width']-.5)))
    x2=max(left,min(left+ww,math.ceil(left+b*ww/cfg['source_width']-.5)))
    pixels[y*W+x1:y*W+x2]=[1]*(x2-x1)
  for cp in level['checkpoints']:
   a,b,c,e=cp['rect']
   if b<=yy<e:
    for x in range(left,left+ww):
     xx=(x-left+.5)*cfg['source_width']/ww
     if a<=xx<c and pixels[y*W+x]==0:pixels[y*W+x]=3
 rle=[]
 for v in pixels:
  if rle and rle[-1][1]==v and rle[-1][0]<255:rle[-1][0]+=1
  else:rle.append([1,v])
 return rle
def array(f,typ,name,rows):
 f.write('static const '+typ+' '+name+'[]={\n')
 for r in rows:f.write(' {'+','.join(map(str,r))+'},\n' if isinstance(r,list) else str(r)+',')
 f.write('\n};\n')
with (G/'levels.h').open('w') as f:
 f.write('/* Generated from original Flash 30-fps timeline. */\n')
 array(f,'Matrix','tracks',[[round(v*65536/(20 if k>=4 else 1)) for k,v in enumerate(t)] for t in d['tracks']])
 array(f,'Node','nodes',d['nodes']);array(f,'uint16_t','children',d['children'])
 bg=[];cm=[];coins=[];checks=[];levels=[]
 for l in d['levels']:
  levels.append([l['enemy_root'],len(bg),len(cm),len(coins),len(l['coins']),l['required_coins'],len(checks),len(l['checkpoints'])])
  bg.extend(v for r in background(l) for v in r);cm.extend(v for r in l['collision_rle'] for v in r)
  coins.extend([round(v*65536) for v in c] for c in l['coins'])
  checks.extend([round(v*65536) for v in c['rect']+c['spawn']] for c in l['checkpoints'])
 array(f,'uint8_t','background_rle',bg);array(f,'uint8_t','collision_rle',cm)
 array(f,'Point','coins',coins);array(f,'Check','checks',checks);array(f,'Level','levels',levels)
with (G/'music.h').open('w') as f:
 array(f,'uint8_t','music',list((P/'reference/music-pokey.bin').read_bytes()))
runpy.run_path(str(P/'generate_font.py'))
print('30 original levels compiled:',len(d['tracks']),'matrix samples;',len(cm),'collision bytes')
