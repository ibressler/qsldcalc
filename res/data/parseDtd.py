import sys
import os
import getopt
from types import *

import xml.etree.ElementTree
from xml.etree.ElementTree import ElementTree

startTag = "<"
closeTag = ">"
startElemTag = "("
closeElemTag = ")"
prefixTag = "!"


def clampAttrName(name):
    if name in ["val", "im", "re"]:
        name = "value"
    return name

# input is string but contains numbers
def beautifyConstants(template, value):
    print "template",template, template.find("."), template.lower().find("e")
    print "value",value, value.find("."), value.lower().find("e")
    if (template.find(".") >= 0 or template.lower().find("e") >= 0) and \
       (value.find(".") < 0 or value.lower().find("e") < 0):
        value += ".0"
    elif template.find(".") < 0 and template.lower().find("e") < 0 and \
       (value.find(".") >= 0 or value.lower().find("e") >= 0):
        num = float(value)
        value = string(int(floor(num)))
    return value

def formatDeclaration(declType, name):
        ptr = ""
        if declType[-1] == "*": # list of structures
            ptr = "*"
            declType = declType[:-1].strip()
        return "\n    %-16s%-2s%s;" % (declType, ptr, name)

class MyError(StandardError):
    def __init__(s, msg = ""):
        StandardError.__init__(s)
        s.msg = msg
    def __str__(s):
        return str(s.msg)
    def __repr__(s):
        return repr(s.msg)

# maps given data types to Cish data types and default values
class DataTypes:
    __default = FloatType
    __fallback = NoneType
    __map = {"integer": IntType,
             "text":    StringTypes,
             "complex": ComplexType,
             "real":    FloatType,
             "bool":    BooleanType
            }
    __data = { IntType:     ("int",       "0"),
               StringTypes: ("text_t *",  "0"),
               ComplexType: ("complex_t", ["0.0", "0.0"]),
               FloatType:   ("double",    "0.0"),
               BooleanType: ("bool",      "0")
             }

    def default(): return DataTypes.__default
    default = staticmethod(default)

    def get(key):
        if DataTypes.__map.has_key(key):
            return DataTypes.__map[key]
        else:
            return DataTypes.__fallback
    get = staticmethod(get)

    def decl(type):
        if DataTypes.__data.has_key(type):
            return DataTypes.__data[type][0]
        else:
            return str(type)
    decl = staticmethod(decl)

    def defValue(type):
        if DataTypes.__data.has_key(type):
            return DataTypes.__data[type][1]
        else:
            return "0"
    defValue = staticmethod(defValue)

class Template:

    def __init__(s):
        s.decl = ""
        s.defi = ""
        s.lits = []
        # finally: C-valid default element with data from DTD
        s.defElem = dict()
        s.curElement = None

        s.newElement("complex")
        s.addAttribute(FloatType, "real", "0.0")
        s.addAttribute(FloatType, "imag", "0.0")
        s.closeElement()
        s.newElement("text")
        s.addAttribute(IntType, "len", "0")
        s.addAttribute("const char *", "str", "0")
        s.closeElement()

    def newElement(s, ename):
        s.curElement = xml.etree.ElementTree.Element(ename)
        s.decl += "\n\ntypedef struct {"
        if ename not in ["ev", "complex", "text"]:
            s.addAttribute(BooleanType, "valid", "0")

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
        ptr = ""
        if name[-1] == "*": # list of structures
            ptr = " *"
            name = name[:-1].strip()
        type = name+"_t"+ptr
        if len(ptr): # is list
            s.addAttribute(type, name, "")
        else:
            # add the new subelement to the current one and add it to the dict
            s.decl += formatDeclaration(type, name)
            if s.defElem.has_key(name):
                s.curElement.append(s.defElem[name])
            else:
                s.defElem[name] = xml.etree.ElementTree.SubElement(s.curElement)

    # input is string
    def addAttribute(s, type, name, value):
        if s.curElement == None:
            raise MyError("Template.addAttribute() called without "+\
                          "preceeding Template.newElement() !")
        # set declaration
        declType = DataTypes.decl(type)
        s.decl += formatDeclaration(declType, name)

        # set default definition data
        defValue = DataTypes.defValue(type)
        if not len(value) or value == "#REQUIRED":
            value = defValue
        # handle complex type
        elif type is ComplexType: # value is list [re, im]
            if value[0] == "#REQUIRED": value[0] = defValue[0]
            if value[1] == "#REQUIRED": value[1] = defValue[1]
        # handle various default value cases
        elif type is StringTypes:
            value = s.addStringLiteral(value)
        # add this field to default data definition
        # save list [index, type, value], tuple can not be modified later
        idx = len(s.curElement.keys())
        s.curElement.set(name, [idx, type, value])

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
        return prefix+s.defi+s.elemDefinition()+postfix

    def elemDefinition(s):
        res = "\n\n"
        name = "chemical_element"
        type = name+"_t"
        res += type+" "+name+" = {"
        res += s.__elemDefinition(s.defElem[name], 0)
        res += "\n};"
        return res

    def __stringify(s, l):
        res = ""
        if isinstance(l, ListType):
            for e in l:
                if len(res): res += ", "
                res += s.__stringify(e)
            res = "{ "+res+" }"
        else:
            res = str(l)
        return res

    def __elemDefinition(s, elem, lvl):
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
            # convert the value: [index, type, value] -> string
            res += prefix + s.__stringify(item[1][2])
        # stringify subelements
        for child in elem.getchildren():
            if len(res): res += ", "
            res += prefix + "{ "+s.__elemDefinition(child, lvl)+" }"
        lvl -= 1
        return res

    def update(s, e):
        print "update",e.tag
        if not s.defElem.has_key(e.tag): return
        s.updateAttributes(s.defElem[e.tag], e)

        for esub in e.getchildren():
            s.update(esub)

    def updateAttributes(s, defElem, newElem):
        print "updateAttributes",defElem.tag,newElem.tag
        for attr in newElem.items():
            key = clampAttrName(attr[0])
            # see if this attribute was declared
            defAttr = defElem.get(key)
            print "def Attr", key, str(defAttr), attr[1], str(type(attr[1]))
            if not defAttr: continue
            # attribute exists, update it
            if isinstance(defAttr[1], types.ListType):
                if attr[0] == "re":
                    defAttr[1][0] = beautifyConstants(defAttr[1][0], attr[1])
                elif attr[0] == "im": 
                    defAttr[1][1] = beautifyConstants(defAttr[1][1], attr[1])
            else:
                defAttr[1] = beautifyConstants(defAttr[1], attr[1])
            defElem.set(key, defAttr) ## TODO complex, ev, ...
            print "aft Attr", key, str(defElem.get(key))

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
        lastType = DataTypes.default()
        lastName = None
        lastValue = None
        for [name, dummy, value] in s.attributeMembers:
            if name[0:4] == "type":
                lastType = DataTypes.get(value)
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
                    # rename value field eventually
                    name = clampAttrName(name)
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


def parseXml(filename):
    filename = os.path.abspath(filename);
    if not os.path.exists(filename):
        raise IOError("File not found!")

    etree = ElementTree(file=filename)
    eroot = etree.getroot() # chemical_element_list
    for esub in eroot.getchildren():
        if esub.tag != "chemical_element": continue
        # we care only about single chemical_elements
        tem.update(esub)
        break

#    print tem.definition()



###################################################

tem = Template()

