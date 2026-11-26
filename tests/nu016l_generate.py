"""Real guest ROMs, all five Fast PWM modes and four shared divider taps.
Long delays use counted AVR loops, not host-controlled timer advancement.
"""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for mode,top in [(5,255),(6,511),(7,1023),(14,3),(15,3)]:
        for cs,div in [(2,8),(3,64),(4,256),(5,1024)]:
            c=dict(id=800+len(cases),mode=mode,top=top,cs=cs,div=div)
            cases.append(c)
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
            write("out",4,6);write("out",5,2)
            if mode==14:write("sts",0x81,24);pair(0x86,top)
            pair(0x88,top if mode==15 else 1);pair(0x8a,0);pair(0x84,top-1)
            stop=(mode&12)<<1
            write("sts",0x81,stop);write("sts",0x80,(mode&3)|0xa0)
            lines.append(" ldi r24,1");write("sts",0x81,stop|cs)
            delay(3*div+3)
            lines.extend([" ldi r24,2"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
            (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
