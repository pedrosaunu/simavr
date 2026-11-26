"""Indirect Timer1 bus characterization: stopped and running WGM14.
Every addressing form accesses the same register, with an independent
avr-gcc binary. Running cases deliberately retain their own instruction phase.
"""
import json
from pathlib import Path

FORMS = ("direct","X","X+","-X","Y","Y+","-Y","Y+7","Z","Z+","-Z","Z+7")

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for cs in (0,1,2):
        for form in FORMS:
            cid=1000+len(cases)
            lines=[".text",".global start","start:"," cli"]
            def wr(a,v):
                lines.extend([f" ldi r16,{v}",f" sts {a},r16"])
            def pair(a,v):
                wr(a+1,v>>8);wr(a,v&255)
            def access(a,r,store):
                if form=="direct":
                    lines.append(f" sts {a},{r}" if store else f" lds {r},{a}")
                    return
                p=form.strip("-+7")
                lo={"X":26,"Y":28,"Z":30}[p]
                base=a+1 if form.startswith("-") else a-7 if form.endswith("+7") else a
                lines.extend([f" ldi r{lo},{base&255}",f" ldi r{lo+1},{base>>8}"])
                op=("std" if store else "ldd") if form.endswith("+7") else ("st" if store else "ld")
                lines.append(f" {op} {form},{r}" if store else f" {op} {r},{form}")
            lines.extend([" ldi r16,6"," out 4,r16"," ldi r16,2"," out 5,r16"])
            wr(0x81,24);pair(0x86,15);pair(0x88,7);pair(0x8a,3);pair(0x84,2)
            wr(0x80,0xa2);wr(0x81,24|cs)
            lines.append(" ldi r24,1")
            # High/low writes exercise TEMP and the buffered OCR transaction.
            lines.append(" ldi r16,0");access(0x8b,"r16",True)
            lines.append(" ldi r16,5");access(0x8a,"r16",True)
            lines.append(" ldi r24,2")
            lines.append(" ldi r16,0");access(0x85,"r16",True)
            lines.append(" ldi r16,4");access(0x84,"r16",True)
            access(0x84,"r21",False);access(0x85,"r22",False)
            # Capture the result of the read before subsequent instructions.
            lines.extend([" ldi r24,3"," in r23,5"," in r20,3"," ldi r25,90"," nop"])
            (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
            cases.append(dict(id=cid,form=form,cs=cs))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases

if __name__=="__main__":
    import sys
    generate(sys.argv[1])
