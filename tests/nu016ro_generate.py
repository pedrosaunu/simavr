"""Unbuffered safe ICR1 TOP guest diagnostic, with W1C after each sample.

The existing simavr opt-in is count-only; flag and output differences are
diagnostic, not a qualified oracle and not grounds to fit the neural graph.
"""
from pathlib import Path
import json
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    specs=[(m,cs,new) for m in (8,10) for cs in (2,3) for new in (7,23)]
    for mode,cs,new in specs:
        com_a,com_b=2,3
        cid=1910+len(cases);top=15
        extreme=False
        a=3
        b=0 if extreme else 2
        initial=top-20 if mode in (1,2,3) else 0
        lines=[".text",".global start","start:"," cli"]
        def write(addr,v):lines.extend([f" ldi r16,{v}",f" sts {addr},r16"])
        def pair(addr,v):write(addr+1,v>>8);write(addr,v&255)
        write(0x43,0x81);write(0x24,6)
        write(0x81,(mode&12)<<1);write(0x80,(com_a<<6)|(com_b<<4)|(mode&3))
        pair(0x86,top);pair(0x88,a);pair(0x8a,b);pair(0x84,initial)
        write(0x36,39);write(0x81,((mode&12)<<1)|cs);write(0x43,0)
        if cs==2:
            lines.extend([" nop"]*3)
        pair(0x86,new)
        lines.extend([" ldi r18,32","sample:"]+[" nop"]*(24 if cs==3 else 0)+[" lds r21,0x84"," lds r22,0x85",
            " in r23,0x16"," in r20,0x03"," inc r24"," out 0x16,r23"," dec r18"," brne sample"," ldi r25,90"," nop"])
        (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
        cases.append(dict(id=cid,mode=mode,cs=cs,top=top,initial=initial,ocr_a=a,ocr_b=b,com_a=com_a,com_b=com_b,updated_icr=new))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
