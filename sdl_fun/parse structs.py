'''
Kieran McDonald
8/2/2020

This script parses the header and cpp files for structs using the //@@Start and //@@End tags around it
If a struct is marked, then it parses all the variables except pointers
and creates functions to read/write these in the serialize.h file
It's up to me (the user) to use these helpers to build the final serialize
'''

files = ["game.h", "board.h"]
delim = " "
nl = "\n"
name = None
tab = "   "

#typeLookup = {"int", "float", "bool", "uint64_t"}

fileSave = open("serialize.h", "w")
#fileLoad = open("load.h", "w")

for fileName in files:
    cpp = open(fileName, "r")
    baseName = fileName.split(".")[0]
        
    saveString = ""
    loadString = ""

    found = False
    for ln in cpp:
        if "//@@Start" in ln:
            split = ln.strip().split(delim)
            structName = split[1]
            saveString += "void _%sSerialize(%s* %s, FILE* file) {\n" %(baseName, structName, baseName)
            
            loadString += "int _%sDeserialize(%s* %s, FILE* file) {\n" %(baseName, structName, baseName)
            
            found = True
            continue
        
        elif "//@@End" in ln:
            
            found = False
            continue
        
        elif found == True:
            if "struct" in ln:
                continue
            split = ln.strip().split(delim)
            if "//" in split[0]:
                continue
            elif "*" in split[0]:
                saveString += tab + "//%s" %ln
                loadString += tab + "//%s" %ln
                continue
            elif len(split) >= 4:
                saveString += tab + 'fwrite(%s->&%s, sizeof(%s), 1, file);\n' %(baseName, split[1], split[0])
                loadString += tab + 'fread(&%s->%s, sizeof(%s), 1, file);\n' %(baseName, split[1], split[0])
                

    saveString += "}\n\n"
    loadString += "}\n\n"
    
    fileSave.write(saveString)
    fileSave.write(loadString)
    cpp.close()

fileSave.close()

