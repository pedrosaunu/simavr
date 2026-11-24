import hashlib,sys,json,collections
groups=collections.defaultdict(list)
for line in open(sys.argv[1]):
    if line.count(",")==38:groups[line.split(",")[0]].append(line)
assert len(groups)==28 and all(len(g)==1152 for g in groups.values()),{k:len(v) for k,v in groups.items()}
for k,g in groups.items():
    print("NU007A_ORACLE "+json.dumps({"op":k,"cases":len(g),"sha256":hashlib.sha256("".join(g).encode()).hexdigest(),"first":g[0].strip(),"last":g[-1].strip()}))
print("NU007A_SUMMARY "+json.dumps({"families":len(groups),"cases":sum(map(len,groups.values()))}))
