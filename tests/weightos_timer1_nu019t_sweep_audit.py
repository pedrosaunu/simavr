"""NU-019T guest-created TOV1 W1C timing sweep; NOT silicon priority."""
from __future__ import annotations
import json
import re
import sys

cases=[]
pending=None
for line in open(sys.argv[1],encoding="utf-8"):
 line=line.strip()
 if line.startswith("NU019T_CASE_BEGIN "):
  if pending is not None:raise ValueError("nested")
  p=dict(re.findall(r"(\w+)=([^\s]+)",line))
  pending={"low":int(p["low"]),"nops":int(p["nops"]),"sha256":p["sha256"],
           "events":[],"writes":[],"guest":[],"summary":None}
 elif line=="NU019T_CASE_END":
  if pending is None or pending["summary"] is None:raise ValueError("missing summary")
  cases.append(pending);pending=None
 elif pending is not None:
  if line.startswith("NU019T_REF_EVENT "):
   p=dict(re.findall(r"(\w+)=([^\s]+)",line))
   pending["events"].append({"cycle":int(p["callback_when"]),"flag_before":int(p["flag_before"],16)})
  elif line.startswith("NU019_REF_WRITE "):
   p=dict(re.findall(r"(\w+)=([^\s]+)",line))
   if int(p["data"],16)==1:
    pending["writes"].append({"cycle":int(p["cpu_cycle"]),"flag_before":int(p["flag_before"],16)})
  elif line.startswith("NU019T_GUEST "):
   pending["guest"].append(line)
  elif line.startswith("NU019T_SUMMARY "):
   p=dict(re.findall(r"(\w+)=([^\s]+)",line))
   pending["summary"]={"r19_before":int(p["r19_before"],16),
          "r20_after":int(p["r20_after"],16),"r21_stopped":int(p["r21_stopped"],16),
          "tifr1":int(p["TIFR1"],16),"stopped":int(p["stopped"])}
if pending is not None or len(cases)!=81:raise ValueError("expected 81 complete TOV1 variants")
for c in cases:
 if c["summary"]["stopped"]!=1 or len(c["writes"])!=1:
  raise ValueError("guest/W1C invalid: "+str((c["low"],c["nops"])))
 w=c["writes"][0]
 c["write_cycle"]=w["cycle"]
 c["event_cycles"]=[e["cycle"] for e in c["events"]]
 c["same_cycle"]=w["cycle"] in c["event_cycles"]
 c["nearest_delta"]=(min((e-w["cycle"] for e in c["event_cycles"]),
                         key=abs) if c["event_cycles"] else None)
 print("NU019T_CASE_RESULT "+json.dumps({
  k:c[k] for k in ("low","nops","sha256","write_cycle",
     "event_cycles","same_cycle","nearest_delta","summary")
 },sort_keys=True))
similar=[x for x in cases if x["nearest_delta"] is not None]
summ={"status":"INDEPENDENT_MODIFIED_SIMAVR_OBSERVATIONAL_REFERENCE_ONLY",
 "case_count":81,"seen_events":len(similar),
 "same_reported_cycle_count":sum(x["same_cycle"] for x in cases),
 "same_cycle_cases":[{"low":x["low"],"nops":x["nops"],
     "sha256":x["sha256"],"w1c_before":x["writes"][0]["flag_before"],
     "read_after":x["summary"]["r20_after"]} for x in cases if x["same_cycle"]],
 "closest_cases":[{"low":x["low"],"nops":x["nops"],
   "delta":x["nearest_delta"],"after":x["summary"]["r20_after"]} for x in similar
   if abs(x["nearest_delta"])<=1][:24],
 "physical_silicon_qualified":False,"full_timer1_qualified":False}
print("NU019T_SWEEP_SUMMARY "+json.dumps(summ,sort_keys=True))
if len(similar)<10:raise SystemExit("Insufficient real TOV1 event coverage")
