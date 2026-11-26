import json,sys
from pathlib import Path
actual=json.loads(Path("/tmp/summary").read_text().split("NU016L REF ",1)[1])
gold=json.loads(Path("tests/nu016m_prescaled_golden.json").read_text())[str(actual["id"])]
assert all(actual[k]==v for k,v in gold.items()),(actual,gold)
print("NU016M PRESCALED_GOLDEN "+json.dumps(actual,separators=(",",":")),flush=True)
