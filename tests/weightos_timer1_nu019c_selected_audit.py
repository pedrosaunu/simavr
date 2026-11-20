"""Six SHA-pinned real AVR programs/stimuli for independent ICF1/W1C.

Three relative event orderings; one late edge after guest ICR low read,
with datasheet-required TEMP high=0 even though underlying ICR1H=0x12,
one second equal-cycle guest binary, and an explicit no-edge negative.
No external-state injection other than real PB0 pin stimulus.
"""
from __future__ import annotations
import hashlib
import json
import re
import struct
import sys
def fields(s):return dict(re.findall(r"(\w+)=([^\s]+)",s))
cases={};case=None
for source in open(sys.argv[1],encoding="utf-8"):
 line=source.strip()
 if line.startswith("NU019C_SELECTED_BEGIN "):
  if case:raise ValueError("nested")
  v=fields(line);case=dict(nops=int(v["nops"]),delay=int(v["delay"]),
    sha256=v["sha256"],words=None,guest=[],events=[],writes=[],summary=None,external_pin=None)
 elif line=="NU019C_SELECTED_END":
  if case is None or not case["summary"] or not case["guest"] or case["words"] is None:
   raise ValueError("incomplete selected")
  data=struct.pack("<"+"H"*len(case["words"]),*case["words"])
  if hashlib.sha256(data).hexdigest()!=case["sha256"]:raise ValueError("guest binary SHA mismatch")
  key=(case["nops"],case["delay"])
  if key in cases:raise ValueError("duplicate")
  cases[key]=case;case=None
 elif case is not None:
  if line.startswith("NU019C_WORDS "):case["words"]=[int(x) for x in line.split()[1:]]
  elif line.startswith("NU019C_REF_EVENT "):
   v=fields(line);case["events"].append(dict(cycle=int(v["cpu_cycle"]),
    flag_before=int(v["flag_before"],16),capture=int(v["capture"],16)))
  elif line.startswith("NU019_REF_WRITE "):
   v=fields(line)
   if int(v["data"],16)==32:
    case["writes"].append(dict(cycle=int(v["cpu_cycle"]),
     flag_before=int(v["flag_before"],16)))
  elif line.startswith("NU019C_EXTERNAL_PIN "):
   v=fields(line);case["external_pin"]=dict(cycle=int(v["cycle"]),pinb=int(v["pinb0"]))
  elif line.startswith("NU019C_REF_GUEST "):case["guest"].append(line)
  elif line.startswith("NU019C_REF_SUMMARY "):
   v=fields(line);case["summary"]={k:int(v[k]) for k in ("steps","done","injected","ready_cycle","injection_cycle")}
   case["summary"].update({k:int(v[k],16) for k in ("r19","r20","r21","r22","r23","tifr1")})
if case is not None:raise ValueError("unterminated")
# Key = (number of firmware NOPs, external pin scheduling delay)
expected={
 (0,0):(-1,0,0x34,0x12,0),
 (0,1):(0,0,0x34,0x12,0),
 (0,2):(1,0x20,0x34,0x12,0x20),
 (0,4):(4,0,0,0,0x20),
 (2,3):(0,0,0x34,0x12,0),
 (2,-1):(None,0,0,0,0),
}
if set(cases)!=set(expected):raise ValueError("selected case set incomplete")
for key,values in expected.items():
 c=cases[key]
 diff,after,low,high,final=values
 if c["summary"]["done"]!=1 or len(c["writes"])!=2 or c["summary"]["steps"]!=len(c["guest"]):
  raise ValueError("guest or W1C stream incomplete")
 if c["summary"]["r20"]!=after or c["summary"]["r21"]!=low or c["summary"]["r22"]!=high or c["summary"]["r23"]!=final:
  raise ValueError("guest GPR mismatched for "+str(key))
 if diff is None:
  if c["events"] or c["external_pin"] or c["summary"]["injected"]!=0:
   raise ValueError("no-edge control mutated")
 else:
  if len(c["events"])!=1 or not c["external_pin"] or c["events"][0]["capture"]!=0x1234:
   raise ValueError("not exactly one genuine ICF1 capture")
  if c["events"][0]["cycle"]-c["writes"][-1]["cycle"]!=diff:
   raise ValueError("wrong external ICF1/W1C scheduling")
 if [int(fields(x)["step"]) for x in c["guest"]]!=list(range(1,len(c["guest"])+1)):
  raise ValueError("guest instruction trace incomplete")
 if key==(0,4):
  # Original simavr reference returned 0x12 for ICR1H, contrary to
  # ATmega328P datasheet §§15.3/15.6. Here ICR1L was read before
  # the capture; the subsequent ICR1H must return the old TEMP=0x00.
  if c["summary"]["r22"]!=0:
   raise ValueError("late capture violated shared Timer1 TEMP latch")
 output=dict(c,event_minus_w1c_cycle=diff,modified_simavr_only=True,
       physical_same_edge_priority_qualified=False)
 print("NU019C_SELECTED_EVIDENCE "+json.dumps(output,sort_keys=True,separators=(",",":")))
print("NU019C_SELECTED_6_REAL_GUESTS_STRICT_PASS")
