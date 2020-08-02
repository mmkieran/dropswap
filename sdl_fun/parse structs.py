
files = ["game.h", ]
delim = " "
nl = "\n"
name = None
saveString = ""
loadString = ""
tab = "   "

#typeLookup = {"int", "float", "bool", "uint64_t"}

fileSave = open("serialize.h", "w")
#fileLoad = open("load.h", "w")

for fileName in files:
    cpp = open(fileName, "r")
    baseName = fileName.split(".")[0]
    found = False
    for ln in cpp:
        if "struct" in ln and "{" in ln:
            split = ln.strip().split(delim)
            structName = split[1]
            saveString += "FILE* %sSerialize(%s* %s) {\n" %(baseName, structName, baseName)
            saveString += tab + "FILE* out;\n"
            saveString += tab + 'int err = fopen_s(&out, "assets/game_state.dat", "w");\n'
            saveString += tab + 'if (err == 0) {\n'
            
            loadString += "int %sDeserialize(%s* %s, const char* path) {\n" %(baseName, structName, baseName) 
            found = True
            continue
        
        if found == True:
            split = ln.strip().split(delim)
            if "*" in split[0]:
                saveString += tab*2 + "//%s" %ln
                continue
            if len(split) >= 4:
                saveString += tab*2 + 'fwrite(%s->&%s, sizeof(%s), 1, out);\n' %(baseName, split[1], split[0])
                loadString += 'if (%s) %s->%s = atoi(%s);' %(split[3], baseName, split[1], split[3])
            
        if "};" in ln:
            
            found = False
            continue

    saveString += 'else { printf("Failed to save file... Err: %d\\n", err); }\n'
    saveString + "fclose(out);\n"
    saveString + "return out;\n"
    saveString += "}\n\n"
    
    fileSave.write(saveString)
    fileSave.write(loadString)
    #fileLoad.write(loadString)
    cpp.close()
    fileSave.close()
    #fileLoad.close()

