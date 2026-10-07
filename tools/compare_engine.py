#!/usr/bin/env python3
"""Compare every timing-critical source/object section and linked function.
Requires pyelftools; arguments: upstream-dir baseline-build new-build output-dir.
"""
from pathlib import Path
import sys,hashlib,json,subprocess
from elftools.elf.elffile import ELFFile
up,old,new,out=map(Path,sys.argv[1:]);out.mkdir(parents=True,exist_ok=True)
root=Path(__file__).resolve().parents[1]
def elf(path):return ELFFile(open(path,'rb'))
a=elf(old/'a8_pico_cart.elf');b=elf(new/'a8_pico_cart.elf')
ao=elf(old/'CMakeFiles/a8_pico_cart.dir/atari_cart.c.obj');bo=elf(new/'CMakeFiles/a8_pico_cart.dir/atari_cart.c.obj')
def symbol(e,n):return e.get_section_by_name('.symtab').get_symbol_by_name(n)[0]
def content(e,n):
 s=symbol(e,n);sec=e.get_section(s['st_shndx']);start=(s['st_value']&~1)-sec['sh_addr'];return sec.data()[start:start+s['st_size']]
rows=[]
for s in ao.iter_sections():
 if not s.name.startswith('.time_critical.'):continue
 name=s.name.split('.',2)[2];av=content(a,name);bv=content(b,name);symA=symbol(a,name);symB=symbol(b,name)
 reloc=ao.get_section_by_name('.rel'+s.name)
 relo=[] if not reloc else [{'offset':r['r_offset'],'type':r['r_info_type']} for r in reloc.iter_relocations()]
 aa=bytearray(av);bb=bytearray(bv)
 for r in relo:
  # All RP2040 ARM absolute words / calls occupy 4 bytes here. Preserve other bytes.
  for k in range(r['offset'],min(r['offset']+4,len(aa),len(bb))):aa[k]=bb[k]=0
 rows.append(dict(function=name,size=len(av),source_section_identical=s.data()==bo.get_section_by_name(s.name).data(),baseline_address=hex(symA['st_value']&~1),new_address=hex(symB['st_value']&~1),linked_bytes_identical=av==bv,relocation_normalized_identical=aa==bb,relocations=relo,baseline_sha256=hashlib.sha256(av).hexdigest(),new_sha256=hashlib.sha256(bv).hexdigest()))
 # Save readable disassembly with absolute addresses for independent review.
 for title,directory in [('baseline',old),('new',new)]:
  d=subprocess.check_output(['arm-none-eabi-objdump','-d','--disassemble='+name,str(directory/'a8_pico_cart.elf')],text=True)
  (out/(title+'-'+name+'.dis')).write_text(d)
(out/'critical-comparison.json').write_text(json.dumps(rows,indent=2))
text='# Timing-critical release comparison\n\nGCC 13.2.Rel1; Pico SDK 1.5.1; Release (-O3). Baseline is the unchanged A8Duo HardGame v021 source rebuilt with the same tools (C linker selected for both builds).\n\n'
text+='| Function | Bytes | Object bytes identical | Linked bytes identical | Relocations normalized | SRAM address |\n|---|---:|---|---|---|---|\n'
for r in rows:text+=f"| `{r['function']}` | {r['size']} | {r['source_section_identical']} | {r['linked_bytes_identical']} | {r['relocation_normalized_identical']} | `{r['new_address']}` |\n"
text+='\nThe comparison includes literal pools. Relocation normalization zeros only locations declared by the compiler relocation table; it does not delete instructions or normalize arbitrary differences. Full disassemblies and SHA-256 values accompany this report. `atari_cart_main` changes at source and instruction level only to recognize CAS and call the outlined file-selection/OS-copy helpers. It is the menu dispatcher, not a cartridge polling routine; no equivalence is claimed for it. `emulate_microcalc` differs only in the relocated BL to the unchanged SDK division veneer; this bank-switch path needs scope validation, including XIP cache-cold behavior. The other 20 bus/loader routines are byte identical, including their literal pools.\n\nThis establishes equivalence to the controlled A8Duo v021 rebuild, **not** physical timing qualification. This CAS update changes no PCB or GPIO mapping. The original A8PicoCart published UF2 is a different baseline; no binary identity with it is claimed.\n'
(out/'TIMING-COMPARISON.md').write_text(text)
print(json.dumps(rows,indent=2))
assert all(r['source_section_identical'] for r in rows if r['function']!='atari_cart_main')
assert all(r['linked_bytes_identical'] for r in rows if r['function'] not in ['atari_cart_main','emulate_microcalc'])

assert all(r['relocation_normalized_identical'] for r in rows if r['function']!='atari_cart_main')
