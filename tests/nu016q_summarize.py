import hashlib,json,re,sys
from pathlib import Path
cid,rom,trace=sys.argv[1:]
raw=Path(trace).read_text()
rows=[list(map(int,x.split(","))) for x in re.findall(r"^NU016Q ROW ([0-9,]+)$",raw,re.M)]
assert rows and "NU016Q PASS" in raw and rows[-1][4]==90
assert all(len(x)==12 and x[0]==i+1 for i,x in enumerate(rows))
print("NU016Q_REF "+json.dumps(dict(id=int(cid),rom_sha256=hashlib.sha256(Path(rom).read_bytes()).hexdigest(),rows=len(rows),digest=hashlib.sha256(json.dumps(rows,separators=(",",":")).encode()).hexdigest(),trace=rows)),flush=True)
