#!/usr/bin/env python3
"""Three POKEY tone voices reduced from the original reference recording.
This is a spectral chiptune reduction, not PCM playback or a note-perfect score.
Input: decoded mono 11025 Hz WAV from original SWF sound 49.
"""
from pathlib import Path
import wave,sys
import numpy as np
from scipy.signal import find_peaks
P=Path(__file__).resolve().parent
with wave.open(sys.argv[1]) as f:rate=f.getframerate();a=np.frombuffer(f.readframes(f.getnframes()),dtype='<i2').astype(float)/32768
assert rate==11025
N=4096;window=np.hanning(N);freq=np.fft.rfftfreq(N,1/rate);rows=[]
for t in range(int(len(a)/rate*30)):
 start=round(t*rate/30);chunk=np.zeros(N);part=a[start:start+N];chunk[:len(part)]=part
 spec=np.abs(np.fft.rfft(chunk*window));ix,_=find_peaks(spec)
 ix=[i for i in ix if 130<freq[i]<1700];ix.sort(key=lambda i:spec[i],reverse=True)
 notes=[]
 for i in ix:
  f=freq[i]
  if any(abs(np.log2(f/ff))<.12 for ff,v in notes):continue
  notes.append((f,spec[i]))
  if len(notes)==3:break
 notes.sort();rows.append([v for f,amp in notes for v in [int(np.clip(round(1789773/(56*f)-1),0,255)),0xA0+int(np.clip(round(amp/14),1,5))]] if len(notes)==3 else [0]*6)
# Persist small machine-independent AUDF/AUDC table; source MP3 not distributed.
out=P/'reference/music-pokey.bin';out.write_bytes(bytes(v for row in rows for v in row))
print(len(rows),'30 Hz rows;',out.stat().st_size,'bytes')
