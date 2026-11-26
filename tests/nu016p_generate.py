"""Standalone hardware discriminator prefixes and UART suffix.
Capture flags and pin samples before and after the disputed boundary.
The candidate is an observation target, never a silicon oracle.
"""
from pathlib import Path
import sys
def generate(root):
    root=Path(root);root.mkdir(parents=True,exist_ok=True)
    for case in range(3):
        lines=[".text",".global start","start:"," cli"]
        def write(op,addr,v):lines.extend([f" ldi r16,{v}",f" {op} {addr},r16"])
        def pair(addr,v):write("sts",addr+1,v>>8);write("sts",addr,v&255)
        def wait(n):lines.extend([" nop"]*n)
        def sample(a,b):lines.extend([f" in r{a},0x16",f" in r{b},3"])
        write("out",4,6);write("out",5,2);write("sts",0x81,24)
        pair(0x86,7 if case==0 else 15)
        pair(0x88,2);pair(0x8a,5 if case==0 else 7)
        pair(0x84,65516 if case==0 else 0);write("sts",0x80,0xa2)
        write("sts",0x81,25)
        if case==0:
            wait(12);sample(19,20);wait(6);sample(21,22);wait(10);sample(23,24)
        else:
            wait(case-1);pair(0x84,2);sample(19,20);wait(2);sample(21,22);wait(16);sample(23,24)
        # Marker before observation UART; stop and disconnect afterwards.
        lines.extend([" ldi r25,90"," nop"])
        prefix="\n".join(lines)+"\n"
        write("sts",0x81,24);write("sts",0x80,2)
        # USART0 9600 baud / 16MHz, 8N1; samples in r19-r24 untouched.
        for addr,v in [(0xc5,0),(0xc0,0),(0xc4,103),(0xc2,6),(0xc1,8)]:write("sts",addr,v)
        lines.extend([" .macro tx source","1:"," lds r16,0xc0"," sbrs r16,5"," rjmp 1b"," sts 0xc6,\\source"," .endm",
            " ldi r17,0xa5"," tx r17"," ldi r17,0x5a"," tx r17",f" ldi r17,{case}"," tx r17"])
        lines.extend([f" tx r{reg}" for reg in range(19,25)])
        lines.extend(["2:"," rjmp 2b"])
        (root/f"prefix-{case}.S").write_text(prefix)
        (root/f"hardware-{case}.S").write_text("\n".join(lines)+"\n")
if __name__=="__main__":generate(sys.argv[1])
