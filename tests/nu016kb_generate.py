"""Running ICR1 TOP changes; deterministic guest stimuli."""
import json
from pathlib import Path
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    cases=[]
    for label,top,count,new_top in [("increase",15,0,31),("decrease",31,0,15),("equal_at_commit",31,0,8),("miss_near_max",65535,65516,7),("miss_compare_above_top",65535,65516,3),("zero_after_max",65535,65516,0)]:
        for phase in (0,2):
            cases.append(dict(id=500+len(cases),label=label,top=top,count=count,new_top=new_top,phase=phase))
    for c in cases:
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([f" .rept {n}"," nop"," .endr"])
        write("out",4,6);write("out",5,2);write("sts",0x81,24)
        pair(0x86,c["top"]);pair(0x88,2);pair(0x8a,65532 if c["label"]=="miss_compare_above_top" else 5)
        pair(0x84,c["count"]);write("sts",0x80,0xa2)
        lines.append(" ldi r24,1");write("sts",0x81,25);wait(c["phase"])
        pair(0x86,c["new_top"])
        lines.extend([" ldi r24,2"," lds r21,0x84"," lds r22,0x85"])
        wait(24)
        lines.extend([" ldi r24,3"," in r23,5"," in r20,3"," lds r21,0x84"," lds r22,0x85"," ldi r25,90"," nop"])
        (root/f"case-{c['id']}.S").write_text("\n".join(lines)+"\n")
    (root/"cases.json").write_text(json.dumps(cases,indent=2)+"\n")
    return cases
if __name__=="__main__":
    import sys
    generate(sys.argv[1])
