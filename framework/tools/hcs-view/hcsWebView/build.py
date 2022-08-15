import os

# development production
os.system("npx webpack --mode=development")

ss = open(r".\..\hcsVSCode\editor.html", "r", encoding="utf8").read()
i1 = ss.index("// update js code begin") + len("// update js code begin") + 1
i2 = ss.index("// update js code end") - 1
destss = open(r".\dist\main.js", "r", encoding="utf8").read()
ss = ss[:i1] + destss + ss[i2:]
open(r".\..\hcsVSCode\editor.html", "w", encoding="utf8").write(ss)

print("replaced")
