import sys
import os
import getopt
import types

import xml.etree.ElementTree
from xml.etree.ElementTree import ElementTree

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
                "ev":       ("ev_t *",    "0")
               }

class MyError(StandardError):
    def __init__(s, msg = ""):
        StandardError.__init__(s)
        s.msg = msg
    def __str__(s):
        return str(s.msg)
    def __repr__(s):
        return repr(s.msg)

class Template:

    def __init__(s):
        s.decl = ""
        s.defi = ""
        s.lits = []
        s.defElem = dict()
        s.curElement = None

        s.newElement("complex")
        s.addAttribute("real", "real", "0.0")
        s.addAttribute("real", "imag", "0.0")
        s.closeElement()
        s.newElement("text")
        s.addAttribute("int", "len", "0")
        s.addAttribute("const char *", "str", "0")
        s.closeElement()

    def newElement(s, ename):
        s.curElement = xml.etree.ElementTree.Element(ename)
        s.decl += "\n\ntypedef struct {"
        if ename not in ["ev", "complex", "text"]:
            s.addAttribute("bool", "valid", "0")

    def closeElement(s):
        if s.curElement == None:
            raise MyError("Template.closeElement() called without "+\
                          "preceeding Template.newElement() !")
        s.decl += "\n} %s_t;" % (s.curElement.tag)
        # add this element to the default element definition
        s.defElem[s.curElement.tag] = s.curElement
        s.curElement = None

    def addElement(s, name):
        if s.curElement == None:
            raise MyError("Template.addAttribute() called without "+\
                          "preceeding Template.newElement() !")
        # set declaration
        isList = False
        ptr = ""
        if name[-1] == "*": # list of structures
            isList = True
            ptr = "*"
            name = name[:-1].strip()
        # add the new subelement to the current one and add it to the dict
        if isList: # add attribute only
            s.addAttribute(name, name, "")
        else:
            s.decl += "\n    %-16s%-2s%s;" % (name+"_t", ptr, name)
            if s.defElem.has_key(name):
                s.curElement.append(s.defElem[name])
            else:
                s.defElem[name] = xml.etree.ElementTree.SubElement(s.curElement)

    # input is string
    def addAttribute(s, type, name, value):
        if s.curElement == None:
            raise MyError("Template.addAttribute() called without "+\
                          "preceeding Template.newElement() !")
        declType = type
        if mapDataTypes.has_key(type):
            declType, defValue = mapDataTypes[type]
        # set declaration
        ptr = ""
        if declType[-1] == "*": # list of structures
            ptr = "*"
            declType = declType[:-1].strip()
        s.decl += "\n    %-16s%-2s%s;" % (declType, ptr, name)
        # set default definition data, handle complex type
        if isinstance(value, types.ListType) and len(value) == 2 and \
           isinstance(defValue, types.ListType) and len(defValue) == 2:
            if value[0] == "#REQUIRED": value[0] = defValue[0]
            if value[1] == "#REQUIRED": value[1] = defValue[1]
        # handle various default value cases
        if not len(value) or value == "#REQUIRED":
            value = defValue
        elif type == "text":
            value = s.addStringLiteral(value)
        # add this field to default data definition
        # save tuple (position, value)
        s.curElement.set(name, (len(s.curElement.keys()), value))

    def addStringLiteral(s, lit):
        i = -1
        try:
            i = s.lits.index(lit)
        except ValueError:
            s.lits.append(lit)
            i = len(s.lits)-1
        return "&string_literals[%d]" % i

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
        res = "\n\n"
        name = "chemical_element"
        type = name+"_t"
        res += type+" "+name+" = {"
        res += s.__test(s.defElem["chemical_element"], 0)
#        s.__resolvePlaceholders()
#        res += s.__stringifyValueList(s.__getValueList(name), 0)
        res += "\n};"
        return res

    def __stringify(s, l):
        res = ""
        if isinstance(l, types.ListType):
            for e in l:
                if len(res): res += ", "
                res += s.__stringify(e)
            res = "{ "+res+" }"
        else:
            res = str(l)
        return res

    def __test(s, elem, lvl):
        lvl += 1
        res = ""
        prefix = ""
        if lvl == 1: prefix = "\n    "
        # sort attribute list by saved position (insert time)
        # (first element of value field)
        attr = elem.items()
        if len(attr) > 1: attr.sort(None, lambda item: item[1][0])
        # stringify attributes
        for item in attr:
            if len(res): res += ", "
            res += prefix + s.__stringify(item[1][1])
        # stringify subelements
        for child in elem.getchildren():
            if len(res): res += ", "
            res += prefix + "{ "+s.__test(child, lvl)+" }"
        lvl -= 1
        return res

    # resolve references to elements which were read in later
    def __resolvePlaceholders(s):
        for elem in s.defElem.values():
            # we have xml.etree.ElementTree.Element inside
            print elem.tag
            for a in elem.items():
                print a[0], a[1]
            if name[-2:] == "[]":
                value = ["/*"+name[:-2]+"*/"]
            else:
                value = "/*"+name+"*/"

    def __getValueList1(s, key):
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

    def __stringifyValueList1(s, list, lvl):
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
        # split the whole line
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
            tem.newElement(s.elementName)
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
                    tem.addAttribute(lastType, name, value)
            lastName = name
        # add custom data fields
        for name in s.elementMembers:
            tem.addElement(name)
        # close this block
        tem.closeElement()
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
    print tem.declaration()
    print tem.definition()
#    print tem.dVal

def parseElement(elem):
    if elem.tag != "chemical_element":
        return

    for attrib in elem.attrib.items():
        # see if this attribute was declared
        print "a:",attrib[0]
        if not tem.dVal.has_key(attrib[0]): continue
        print "bla:",attrib, "def:", tem.dVal[attrib[0]]

    for prop in elem.getchildren():
        # if has_key !!
        print prop.tag, tem.dVal[prop.tag]

def parseXml(filename):
    filename = os.path.abspath(filename);
    if not os.path.exists(filename):
        raise IOError("File not found!")

#    print "parseXml",filename
    etree = ElementTree(file=filename)
    eroot = etree.getroot()
#    print eroot.tag
#    for esub in eroot.getchildren():
#        parseElement(esub)



###################################################

tem = Template()

