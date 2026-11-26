import hashlib,json,sys
from pathlib import Path
cid,rom,trace=sys.argv[1:]
rows=Path(trace).read_text().splitlines()
events=[list(map(int,x.split()[1:])) for x in rows if x.startswith("IRQ ")]
done=next(list(map(int,x.split()[1:])) for x in rows if x.startswith("DONE "))
print("NU016RU_REF "+json.dumps(dict(id=int(cid),rom_sha256=hashlib.sha256(Path(rom).read_bytes()).hexdigest(),irq_sequence=events,counts=done[:4],samples=done[4])))
