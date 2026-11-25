"""Stopped-clock pending OCR stimuli; no reference or expected trace generation."""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,31),(15,31)]:
        for phase in (0,2):
            cases.append(dict(id=400+len(cases),mode=mode,top=top,phase=phase,
                              count=top-20,a=top if mode==15 else 2,b=3))
    for c in cases:
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([f" .rept {n}"," nop"," .endr"])
        def stage(n):lines.append(f" ldi r24,{n}")
        stop=(c["mode"]&12)<<1
        write("out",4,6);write("out",5,2)
        if c["mode"]==14:
            write("sts",0x81,24);pair(0x86,c["top"])
        pair(0x88,c["a"]);pair(0x8a,c["b"]);pair(0x84,c["count"])
        write("sts",0x81,stop);write("sts",0x80,(c["mode"]&3)|0xa0)
        stage(1);write("sts",0x81,stop|1);wait(c["phase"])
        pair(0x8a,0)
        write("sts",0x81,stop);stage(2)
        lines.extend([" lds r21,0x84"," lds r22,0x85"])
        pair(0x88,7 if c["mode"]==15 else 0)
        pair(0x8a,5 if c["mode"]==15 else c["top"])
        wait(4);stage(3);write("sts",0x81,stop|1)
        wait(24);stage(4)
        lines.extend([" in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
