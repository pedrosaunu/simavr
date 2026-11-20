"""Strict independent ICP1 external-edge sweep, software reference only."""
import hashlib
import json
import re
import struct
import sys

def terms(line):return dict(re.findall(r"(\w+)=([^\s]+)",line))
rows=[];current=None
for raw in open(sys.argv[1],encoding="utf-8"):
 line=raw.strip()
 if line.startswith("NU019C_CASE_BEGIN "):
  if current is not None:raise ValueError("nested")
  x=terms(line);current=dict(nops=int(x["nops"]),delay=int(x["delay"]),
       firmware_sha256=x["sha256"],words=None,events=[],writes=[],
       guest=[],summary=None,external_pin=None)
 elif line=="NU019C_CASE_END":
  if current is None or current["words"] is None or current["summary"] is None:
   raise ValueError("incomplete")
  b=struct.pack("<"+"H"*len(current["words"]),*current["words"])
  if hashlib.sha256(b).hexdigest()!=current["firmware_sha256"]:
   raise ValueError("AVR words do not match independently compiled binary")
  if current["summary"]["done"]!=1:raise ValueError("guest not done")
  if current["delay"]>=0 and (len(current["events"])!=1 or
     current["external_pin"] is None or current["summary"]["injected"]!=1):
   raise ValueError("External capture did not occur exactly once")
  if current["delay"]<0 and (current["events"] or current["external_pin"]):
   raise ValueError("Negative control received an unexpected capture")
  if len(current["writes"])!=2:
   raise ValueError("Expected old-clear + experimental W1C")
  rows.append(current);current=None
 elif current is not None:
  if line.startswith("NU019C_WORDS "):
   current["words"]=[int(x) for x in line.split()[1:]]
  elif line.startswith("NU019C_REF_EVENT "):
   x=terms(line);current["events"].append(dict(
     cycle=int(x["cpu_cycle"]),capture=int(x["capture"],16),
     flag_before=int(x["flag_before"],16)))
  elif line.startswith("NU019_REF_WRITE "):
   x=terms(line)
   if int(x["data"],16)==0x20:current["writes"].append(dict(
       cycle=int(x["cpu_cycle"]),flag_before=int(x["flag_before"],16)))
  elif line.startswith("NU019C_EXTERNAL_PIN "):
   x=terms(line);current["external_pin"]=dict(cycle=int(x["cycle"]),pinb=int(x["pinb0"]))
  elif line.startswith("NU019C_REF_GUEST "):
   current["guest"].append(line)
  elif line.startswith("NU019C_REF_SUMMARY "):
   x=terms(line);current["summary"]={key:int(x[key]) for key in
       ("steps","done","injected","ready_cycle","injection_cycle")}
   current["summary"].update({key:int(x[key],16) for key in
       ("r19","r20","r21","r22","r23","tifr1")})
if current is not None:raise ValueError("unterminated")
if len(rows)!=31:raise ValueError(f"Expected 31 cases, found {len(rows)}")
keys={(r["nops"],r["delay"]) for r in rows}
if keys!={(n,d) for n in range(6) for d in range(5)}|{(2,-1)}:
 raise ValueError("missing or duplicate sweep cases")
for r in rows:
 if [int(terms(x)["step"]) for x in r["guest"]]!=list(range(1,len(r["guest"])+1)):
  raise ValueError("missing guest boundaries")
 if r["summary"]["steps"]!=len(r["guest"]):
  raise ValueError("guest step count inconsistent")
 if r["delay"]>=0:
  if r["events"][0]["capture"]!=0x1234:
   raise ValueError("capture engine read wrong TCNT1 for "+str((r["nops"],r["delay"])))
  # An edge occurring AFTER the guest's LDS ICR1L may correctly leave
  # r21=0; an edge between its two 16-bit reads may expose mixed
  # readback. Do not demand a retroactive guest read after an event.
  if r["summary"]["r21"] not in (0,0x34) or r["summary"]["r22"] not in (0,0x12):
   raise ValueError("unexpected guest readback for "+str((r["nops"],r["delay"])))
  # If the event was visible by the W1C instruction, both ICR reads
  # necessarily occur later and must see the full captured value.
  if r["events"][0]["cycle"] <= r["writes"][-1]["cycle"] and (
       r["summary"]["r21"],r["summary"]["r22"])!=(0x34,0x12):
   raise ValueError("early event failed complete guest capture readback")
 else:
  if r["summary"]["r21"] or r["summary"]["r22"] or r["summary"]["r23"]&0x20:
   raise ValueError("control unexpectedly captured")
 w=r["writes"][-1]
 r["event_minus_clear_cycle"]=r["events"][0]["cycle"]-w["cycle"] if r["events"] else None
 r["post_write_flag"]=r["summary"]["r20"]&0x20
 print("NU019C_CASE_RESULT "+json.dumps({
  k:r[k] for k in ("nops","delay","firmware_sha256","event_minus_clear_cycle",
   "post_write_flag","summary","external_pin")},sort_keys=True))
same=[r for r in rows if r["event_minus_clear_cycle"]==0]
before=[r for r in rows if r["event_minus_clear_cycle"] is not None and r["event_minus_clear_cycle"]<0]
after=[r for r in rows if r["event_minus_clear_cycle"] is not None and r["event_minus_clear_cycle"]>0]
summary=dict(schema="weightos.nu019c.simavr-pin-edge-sweep/v1",
 status="OBSERVATIONAL_MODIFIED_SIMAVR_NOT_SILICON",firmware_variants=len(rows),
 external_pin_events=sum(bool(x["events"]) for x in rows),
 equal_reported_cycles=len(same),
 early_events=len(before),late_events=len(after),
 same_cases=[dict(nops=x["nops"],delay=x["delay"],sha=x["firmware_sha256"],
   post_w1c=x["post_write_flag"]) for x in same],
 no_edge_negative_control=True,whole_timer1_qualified=False,
 silicon_same_edge_priority_qualified=False)
print("NU019C_SWEEP_SUMMARY "+json.dumps(summary,sort_keys=True))
if not before or not after:
 raise ValueError("Sweep missed before or after event coverage")
