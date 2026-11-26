"""Stable TOP/OCR/COM guest diagnostic, with W1C after each sample.

The existing simavr opt-in is count-only; flag and output differences are
diagnostic, not a qualified oracle and not grounds to fit the neural graph.
"""
from pathlib import Path
import json
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    specs=[(m,cs,2,3) for m in (1,2,3,8,9,10,11) for cs in (1,2)]
    specs += [(m,cs,1,2) for m in (9,11) for cs in (1,2)]
    specs += [(m,1,2,2) for m in (8,10)]
    for mode,cs,com_a,com_b in specs:
        cid=1600+len(cases);top={1:255,2:511,3:1023}.get(mode,7)
        extreme=cid>=1618
        a=top if mode in (9,11) or extreme else 3
        b=0 if extreme else 2
        initial=0
        lines=[".text",".global start","start:"," cli"]
        def write(addr,v):lines.extend([f" ldi r16,{v}",f" sts {addr},r16"])
        def pair(addr,v):write(addr+1,v>>8);write(addr,v&255)
        write(0x43,0x81);write(0x24,6)
        write(0x81,(mode&12)<<1);write(0x80,(com_a<<6)|(com_b<<4)|(mode&3))
        pair(0x86,top);pair(0x88,a);pair(0x8a,b);pair(0x84,initial)
        write(0x36,39);write(0x81,((mode&12)<<1)|cs);write(0x43,0)
        lines.extend([" ldi r18,32","sample:"," lds r21,0x84"," lds r22,0x85",
            " in r23,0x16"," in r20,0x03"," inc r24"," out 0x16,r23"," dec r18"," brne sample"," ldi r25,90"," nop"])
        (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
        cases.append(dict(id=cid,mode=mode,cs=cs,top=top,initial=initial,ocr_a=a,ocr_b=b,com_a=com_a,com_b=com_b))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
