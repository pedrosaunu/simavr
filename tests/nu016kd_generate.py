"""Directed lifecycle cases above TOP and recovery by changing unbuffered ICR."""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,7),(15,7)]:
        for label in ("initial_above","resume_above"):
            cases.append(dict(id=700+len(cases),mode=mode,top=top,label=label))
    for label in ("recover_before_max","recover_equal","pending_at_max","stop_then_raise","stop_then_lower"):
        cases.append(dict(id=700+len(cases),mode=14,top=7,label=label))
    for c in cases:
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([f" .rept {n}"," nop"," .endr"])
        def sample():lines.extend([" lds r21,0x84"," lds r22,0x85"])
        mode,top,label=c["mode"],c["top"],c["label"]
        stop=(mode&12)<<1
        write("out",4,6);write("out",5,2)
        if mode==14:write("sts",0x81,24);pair(0x86,top)
        pair(0x88,top if mode==15 else 2);pair(0x8a,3)
        count=65520 if label in ("initial_above","resume_above","pending_at_max") else 12
        pair(0x84,count)
        write("sts",0x81,stop);write("sts",0x80,(mode&3)|0xa0)
        lines.append(" ldi r24,1");write("sts",0x81,stop|1)
        if label=="resume_above":
            wait(2);write("sts",0x81,stop);lines.append(" ldi r24,2");sample()
            pair(0x8a,5);write("sts",0x81,stop|1)
        elif label=="recover_before_max":pair(0x86,31)
        elif label=="recover_equal":pair(0x86,20)
        elif label=="pending_at_max":pair(0x8a,5)
        elif label in ("stop_then_raise","stop_then_lower"):
            write("sts",0x81,stop);pair(0x86,31 if label=="stop_then_raise" else 3);write("sts",0x81,stop|1)
        lines.append(" ldi r24,3");sample();wait(22)
        lines.extend([" ldi r24,4"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
