"""Compare complete raw reference trace within build; no log transport dependency."""
import hashlib,json,re,sys
from pathlib import Path
id,rom,trace=sys.argv[1:]
gold=json.loads(Path("tests/nu016kd_regression_golden.json").read_text())[id]
actual_hash=hashlib.sha256(Path(rom).read_bytes()).hexdigest()
raw=Path(trace).read_text()
rows=[list(map(int,x.split(","))) for x in re.findall(r"^NU016F ROW ([0-9,]+)$",raw,re.M)]
assert actual_hash==gold["hash"],(id,"ROM hash",actual_hash,gold["hash"])
assert "NU016F PASS" in raw and rows and rows[-1][14]==90,(id,"terminal marker")
assert len(rows)==len(gold["rows"]) and all(len(x)==17 and x[0]==i+1 for i,x in enumerate(rows)),(id,"shape")
assert rows==gold["rows"],(id,"raw trace mismatch",[(i+1,a,b) for i,(a,b) in enumerate(zip(rows,gold["rows"])) if a!=b][:3])
digest=hashlib.sha256(json.dumps(rows,separators=(",",":")).encode()).hexdigest()
print(f"NU016KD GOLDEN {id} {len(rows)} {actual_hash} {digest}",flush=True)
