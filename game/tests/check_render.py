import ctypes as C,json
from pathlib import Path
from PIL import Image,ImageDraw
R=Path(__file__).resolve().parents[2];dll=C.CDLL(str(R/'build/game.so'));dll.hg_test_pixels.restype=C.POINTER(C.c_uint8)
cfg=json.loads((R/'game/display.json').read_text());W,H=cfg['screen_width'],cfg['screen_height'];panel=H*2+16
montage=Image.new('RGB',(5*W*2,6*panel),'black');draw=ImageDraw.Draw(montage)
colors=[(238,238,245),(93,79,154),(20,45,215),(155,235,145),(250,214,20)]
res=[]
for l in range(30):
 dll.hg_test_level(l);mx=0
 for t in range(6000):
  dll.hg_test_tick(t);mx=max(mx,dll.hg_test_glyphs());assert dll.hg_test_state()!=4,(l,t,'glyph overflow')
  if t==0:
   a=dll.hg_test_pixels();im=Image.new('RGB',(W,H));im.putdata([colors[a[i]] for i in range(W*H)]);im=im.resize((W*2,H*2));montage.paste(im,((l%5)*W*2,(l//5)*panel));draw.text(((l%5)*W*2+8,(l//5)*panel+H*2),str(l+1),fill='white')
 res.append(dict(level=l+1,max_glyphs=mx,hazards=dll.hg_test_hazards()));print(res[-1],flush=True)
(R/'validation').mkdir(exist_ok=True)
montage.save(R/'validation/original-30-levels.png');(R/'validation/glyph-budget.json').write_text(json.dumps(res,indent=2))
