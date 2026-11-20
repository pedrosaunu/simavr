"""NU-019T independent TOV1 guest witness: SHA-pinned and fail-closed.

Real AVR from reset; host simavr observes but does not inject CPU/Timer1.
This is a modified-simulator SOFTWARE timing witness, not chip priority.
"""
from __future__ import annotations
import hashlib
import json
import re
import struct
import sys

cases={}
current=None
for line in open(sys.argv[1],encoding="utf-8"):
 line=line.strip()
 if line.startswith("NU019T_SELECTED_BEGIN "):
  if current is not None:raise ValueError("nested reference")
  x=dict(re.findall(r"(\w+)=([^\s]+)",line))
  current={"low":int(x["low"]),"nops":int(x["nops"]),"sha256":x["sha256"],
           "words":None,"events":[],"writes":[],"guest":[],"summary":None}
 elif line=="NU019T_SELECTED_END":
  if current is None or current["words"] is None or current["summary"] is None:
   raise ValueError("incomplete reference")
  raw=struct.pack("<"+"H"*len(current["words"]),*current["words"])
  if hashlib.sha256(raw).hexdigest()!=current["sha256"]:
   raise ValueError("independent AVR SHA-256 differs from binary words")
  key=current["low"],current["nops"]
  if key in cases:raise ValueError("duplicate reference")
  cases[key]=current
  current=None
 elif current is not None:
  if line.startswith("NU019T_WORDS "):
   current["words"]=[int(x) for x in line.split()[1:]]
  elif line.startswith("NU019T_REF_EVENT "):
   x=dict(re.findall(r"(\w+)=([^\s]+)",line))
   current["events"].append({"cycle":int(x["callback_when"]),
                              "flag_before":int(x["flag_before"],16)})
  elif line.startswith("NU019_REF_WRITE "):
   x=dict(re.findall(r"(\w+)=([^\s]+)",line))
   if int(x["data"],16)==1:
    current["writes"].append({"cycle":int(x["cpu_cycle"]),
                              "flag_before":int(x["flag_before"],16)})
  elif line.startswith("NU019T_GUEST "):
   current["guest"].append(line)
  elif line.startswith("NU019T_SUMMARY "):
   x=dict(re.findall(r"(\w+)=([^\s]+)",line))
   current["summary"]={"r19_before":int(x["r19_before"],16),
     "r20_after":int(x["r20_after"],16),"r21_stopped":int(x["r21_stopped"],16),
     "tifr1":int(x["TIFR1"],16),"stopped":int(x["stopped"])}
if current is not None:raise ValueError("unterminated reference")

# One before, one after, two equal reported cycles. The values are
# independent-observed expectations, not a fabricated silicon oracle.
expected={(253,0):(1,1),(253,1):(0,0),(253,2):(-1,0),(254,0):(0,0)}
if set(cases)!=set(expected):raise ValueError("Missing exact four references")
for key,(offset,after) in sorted(expected.items()):
 c=cases[key]
 if len(c["writes"])!=1 or not c["events"] or not c["guest"]:
  raise ValueError("missing event/write/guest evidence: "+str(key))
 if c["events"][0]["cycle"]-c["writes"][0]["cycle"]!=offset:
  raise ValueError("event/write ordering mismatch: "+str(key))
 if c["summary"]["r20_after"]!=after or c["summary"]["r21_stopped"]!=after or c["summary"]["tifr1"]!=after:
  raise ValueError("guest-visible TOV1 differs: "+str(key))
 if c["summary"]["stopped"]!=1:
  raise ValueError("guest never stopped: "+str(key))
 if len(c["words"])<50 or len(c["words"])>75:
  raise ValueError("unexpected firmware size: "+str(key))
 output=dict(c,event_minus_w1c_cycle=offset,
   hardware_same_edge_qualified=False,simavr_modified=True)
 print("NU019T_SELECTED_EVIDENCE "+json.dumps(output,sort_keys=True,separators=(",",":")))
print("NU019T_SELECTED_4_REAL_AVR_GUESTS_STRICT_PASS")
