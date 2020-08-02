
files = ["game.h", ]
delim = " "
nl = "\n"
name = None
saveString = ""
loadString = ""

#typeLookup = {"int", "float", "bool", "uint64_t"}

fileOut = open("save.h", "w")
fileOut = open("load.h", "w")

for fileName in files:
    cpp = open(fileName, "r")
    baseName = fileName.split(".")[0]
    found = False
    for ln in cpp:
        if "struct" in ln and "{" in ln:
            split = ln.strip().split(delim)
            structName = split[1]
            saveString += "FILE* %sSave(%s* %s) {\n" %(baseName, structName, baseName)
            loadString += "int %sLoad(%s* %s, const char* path) {\n" %(baseName, structName, baseName) 
            found = True
            continue
        
        if found == True:
            split = ln.strip().split(delim)
            if "*" in split[0]:
                string += "//%s\n" %ln
                continue
            if len(split) >= 4:
                saveString += '''fprintf(out, "%s,%s,%s,%%d\n", %s->%s);''' %(baseName, split[0], split[1], baseName, split[1])
                if (
                loadString += '''if (%s) %s->%s = atoi(%s);''' %(split[3], baseName, split[1], split[3])
            
        if "};" in ln:
            
            found = False
            continue

    cpp.close()
    fileOut.close()

