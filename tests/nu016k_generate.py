"""Deterministic guest ROM stimuli for running OCR commits, no oracle logic."""
import json
from pathlib import Path

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,7),(15,7)]:
        for phase in (0,2):
            cases.append(dict(mode=mode,top=top,phase=phase,com=0xa0,
                 count=top-14 if mode<14 else 0,
                 a=top if mode==15 else 2,b=3,
                 new_a=3 if mode==15 else 0,new_b=5 if mode==15 else top))
    cases.extend([dict(mode=15,top=7,phase=p,com=0x60,count=0,a=7,b=2,new_a=11,new_b=0) for p in (1,3)])
    for i,c in enumerate(cases):
        c["id"]=i+300
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([f" .rept {n}"," nop"," .endr"])
        write("out",4,6);write("out",5,2)
        if c["mode"]==14:
            write("sts",0x81,24);pair(0x86,c["top"])
        pair(0x88,c["a"]);pair(0x8a,c["b"]);pair(0x84,c["count"])
        write("sts",0x81,(c["mode"]&12)<<1);write("sts",0x80,(c["mode"]&3)|c["com"])
        lines.append(" ldi r24,1");write("sts",0x81,((c["mode"]&12)<<1)|1)
        wait(c["phase"]);pair(0x88,c["new_a"]);pair(0x8a,c["new_b"])
        lines.append(" ldi r24,2");wait(24)
        if c["mode"]==15:
            pair(0x88,7);pair(0x8a,2);wait(16)
        lines.extend([" ldi r24,3"," in r23,5"," in r20,3"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
