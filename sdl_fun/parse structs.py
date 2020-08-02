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

genCPP = open("serialize.cpp", "w")
genH = open("serialize.h", "w")

headerString = ""
saveString = '#include "serialize.h"\n\n'
genCPP.write(saveString)

for fileName in files:
    baseName = fileName.split(".")[0]
    headerString += '#include "%s.h"\n' %(baseName)

headerString += "\n"
genH.write(headerString)

for fileName in files:
    codeFile = open(fileName, "r")
    baseName = fileName.split(".")[0]

    headerString = ""
    saveString = ""
    loadString = ""

    found = False
    for ln in codeFile:
        if "//@@Start" in ln:       
            found = True
            continue
        
        elif "//@@End" in ln:
            
            found = False
            continue
        
        elif found == True:
            split = ln.strip().split(delim)             
            if "struct" in ln:
                structName = split[1]
                
                saveString += "void _%sSerialize(%s* %s, FILE* file) {\n" %(baseName, structName, baseName)
                headerString += "void _%sSerialize(%s* %s, FILE* file);\n" %(baseName, structName, baseName) 
            
                loadString += "void _%sDeserialize(%s* %s, FILE* file) {\n" %(baseName, structName, baseName)
                headerString += "void _%sDeserialize(%s* %s, FILE* file);\n" %(baseName, structName, baseName)
                
                continue
            if "//" in split[0]:
                continue
            elif "*" in split[0]:
                saveString += tab + "//%s" %ln
                loadString += tab + "//%s" %ln
                continue
            elif len(split) >= 2:
                var = split[1]
                vType = split[0]
                if ";" in var:
                    var = var[:-1]
                saveString += tab + 'fwrite(&%s->%s, sizeof(%s), 1, file);\n' %(baseName, var, vType)
                loadString += tab + 'fread(&%s->%s, sizeof(%s), 1, file);\n' %(baseName, var, vType)
                

    saveString += "}\n\n"
    loadString += "}\n\n"
    
    genCPP.write(saveString)
    genCPP.write(loadString)
    genH.write(headerString)
    codeFile.close()

genCPP.close()
genH.close()

