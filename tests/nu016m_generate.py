"""Lifecycle stimuli: same-family mode transitions, divider changes and GTCCR.
Each change uses one direct guest store; unsupported intermediary modes are excluded.
"""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for before,after in [(5,6),(6,5),(6,7),(7,6),(14,15),(15,14),(6,14),(14,6),(7,15),(15,7)]:
        for cs,div in [(1,1),(2,8)]:
            cases.append(dict(id=900+len(cases),kind="mode",mode=before,target=after,cs=cs,div=div))
    for cs,div in [(2,8),(3,64),(4,256),(5,1024)]:
        for kind in ("clock","stop_resume","hold_release","reset"):
            cases.append(dict(id=900+len(cases),kind=kind,mode=14,target=1 if kind=="clock" else cs,cs=cs,div=div))
    for c in cases:
        lines=[".text",".global start","start:"," cli"];loop=0
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def delay(n):
            nonlocal loop
            while n>=3:
                k=min(255,n//3);loop+=1
                lines.extend([f" ldi r18,{k}",f"loop{loop}:"," dec r18",f" brne loop{loop}"])
                n-=3*k
            lines.extend([" nop"]*n)
        def sample():lines.extend([" lds r21,0x84"," lds r22,0x85"])
        mode,cs,div,kind,target=(c[k] for k in ("mode","cs","div","kind","target"))
        top={5:255,6:511,7:1023,14:15,15:15}[mode]
        stop=(mode&12)<<1
        write("out",4,6);write("out",5,2);write("sts",0x81,24)
        pair(0x86,15);pair(0x88,15 if mode==15 else 7);pair(0x8a,3);pair(0x84,top-4)
        write("sts",0x81,stop);write("sts",0x80,(mode&3)|0xa0)
        lines.append(" ldi r24,1");write("sts",0x81,stop|cs)
        delay(div+2);sample()
        if kind=="mode":
            if (mode&12)==(target&12):write("sts",0x80,(target&3)|0xa0)
            else:write("sts",0x81,((target&12)<<1)|cs)
        elif kind=="clock":
            write("sts",0x81,stop|target)
        elif kind=="stop_resume":
            write("sts",0x81,stop);pair(0x8a,5);sample();delay(div+1);write("sts",0x81,stop|cs)
        elif kind=="hold_release":
            write("out",0x23,0x81);pair(0x8a,5);sample();delay(div+1);write("out",0x23,0)
        else:
            write("out",0x23,1)
        lines.append(" ldi r24,2");sample();delay(2*div+5)
        lines.extend([" ldi r24,3"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
