import sys
import os
import getopt
import types

startTag = "<"
closeTag = ">"
startElemTag = "("
closeElemTag = ")"
prefixTag = "!"

# maps given data types to Cish data types and default values
mapDataTypes = {"integer":  ("int",       "0"),
                "text":     ("text_t *",  "0"),
                "complex":  ("complex_t", ["0.0", "0.0"]),
                "real":     ("double",    "0.0"),
                "bool":     ("bool",      "0"),
                "ev_t *":   ("ev_t *",    "0")
               }

class MyError(StandardError):
    def __init__(s, msg = ""):
        StandardError.__init__(s)
        s.msg = msg
    def __str__(s):
        return str(s.msg)
    def __repr__(s):
        return repr(s.msg)

class Output:

    def __init__(s):
        s.decl = ""
        s.defi = ""
        s.dVal = dict()
        s.lits = []
        s.curElemName = ""

        s.startStruct("complex")
        s.appendStruct("real", "real", "0.0")
        s.appendStruct("real", "imag", "0.0")
        s.endStruct()
        s.startStruct("text")
        s.appendStruct("int", "len", "0")
        s.appendStruct("const char *", "str", "0")
        s.endStruct()

    def startStruct(s, ename):
        s.curElemName = ename
        s.decl += "\n\ntypedef struct {"
        if ename not in ["ev", "complex", "text"]:
            s.appendStruct("bool", "valid", "0")

    def endStruct(s):
        if not len(s.curElemName):
            raise MyError("Output.endStruct() called without "+\
                          "preceeding Output.startStruct() !")
        s.decl += "\n} %s_t;" % (s.curElemName)
        s.curElemName = ""

    # input is string
    def appendStruct(s, type, name, value):
        if not len(s.curElemName):
            raise MyError("Output.appendStruct() called without "+\
                          "preceeding Output.startStruct() !")
        declType = type
        if mapDataTypes.has_key(type):
            declType, defValue = mapDataTypes[type]
        # set declaration
        ptr = ""
        if declType[-1] == "*":
            declType = declType[:-1].strip()
            ptr = "*"
        s.decl += "\n    %-16s%-2s%s;" % (declType, ptr, name)
        # set default definition data, handle complex type
        if isinstance(value, types.ListType) and len(value) == 2 and \
           isinstance(defValue, types.ListType) and len(defValue) == 2:
            if value[0] == "#REQUIRED": value[0] = defValue[0]
            if value[1] == "#REQUIRED": value[1] = defValue[1]
        # handle various default cases
        if not len(value): # custom type
            if name[-2:] == "[]":
                value = ["/*"+name[:-2]+"*/"]
            else:
                value = "/*"+name+"*/"
        elif value == "#REQUIRED":
            value = defValue
        elif type == "text":
            idx = s.addStringLiteral(value)
            value = "&string_literals["+str(idx)+"]"
        # add this field to default data definition
        if s.dVal.has_key(s.curElemName):
            s.dVal[s.curElemName].append(value)
        else:
            s.dVal[s.curElemName] = [value]

    def addStringLiteral(s, lit):
        try:
            i = s.lits.index(lit)
        except ValueError:
            s.lits.append(lit)
            return len(s.lits)-1
        else:
            return i

    def declaration(s):
        prefix = \
                 "\n#include <stdio.h>\n"+\
                 "\n#define STR_LIT(name) { sizeof(name)-1, name }\n"+\
                 "\ntypedef int bool;\n"
#        postfix = "\n\nconst char * string_literals["+str(len(s.lits))+"];"
        postfix = ""
        return prefix+s.decl+postfix

    def definition(s):
        prefix = ""
        for lit in s.lits:
            if len(prefix):
                prefix += ", "
            prefix += "\n    STR_LIT(\""+lit+"\")"
        prefix = "\ntext_t string_literals["+ \
                 str(len(s.lits))+"] = {"+prefix+"\n};"+\
                 "\n\n// #undef STR_LIT"
        postfix = \
            "\n\nint main(int argc, const char * argv[])\n"+\
            "{\n"+\
            "    printf(\"chemical_element_t size: %ld\\n\",sizeof(chemical_element_t));\n"+\
            "    return 0;\n"+\
            "}\n"
        return prefix+s.defi+s.defaultDefinition()+postfix

    def defaultDefinition(s):
#        print s.dVal
        res = "\n\n"
        name = "chemical_element"
        type = name+"_t"
        res += type+" "+name+" = {\n"
        res += s.__stringifyValueList(s.__getValueList(name), 0)
        res += "\n};"
        return res

    def __getValueList(s, key):
        if isinstance(key, types.StringType):
            if key[0] == "/" and key[-1] == "/":
                key = key.strip("/*")

        result = []
        if isinstance(key, types.ListType):
            for item in key:
                result.append(s.__getValueList(item))
        elif s.dVal.has_key(key):
            for item in s.dVal[key]:
                result.append(s.__getValueList(item))
        else:
            result = key
        return result

    def __stringifyValueList(s, list, lvl):
        lvl+=1
        res = ""
        for item in list:
            if len(res):
                res += ", "
                if lvl == 1: res += "\n"
            if lvl == 1: res += "    "
            if isinstance(item, types.ListType):
                res += "{ "+s.__stringifyValueList(item, lvl)+" }"
            else:
                res += str(item)
        lvl-=1
        return res

class State:

    def __init__(s):
        s.parserFlag = True
        s.elementFlag = False
        s.elementName = ""
        s.elementMembers = []
        s.attributeFlag = False
        s.attributeMembers = []
        s.line = ""
        s.nLine = 0
        s.pos = 0

    def processLine(s, line):
        s.line = line
        s.nLine += 1
        # process a single line
        s.pos = 0
        while s.pos < len(line):

            if line[s.pos] == startTag:
                s.processStartTag()

            elif line[s.pos] == closeTag:
                s.processCloseTag()

            elif line[s.pos] == startElemTag:
                s.elementFlag = True

            elif line[s.pos] == closeElemTag:
                s.nextPos()
                # leave element-list scope
                if s.elementFlag:
                    s.elementFlag = False
            else:
                s.processMembers()
            s.nextPos()

    def processStartTag(s):
        s.nextPos()
        elementName = ""
        parserFlag = True
        # ensure the start tag is followed by the prefix
        if s.line[s.pos] != prefixTag:
            raise MyError("No '"+prefixTag+"' after '"+startTag
                          +"'! (line "+str(s.nLine)+")")
        s.nextPos()
        wordList = s.line[s.pos:].rstrip(closeTag).split()
        if len(wordList) < 2:
            raise MyError("Missing element name at line "
                          +str(s.nLine)+", pos "+str(s.pos)+" !")
        s.pos += len(wordList[0]) + len(wordList[1]) + 1
        s.elementName = wordList[1]
        if wordList[0] == "ELEMENT":
            s.elementMembers = [] # reset
        elif wordList[0] == "ATTLIST":
            s.attributeFlag = True # entering attribute scope
            out.startStruct(s.elementName)
        else:
            raise MyError("Tag type '"+wordList[0]
                          +"' is neither 'ELEMENT' nor 'ATTLIST'!"
                          +" (line "+str(s.nLine)+")")

    def processCloseTag(s):
        if not s.attributeFlag: return
        s.attributeFlag = False
        # finished reading in attributes, now processing them
        lastType = "double"
        lastName = None
        lastValue = None
        for [name, dummy, value] in s.attributeMembers:
            if name[0:4] == "type":
                lastType = value
            else:
                # handle complex fields eventually
                if name == "re" and lastName != "im" or \
                   name == "im" and lastName != "re":
                    lastValue = value
                else:
                    if name == "re" and lastName == "im": 
                        value = [value, lastValue]
                    elif name == "im" and lastName == "re": 
                        value = [lastValue, value]
                    # rename value field
                    if name == "val" or name == "im" or name == "re":
                        name = "value"
                    # add field finally
                    out.appendStruct(lastType, name, value)
            lastName = name
        # add custom data fields
        for name in s.elementMembers:
            type = name+"_t"
            value = ""
            if name[-1] == "*":
                type = name[:-1]+"_t *"
                name = name[:-1]
                value = "0"
            out.appendStruct(type, name, value)
        # close this block
        out.endStruct()
        # cleanup
        s.attributeMembers = []
        s.elementMembers = []

    def processMembers(s):
        if not s.elementFlag and not s.attributeFlag:
            return
        preparedLine = s.line[s.pos:].rstrip(closeTag+closeElemTag)
        s.pos += len(preparedLine)-1
        wordList = [word.strip("\",?") for word in preparedLine.split()]
        if s.elementFlag:
            s.elementMembers.extend(wordList)
        elif s.attributeFlag:
            if len(wordList) == 3:
                s.attributeMembers.append(wordList)
            else:
                raise MyError("#Attributes not 3! (line "+str(s.nLine)+")")

    def nextPos(s):
        s.pos += 1

def parseDtd(filename):
    filename = os.path.abspath(filename);
    if not os.path.exists(filename):
        raise IOError("File not found!")

    fd = open(filename, 'r')
    state = State()
    for line in fd:
        line = line.strip(" \r\t\n")
        # ignore empty lines and comments
        if not len(line) or line[0:4] == "<!--":
            continue

        state.processLine(line)

    fd.close()
    print out.declaration()
    print out.definition()
#    print out.dVal


out = Output()

