import hashlib,json,re,sys
from pathlib import Path
id,rom,trace=sys.argv[1:]
raw=Path(trace).read_text()
r=[list(map(int,x.split(","))) for x in re.findall(r"^NU016F ROW ([0-9,]+)$",raw,re.M)]
assert r and "NU016F PASS" in raw and r[-1][14]==90 and all(len(x)==17 and x[0]==i+1 for i,x in enumerate(r))
rows=[x[:10]+[x[16]]+x[11:16] for x in r]
print("NU016L REF "+json.dumps(dict(id=int(id),rom_sha256=hashlib.sha256(Path(rom).read_bytes()).hexdigest(),rows=len(rows),digest=hashlib.sha256(json.dumps(rows,separators=(",",":")).encode()).hexdigest(),sample=rows[-1])),flush=True)
