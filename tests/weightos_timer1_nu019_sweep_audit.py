#!/usr/bin/env python3
"""NU-019 observational timing sweep. Does NOT assert silicon event priority.

Analyze independent simavr timer callback logs captured while actual AVR
programs execute from reset. No flags/PC/CPU cycles are injected from host.
"""
import json
import re
import sys

lines=open(sys.argv[1],encoding="utf-8").read().splitlines()
cases=[]
current=None
for line in lines:
    if line.startswith("NU019_CASE_BEGIN "):
        if current is not None:
            raise ValueError("nested case marker")
        meta=dict(re.findall(r"(\w+)=([^\s]+)",line))
        current=dict(ocr=int(meta["ocr"]),nops=int(meta["nops"]),
                     firmware_sha256=meta["sha256"],events=[],writes=[],
                     guest=[],summary=None)
    elif line.startswith("NU019_CASE_END"):
        if current is None or current["summary"] is None:
            raise ValueError("case ended without guest summary")
        cases.append(current)
        current=None
    elif current is not None:
        if line.startswith("NU019_REF_EVENT "):
            m=dict(re.findall(r"(\w+)=([^\s]+)",line))
            current["events"].append(dict(event_cycle=int(m["callback_when"]),
                                        exec_cycle=int(m["cpu_cycle"]),
                                        flag_before=int(m["flag_before"],16)))
        elif line.startswith("NU019_REF_WRITE "):
            m=dict(re.findall(r"(\w+)=([^\s]+)",line))
            current["writes"].append(dict(cycle=int(m["cpu_cycle"]),
                                         data=int(m["data"],16),
                                         flag_before=int(m["flag_before"],16)))
        elif line.startswith("NU019_REF_GUEST "):
            current["guest"].append(line)
        elif line.startswith("NU019_REF_SUMMARY "):
            m=dict(re.findall(r"(\w+)=([^\s]+)",line))
            current["summary"]={
               "r19_before":int(m["r19_before"],16),
               "r20_after":int(m["r20_after"],16),
               "r21_stopped":int(m["r21_stopped"],16),
               "TIFR1":int(m["TIFR1"],16),
               "pass":int(m["pass"]),
               "step_count":int(m["steps"]),
            }
if current is not None:
    raise ValueError("unterminated case marker")

for case in cases:
    if case["summary"]["pass"] != 1:
        raise ValueError("Guest failed: "+str((case["ocr"],case["nops"])))
    # Only the actual OUT 0x02 is the collision candidate; an initial
    # OUT 0x06 also runs to clear any previous flags.
    clears=[w for w in case["writes"] if w["data"]==2]
    if len(clears)!=1:
        raise ValueError("Guest W1C occurrence mismatch: "+str(case))
    w=clears[0]
    case["clear_cycle"]=w["cycle"]
    case["compare_event_cycles"]=[e["event_cycle"] for e in case["events"]]
    case["same_cycle_event"]=any(e["event_cycle"]==w["cycle"]
                                 for e in case["events"])
    case["nearest_event_cycle_distance"]= (
      min(abs(e["event_cycle"]-w["cycle"]) for e in case["events"])
      if case["events"] else None)
    case["w1c_flags_before"]=w["flag_before"]

collided=[case for case in cases if case["same_cycle_event"]]
neighbors=sorted([c for c in cases if c["nearest_event_cycle_distance"] is not None],
                 key=lambda x:(x["nearest_event_cycle_distance"],x["ocr"],x["nops"]))
summary={
 "schema":"nu019.simavr-ocf1a-w1c-phase-sweep/v1",
 "status":"OBSERVATIONAL_REFERENCE_ONLY_NOT_SILICON",
 "case_count":len(cases),"collisions":len(collided),
 "same_cycle_cases":[{k:x[k] for k in ("ocr","nops","firmware_sha256",
                       "clear_cycle","compare_event_cycles",
                       "w1c_flags_before","summary")} for x in collided],
 "nearest_cases":[{k:x[k] for k in ("ocr","nops","firmware_sha256",
                       "clear_cycle","nearest_event_cycle_distance",
                       "compare_event_cycles","summary")} for x in neighbors[:8]],
 "modifies_guest_state":False,
 "physical_silicon_confirmed":False,
 "full_timer1_qualified":False,
}
for case in cases:
    print("NU019_CASE_RESULT "+json.dumps({
      k:case[k] for k in ("ocr","nops","firmware_sha256",
                         "clear_cycle","same_cycle_event",
                         "nearest_event_cycle_distance","compare_event_cycles",
                         "w1c_flags_before","summary")
    },sort_keys=True,separators=(",",":")))
print("NU019_PHASE_SWEEP_SUMMARY "+json.dumps(summary,sort_keys=True,separators=(",",":")))
if len(cases)!=81:
    raise SystemExit("Expected 81 guest variants (OCR=0..8, NOP=0..8)")
if not neighbors:
    raise SystemExit("No hardware timer compare event observed by independent simavr")
