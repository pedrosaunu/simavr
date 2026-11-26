"""Timer0 normal-mode guest compare probes, shared-clock ABI.
Timer0 starts while held; stable OCR values are set before release.
All observations are made by AVR guest instructions, not host injection.
"""
import json
from pathlib import Path

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    for cs in (1,2):
        for initial,a,b in ((250,253,255),(254,255,0),(0,0,2),(6,9,9)):
            for clear in (1,6):
                cid=1300+len(cases)
                lines=[".text",".global start","start:"," cli"]
                def write(addr,v):
                    lines.extend([f" ldi r16,{v}",f" sts {addr},r16"])
                def sample(tag):
                    lines.extend([f" ldi r24,{tag}"," lds r17,0x46"," in r19,0x15"])
                def delay(n):
                    for _ in range(n//2):
                        label=f"delay_{len(lines)}"
                        lines.extend([f" rjmp {label}",f"{label}:"])
                write(0x43,0x81);write(0x44,0);write(0x45,0)
                write(0x47,a);write(0x48,b);write(0x46,initial);write(0x45,cs)
                sample(1);write(0x43,0);delay(16 if cs==1 else 48);sample(2)
                write(0x43,0x81);sample(3);write(0x35,clear);sample(4)
                lines.extend([" ldi r25,90"," nop"])
                (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
                cases.append(dict(id=cid,cs=cs,initial=initial,ocr_a=a,ocr_b=b,clear=clear))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases

if __name__=="__main__":
    import sys
    generate(sys.argv[1])
