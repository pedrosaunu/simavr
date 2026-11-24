import sys,json,hashlib,collections
g=collections.defaultdict(list)
for line in open(sys.argv[1]):
    if line.count(",")==39:g[line.split(",")[0]].append(line)
expected={"adiw":3072,"sbiw":3072,"bset":2048,"bclr":2048,"bst":8192,"bld":8192}
assert {k:len(v) for k,v in g.items()}==expected
for k,v in g.items():print("NU007B_ORACLE "+json.dumps({"op":k,"cases":len(v),"sha256":hashlib.sha256("".join(v).encode()).hexdigest(),"first":v[0].strip(),"last":v[-1].strip()}))
print("NU007B_SUMMARY "+json.dumps({"cases":sum(expected.values()),"families":len(expected)}))
