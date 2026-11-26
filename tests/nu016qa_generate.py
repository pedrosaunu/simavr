"""Compact guest dual timer matrix; GTCCR OUT vs STS.
Separate from indirect-access timing hypotheses. Clock divisors are
selected by firmware; no host-generated peripheral advancement.
"""
import json
from pathlib import Path

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    for access in ("sts","out"):
      for cs0,cs1 in ((1,2),(2,1),(2,2),(3,2)):
        for kind in ("steady","hold_release","reset","stop0"):
            cid=1200+len(cases)
            lines=[".text",".global start","start:"," cli"]
            def write(addr,v):
                lines.extend([f" ldi r16,{v}",f" out {addr-0x20},r16" if addr==0x43 and access=="out" else f" sts {addr},r16"])
            def pair(addr,v):
                write(addr+1,v>>8);write(addr,v&255)
            def sample(tag):
                lines.extend([f" ldi r24,{tag}"," lds r17,0x46"," in r19,0x15"," lds r21,0x84"," lds r22,0x85"," in r23,0x16"])
            def delay(n):
                for _ in range(n//2):
                    label=f"delay_{len(lines)}"
                    lines.extend([f" rjmp {label}",f"{label}:"])
                if n%2:lines.append(" nop")
            write(0x44,0);write(0x45,0);write(0x46,250)
            write(0x81,24);pair(0x86,15);pair(0x88,7);pair(0x8a,3);pair(0x84,10);write(0x80,0xa2)
            write(0x43,0x81);write(0x45,cs0);write(0x81,24|cs1)
            write(0x43,0);delay(12);sample(1)
            if kind=="hold_release":
                write(0x43,0x81);sample(2);delay(12);sample(3);write(0x43,0)
            elif kind=="reset":
                write(0x43,1);sample(2)
            elif kind=="stop0":
                write(0x45,0);sample(2)
            else:
                sample(2)
            delay(20);sample(4);lines.extend([" ldi r25,90"," nop"])
            (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
            cases.append(dict(id=cid,cs0=cs0,cs1=cs1,kind=kind,gtccr_access=access))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases

if __name__=="__main__":
    import sys
    generate(sys.argv[1])
