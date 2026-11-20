"""NU-019D independent running ICP1 repeated-event reference.

Observes real guest and genuine simavr input capture; no host injection
beyond the PB0 electrical input. Qualifies neither neural nor silicon.
"""
from __future__ import annotations
import hashlib,json,re,struct,sys
names=("none","single","split","overlap","late","stress")
expected_edges={"none":0,"single":1,"split":2,"overlap":2,"late":2,"stress":3}
def parse(line):return dict(re.findall(r"(\w+)=([^\s]+)",line))
cases=[];current=None
for line in open(sys.argv[1],encoding="utf-8"):
 line=line.strip()
 if line.startswith("NU019D_CASE_BEGIN "):
  if current is not None:raise ValueError("nested case")
  x=parse(line);current={"mode":x["mode"],"pad":int(x["pad"]),"sha256":x["sha256"],
   "words":None,"pin":[],"guest":[],"events":[],"writes":[],"summary":None}
 elif line=="NU019D_CASE_END":
  if current is None:raise ValueError("ending nonexistent case")
  if current["words"] is None or not current["summary"]:raise ValueError("incomplete case")
  guest=struct.pack("<"+"H"*len(current["words"]),*current["words"])
  if hashlib.sha256(guest).hexdigest()!=current["sha256"]:
   raise ValueError("compiled binary / words SHA mismatch")
  if current["summary"]["ready"]!=1 or current["summary"]["done"]!=1:
   raise ValueError("guest did not reach completion marker")
  if current["summary"]["tccr1b"]!=0x41:raise ValueError("timer was not running")
  if len(current["events"])!=expected_edges[current["mode"]]:
   raise ValueError("unexpected external ICP1 callback count: "+repr((current["mode"],current["pad"],current["events"])))
  if len(current["pin"])!=current["summary"]["transitions"]:
   raise ValueError("pin transition bookkeeping inconsistent")
  if current["summary"]["steps"]!=len(current["guest"]):
   raise ValueError("guest instruction boundary loss")
  if [g["step"] for g in current["guest"]]!=list(range(1,len(current["guest"])+1)):
   raise ValueError("nonconsecutive guest boundaries")
  if current["mode"]=="none":
   if current["pin"] or current["events"]:
    raise ValueError("no-pin control unexpectedly generated capture")
  else:
   levels=[x["level"] for x in current["pin"]]
   if levels[0]!=1 or current["summary"]["transitions"] not in (1,3,5):
    raise ValueError("invalid PB0 pin waveform")
   captures=[x["capture"] for x in current["events"]]
   if not all(x>0 for x in captures) or len(set(captures))!=len(captures):
    raise ValueError("repeated running captures not distinct and nonzero")
   if captures!=sorted(captures):
    raise ValueError("timer counter did not advance monotonically")
  current["guest_boundaries"]=len(current["guest"])
  current["capture_values"]=[x["capture"] for x in current["events"]]
  current["simavr_reference_only"]=True
  cases.append(current);current=None
 elif current is not None:
  if line.startswith("NU019D_WORDS "):
   current["words"]=[int(v) for v in line.split()[1:]]
  elif line.startswith("NU019D_PIN "):
   x=parse(line);current["pin"].append({"step":int(x["step"]),
       "cycle":int(x["cycle"]),"level":int(x["level"]),"age":int(x["age"])})
  elif line.startswith("NU019D_GUEST "):
   x=parse(line)
   current["guest"].append({"step":int(x["step"]),"pc":int(x["pc"],16),
       "cycles":int(x["cycles"]),
       **{"r"+str(i):int(x["r"+str(i)],16) for i in range(18,26)},
       "tifr1":int(x["tifr1"],16),"tccr1b":int(x["tccr1b"],16),
       "level":int(x["level"]),"ready":int(x["ready"])})
  elif line.startswith("NU019C_REF_EVENT "):
   x=parse(line);current["events"].append({"cycle":int(x["cpu_cycle"]),
       "capture":int(x["capture"],16),"flag_before":int(x["flag_before"],16)})
  elif line.startswith("NU019_REF_WRITE "):
   x=parse(line)
   if int(x["data"],16)==0x20:
    current["writes"].append({"cycle":int(x["cpu_cycle"]),
      "flag_before":int(x["flag_before"],16)})
  elif line.startswith("NU019D_SUMMARY "):
   x=parse(line)
   current["summary"]={**{k:int(x[k]) for k in ("ready","done","steps","transitions")},
    **{k:int(x[k],16) for k in ["r"+str(i) for i in range(18,26)]+["tifr1","tccr1b"]}}
if current is not None:raise ValueError("unterminated case")
if len(cases)!=18 or {(x["mode"],x["pad"]) for x in cases}!={(m,p) for m in names for p in (0,2,4)}:
 raise ValueError("missing or duplicated 18 guest programs")
for c in cases:
 # Railway log records may truncate long lines. Keep every individual
 # evidence record below 900 bytes and reject incomplete chunk sets
 # during WeightOS import. The guest trace is never downsampled.
 meta={k:v for k,v in c.items() if k not in ("guest","words")}
 print("NU019D_CASE_META "+json.dumps(meta,sort_keys=True,separators=(",",":")))
 for i in range(0,len(c["words"]),32):
  part={"mode":c["mode"],"pad":c["pad"],"part":i//32,"words":c["words"][i:i+32]}
  print("NU019D_CASE_WORDS "+json.dumps(part,sort_keys=True,separators=(",",":")))
 for i in range(0,len(c["guest"]),3):
  part={"mode":c["mode"],"pad":c["pad"],"part":i//3,"guest":c["guest"][i:i+3]}
  encoded=json.dumps(part,sort_keys=True,separators=(",",":"))
  if len(encoded)>950:raise ValueError("guest log chunk exceeds safe Railway length")
  print("NU019D_CASE_TRACE "+encoded)
total=sum(c["guest_boundaries"] for c in cases)
summary={"schema":"weightos.nu019d.running-repeated-icp1-reference/v1",
 "status":"INDEPENDENT_MODIFIED_SIMAVR_GUEST_SOFTWARE_REFERENCE_ONLY",
 "case_count":len(cases),"unique_binary_hashes":len(set(c["sha256"] for c in cases)),
 "external_capture_events":sum(len(c["events"]) for c in cases),
 "selected_instruction_boundaries":total,"modes":list(names),
 "physical_edge_priority_qualified":False,"neural_differential_qualified":False,
 "full_timer1_qualified":False}
print("NU019D_SWEEP_SUMMARY "+json.dumps(summary,sort_keys=True))
