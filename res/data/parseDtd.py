import sys
import os
import getopt
import copy
import math
import inspect
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

def formatDeclaration(declType, name):
        ptr = ""
        if declType[-1] == "*": # list of structures
            ptr = "*"
            declType = declType[:-1].strip()
        return "\n    %-16s%-2s%s;" % (declType, ptr, name)

def stringifyElement(elem, lvl):
    lvl += 1
    res = ""
    prefix = ""
    if lvl == 1: prefix = "\n    "
    # sort attribute list by saved position (insert time)
    # (first element of value field)
    attributes = elem.items()
    if len(attributes) > 1: 
        attributes.sort(None, lambda item: item[1].sortKey())
    # stringify attributes
    for item in attributes:
        if len(res): res += ", "
        # convert the value: [index, type, value] -> string
        res += prefix + item[1].stringify()
    # stringify subelements
    for child in elem.getchildren():
        if len(res): res += ", "
        res += prefix + "{ "+stringifyElement(child, lvl)+" }"
    lvl -= 1
    return res

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
    __defaultType = FloatType
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

    def defaultType(): return DataTypes.__defaultType
    defaultType = staticmethod(defaultType)

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

    def default(type):
        if DataTypes.__data.has_key(type):
            return DataTypes.__data[type][1]
        else:
            return "0" # NULL pointer for lists/arrays of custom types
    default = staticmethod(default)

    def verify(type, default, value): # helper
        if value == None:    return default

        # transform the new value accordingly
        if type is FloatType:
            if value.find(".") < 0 and \
               value.lower().find("e") < 0:
                value += ".0"
        elif type is IntType:
            if value.find(".") >= 0 and \
               value.lower().find("e") >= 0:
                num = float(value)
                value = int(math.floor(num))
        elif type is StringTypes:
            value = tem.addStringLiteral(str(value))
        elif type is BooleanType:
            if isinstance(value, BooleanType):
                value = str(int(value))
        elif type is ComplexType:
            # complex consists of two floats
            value[0] = DataTypes.verify(FloatType, default[0], value[0])
            value[1] = DataTypes.verify(FloatType, default[1], value[1])
        return value
    verify = staticmethod(verify)

class Attribute(object):

    def __init__(s, parentElem, name, type, value):
        s.name = name
        s.__initValue(type, value)
        s.index = len(parentElem.keys())
        parentElem.set(name, s)

    def __initValue(s, type, value):
        s.type = type
        s.__setValue(value)

    # There are several levels of default data values:
    # - the ones specified in the DTD 'value', will be overwritten
    # - the ones within this script which are required
    #   for a C structure definition to be valid 'default'
    def __setValue(s, value):

        # get default definition data
        default = DataTypes.default(s.type)

        # retreive the new value
        if isinstance(value, StringTypes):
            if not len(value) or value == "#REQUIRED":
                value = None # fixed in DataTypes.verify()
        elif isinstance(value, ListType):
            # handle complex type
            if s.type is ComplexType: # value is list [re, im] (has to)
                if value[0] == "#REQUIRED": value[0] = default[0]
                if value[1] == "#REQUIRED": value[1] = default[1]

        s.value = DataTypes.verify(s.type, default, value)

    def setValue(s, value):
        res = copy.deepcopy(s)
        res.__setValue(value)
        return res

    def stringify(s):
        def stringifyList(lst):
            res = ""
            if isinstance(lst, ListType):
                for e in lst:
                    if len(res): res += ", "
                    res += stringifyList(e)
                res = "{ "+res+" }"
            else:
                res = str(lst)
            return res
        return stringifyList(s.value)

    def sortKey(s): return s.index

    # returns a deep copy if this attribute is a valid flag
    def makeValid(s):
        if not s.type is BooleanType or s.name != "valid":
            return None
        return s.setValue(True)

class Template:

    def __init__(s):
        s.decl = ""
        s.defi = ""
        s.lits = []
        # a dict: to be order independent, in DTD element order is arbitrary
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

    def addSubElement(s, name):
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
        # add this field to default data definition
        Attribute(s.curElement, name, type, value)

    def addStringLiteral(s, lit):
        if lit == "0": bla
        i = 0
        for item in s.lits:
            if item == lit:
                break
            i += 1
        if i == len(s.lits):
            s.lits.append(lit)
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
        return prefix+s.defi+s.elemDefinition()

    def elemDefinition(s):
        res = "\n\n"
        name = "chemical_element"
        type = name+"_t"
        res += type+" "+name+" = {"
        res += stringifyElement(s.defElem[name], 0)
        res += "\n};"
        return res

    def testCode(s):
        postfix = \
            "\n\nint main(int argc, const char * argv[])\n"+\
            "{\n"+\
            "    printf(\"chemical_element_t size: %ld\\n\",sizeof(chemical_element_t));\n"+\
            "    return 0;\n"+\
            "}\n"
        return postfix

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
        lastType = DataTypes.defaultType()
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
            tem.addSubElement(name)
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


def updateAttributes(template, elem, newElem):

    isValid = False # True: non-default value, data was read in
    for item in template.items(): # process attributes
        key = item[0]
        tempAttr = item[1]
        newAttr = None

        if tempAttr.type is ComplexType:
            reValue = elem.get("re")
            imValue = elem.get("im")
            if reValue != None or imValue != None:
                newAttr = tempAttr.setValue([reValue, imValue])
                isValid = True
        else:
            value = elem.get(key)
            if value != None: 
                if isinstance(value, ListType): # probably from template
                    value = value[2]
                # update attribute
                newAttr = tempAttr.setValue(value)
                isValid = True
        # set the default value (even) if there was no data
        if newAttr != None:
            newElem.set(key, newAttr)

    # toggles the valid flag, to signal there was an data update from file
    if isValid:
        newAttr = template.get("valid").makeValid() # copy default data again
        newElem.set("valid", newAttr)


def update(elem):
    if not tem.defElem.has_key(elem.tag): return None
    # maintain element&attrib order from template
    # attributes are not ordered anyway and carry an position index
    newElem = xml.etree.ElementTree.Element(elem.tag) # modified
    template = tem.defElem[elem.tag]                  # must not be modified
    updateAttributes(template, elem, newElem)

    for child in template.getchildren(): # process subelements
        subElem = elem.find(child.tag) # data from file
        newSubElem = None
        if subElem != None:
            # add element with data from file
            newSubElem = update(subElem)
        else:
            # add element with default data from template
            newSubElem = child
        if newSubElem != None:
            newElem.append(newSubElem)

    return newElem


def parseXml(filename):
    filename = os.path.abspath(filename);
    if not os.path.exists(filename):
        raise IOError("File not found!")

    etree = ElementTree(file=filename)
    eroot = etree.getroot() # chemical_element_list
    for elem in eroot.getchildren():
        # we care only about single chemical_elements
        if elem.tag != "chemical_element": continue
        finalElem = update(elem)
        if finalElem == None: continue
        print stringifyElement(finalElem, 0)

#    print tem.elemDefinition()
    print tem.definition()
#    print tem.testCode()



###################################################

tem = Template()

