import hashlib,json,re,sys
from pathlib import Path
cid,rom,trace=sys.argv[1:]
raw=Path(trace).read_text()
rows=[list(map(int,x.split(","))) for x in re.findall(r"^NU016R ROW ([0-9,]+)$",raw,re.M)]
assert len(rows)==32 and "NU016R PASS" in raw
assert all(len(x)==12 and x[3]==i+1 for i,x in enumerate(rows))
print("NU016R_REF "+json.dumps(dict(id=int(cid),rom_sha256=hashlib.sha256(Path(rom).read_bytes()).hexdigest(),samples=len(rows),trace=rows)),flush=True)
