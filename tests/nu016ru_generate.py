"""Guest vector delivery discriminator, stable dual-slope timer.
ISR records source ordinal in r20 and increments a dedicated counter.
The CPU main loop terminates after a fixed number of guest iterations.
No assumptions about exact interrupt entry timing in simavr.
"""
from pathlib import Path
import json
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True);cases=[]
    specs=[(m,2) for m in (1,2,3,8,9,10,11)]+[(8,32),(9,1),(10,4),(11,7)]
    for mode,mask in specs:
        cid=2000+len(cases);top={1:255,2:511,3:1023}.get(mode,15)
        a=top if mode in (9,11) else top-3;b=top-2
        initial=top-6 if mode in (1,2,3) else 0
        lines=[".text",".global start","start:"," rjmp main"]
        for i,(address,handler) in enumerate(((0x28,"cap"),(0x2c,"a"),(0x30,"b"),(0x34,"ov"))):
            lines.extend([f".org {address}",f" rjmp isr_{handler}"])
        lines.extend([".org 0x38","main:"," cli"])
        def write(addr,v):lines.extend([f" ldi r16,{v}",f" sts {addr},r16"])
        def pair(addr,v):write(addr+1,v>>8);write(addr,v&255)
        write(0x43,0x81);write(0x81,(mode&12)<<1);write(0x80,mode&3)
        pair(0x86,top);pair(0x88,a);pair(0x8a,b);pair(0x84,initial)
        write(0x36,39);write(0x6f,mask);write(0x81,((mode&12)<<1)|2);write(0x43,0)
        lines.extend([" sei"," nop"," ldi r18,64","sample:"," nop"," nop"," inc r24"," dec r18"," brne sample"," cli"," ldi r25,90"," nop"])
        for i,(name,reg) in enumerate((("cap",17),("a",19),("b",26),("ov",27)),1):
            lines.extend([f"isr_{name}:",f" inc r{reg}",f" ldi r20,{i}"," reti"])
        (root/f"case-{cid}.S").write_text("\n".join(lines)+"\n")
        cases.append(dict(id=cid,mode=mode,mask=mask,top=top,ocr_a=a,ocr_b=b,initial=initial))
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n");return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
