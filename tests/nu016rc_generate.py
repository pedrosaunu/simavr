"""Count-only guest diagnostic for all seven Timer1 dual-slope modes.

Stable TOP and OCR, held configuration, /1 or /8, near-TOP start for fixed
width and BOTTOM start for variable width. These probes establish evidence,
not an assertion that the simulator is a silicon timing oracle.
"""
from pathlib import Path
import json

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    for mode in (1,2,3,8,9,10,11):
        for cs in (1,2):
            cid=1500+len(cases)
            top={1:255,2:511,3:1023}.get(mode,7)
            initial=top-3 if mode in (1,2,3) else 0
            a=top if mode in (9,11) else top-2
            b=2
            lines=[".text",".global start","start:"," cli"]
            def write(addr,v):lines.extend([f" ldi r16,{v}",f" sts {addr},r16"])
            def pair(addr,v):write(addr+1,v>>8);write(addr,v&255)
            write(0x43,0x81);write(0x24,6)
            write(0x81,(mode&12)<<1);write(0x80,mode&3)
            pair(0x86,top);pair(0x88,a);pair(0x8a,b);pair(0x84,initial)
            write(0x81,((mode&12)<<1)|cs);write(0x43,0)
            lines.extend([" ldi r18,32","sample:"," lds r21,0x84"," lds r22,0x85",
                " in r23,0x16"," in r20,0x03"," inc r24"," dec r18"," brne sample"," ldi r25,90"," nop"])
            (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
            cases.append(dict(id=cid,mode=mode,cs=cs,top=top,initial=initial,ocr_a=a,ocr_b=b))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases

if __name__=="__main__":
    import sys
    generate(sys.argv[1])
