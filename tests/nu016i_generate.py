"""Generate fixed, deterministic guest stimuli; no emulator or expected trace."""
import json
from pathlib import Path

def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,7),(15,7)]:
        for label,oa,ob,com in [("extreme",top if mode==15 else 0,0 if mode==15 else top,0xa0),
            ("inverse",top,top if mode==15 else 0,0xf0),
            ("interior",top if mode==15 else 2,3,0xa0)]:
            cases.append(dict(mode=mode,top=top,oa=oa,ob=ob,com=com,label=label,count=top-4))
    cases.extend([dict(mode=15,top=7,oa=7,ob=3,com=0x60,label="toggle",count=3),
                  dict(mode=14,top=7,oa=2,ob=3,com=0x60,label="toggle",count=3)])
    for i,c in enumerate(cases):
        c["id"]=i
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):
            lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):
            write("sts",addr+1,v>>8);write("sts",addr,v&255)
        write("out",4,6);write("out",5,2)
        pair(0x86,c["top"]);pair(0x88,c["oa"]);pair(0x8a,c["ob"]);pair(0x84,c["count"])
        write("sts",0x81,(c["mode"]&12)<<1)
        write("sts",0x80,(c["mode"]&3)|c["com"])
        lines.append(" ldi r24,1")
        write("sts",0x81,((c["mode"]&12)<<1)|1)
        lines.extend([" .rept 22"," nop"," .endr"," ldi r24,2"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{i}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases

if __name__=="__main__":
    import sys
    generate(sys.argv[1])
