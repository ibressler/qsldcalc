# gen.py
#
# This file is part of ###.
#
# ### is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# ### is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with ###.  If not, see <http://www.gnu.org/licenses/>.
#
# Authors:
#     Ingo Bressler (May 2010)

import sys
import os
import getopt

from parseDtd import parseDtd
from parseDtd import parseXml

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def cmdName(argv):
    if not argv or len(argv) <= 0:
        return ""
    else:
        return os.path.basename(argv[0])

def showUsage(argv):
    msg = "USAGE: " + cmdName(argv) + " <filename>\n"
    return msg

def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "h", ["help"])
        except getopt.error, msg:
            raise Usage(msg)
    except Usage, err:
        print >>sys.stderr, err.msg
        print >>sys.stderr, cmdName(argv)+": For help use --help"
        return 2
#    print "opts:", opts, "args:", args
    if (unicode("-h"), "") in opts or \
       (unicode("--help"), "") in opts or \
       len(argv) < 3:
        print >>sys.stdout, showUsage(argv)
        return 0
    else:
#        try:
        parseDtd(argv[1])
        parseXml(argv[2])
#        except Exception, e:
#            print "An Error occured:", e

if __name__ == "__main__":
    sys.exit(main())
