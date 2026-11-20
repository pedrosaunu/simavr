"""Strict four-case independent guest timing witness for NU-019.

Only validates what the modified reference simulator actually observed.
Does not claim that simavr defines the physical ATmega328P race priority.
"""
import hashlib
import json
import re
import struct
import sys

cases={}
current=None
for raw in open(sys.argv[1],encoding="utf-8"):
    line=raw.strip()
    if line.startswith("NU019_SELECTED_BEGIN "):
        if current is not None:
            raise ValueError("nested variant")
        m=dict(re.findall(r"(\w+)=([^\s]+)",line))
        current={"ocr":int(m["ocr"]),"nops":int(m["nops"]),
                 "sha256":m["sha256"],"events":[],"writes":[],"guest":[],
                 "words":None,"summary":None}
    elif line=="NU019_SELECTED_END":
        if current is None or not current["words"] or not current["summary"]:
            raise ValueError("Incomplete selected variant")
        data=struct.pack("<"+"H"*len(current["words"]),*current["words"])
        if hashlib.sha256(data).hexdigest()!=current["sha256"]:
            raise ValueError("Firmware SHA mismatch")
        key=(current["ocr"],current["nops"])
        if key in cases:raise ValueError("duplicate selected case")
        cases[key]=current
        current=None
    elif current is not None:
        if line.startswith("NU019_WORDS "):
            current["words"]=[int(x) for x in line.split()[1:]]
        elif line.startswith("NU019_REF_EVENT "):
            x=dict(re.findall(r"(\w+)=([^\s]+)",line))
            current["events"].append({"cycle":int(x["callback_when"]),
                                      "flag_before":int(x["flag_before"],16)})
        elif line.startswith("NU019_REF_WRITE "):
            x=dict(re.findall(r"(\w+)=([^\s]+)",line))
            if int(x["data"],16)==2:
                current["writes"].append({"cycle":int(x["cpu_cycle"]),
                                          "flag_before":int(x["flag_before"],16)})
        elif line.startswith("NU019_REF_GUEST "):
            current["guest"].append(line)
        elif line.startswith("NU019_REF_SUMMARY "):
            x=dict(re.findall(r"(\w+)=([^\s]+)",line))
            current["summary"]={p:int(x[p],16) for p in
                                ("r19_before","r20_after","r21_stopped","TIFR1")}
            current["summary"]["pass"]=int(x["pass"])

expected={(2,0):(-1,0),(3,0):(0,0),(4,0):(1,2),(4,1):(0,0)}
if set(cases)!=set(expected):
    raise SystemExit("Missing selected controls, expected "+str(sorted(expected)))
for key,(offset,after) in expected.items():
    case=cases[key]
    if len(case["writes"])!=1 or not case["events"]:
        raise ValueError("No unique W1C or compare source "+str(key))
    w=case["writes"][0]
    event=case["events"][0]
    if event["cycle"]-w["cycle"]!=offset:
        raise ValueError("Invalid event/write phase "+str((key,event,w)))
    if case["summary"]["r20_after"]!=after or case["summary"]["TIFR1"]!=after:
        raise ValueError("Guest IO status differs "+str((key,case["summary"])))
    if case["summary"]["pass"]!=1:
        raise ValueError("Guest did not finish "+str(key))
    out=dict(case)
    out["event_minus_write_cycle"]=offset
    out["expected_post_write_flag"]=after
    out["full_simavr_ref_not_silicon"]=True
    print("NU019_SELECTED_EVIDENCE "+json.dumps(
        out,sort_keys=True,separators=(",",":")))
print("NU019_SELECTED_4_CASES_REFERENCE_STRICT_PASS")
