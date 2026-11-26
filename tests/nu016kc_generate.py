"""Running TCNT1 commits, compare suppression and initial TOP zero."""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,15),(15,15)]:
        for phase,value in [(0,2),(1,top),(2,top+1)]:
            cases.append(dict(id=600+len(cases),mode=mode,top=top,phase=phase,value=value,count=0 if value<=top else 65520 if mode>=14 else top-10))
    for mode in (14,15):
        cases.append(dict(id=600+len(cases),mode=mode,top=0,phase=0,value=0,count=0))
    # Separate near-MAX writes exercise a real natural wrap within the short ROM.
    for mode in (5,14,15):
        cases.append(dict(id=600+len(cases),mode=mode,top=255 if mode==5 else 15,phase=0,value=65533,count=0))
    for c in cases:
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([f" .rept {n}"," nop"," .endr"])
        mode,top=c["mode"],c["top"]
        stop=(mode&12)<<1
        write("out",4,6);write("out",5,2)
        if mode==14:
            write("sts",0x81,24);pair(0x86,top)
        pair(0x88,top if mode==15 else 2);pair(0x8a,7 if top else 0);pair(0x84,0)
        write("sts",0x81,stop);write("sts",0x80,(mode&3)|0xa0)
        lines.append(" ldi r24,1");write("sts",0x81,stop|1);wait(c["phase"])
        pair(0x84,c["value"])
        lines.extend([" ldi r24,2"," lds r21,0x84"," lds r22,0x85"])
        wait(20);lines.extend([" ldi r24,3"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
