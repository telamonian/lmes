#!/usr/bin/env python3

from collections import deque
import os,sys
from six import print_
import re

class File(file):
    """ An helper class for file reading  """

    def __init__(self, *args, **kwargs):
        super(File, self).__init__(*args, **kwargs)
        self.BLOCKSIZE = 4096

    def head(self, lines_2find=1):
        self.seek(0)                            #Rewind file
        return [super(File, self).next() for x in xrange(lines_2find)]

    def tail(self, lines_2find=1):  
        self.seek(0, 2)                         #Go to end of file
        bytes_in_file = self.tell()
        lines_found, total_bytes_scanned = 0, 0
        while (lines_2find + 1 > lines_found and
               bytes_in_file > total_bytes_scanned): 
            byte_block = min(
                self.BLOCKSIZE,
                bytes_in_file - total_bytes_scanned)
            self.seek( -(byte_block + total_bytes_scanned), 2)
            total_bytes_scanned += byte_block
            lines_found += self.read(self.BLOCKSIZE).count('\n')
        self.seek(-total_bytes_scanned, 2)
        line_list = list(self.readlines())
        return line_list[-lines_2find:]

    def backward(self):
        self.seek(0, 2)                         #Go to end of file
        blocksize = self.BLOCKSIZE
        last_row = ''
        while self.tell() != 0:
            try:
                self.seek(-blocksize, 1)
            except IOError:
                blocksize = self.tell()
                self.seek(-blocksize, 1)
            block = self.read(blocksize)
            self.seek(-blocksize, 1)
            rows = block.split('\n')
            rows[-1] = rows[-1] + last_row
            while rows:
                last_row = rows.pop(-1)
                if rows and last_row:
                    yield last_row
        yield last_row

def getProgress(fPath):
    lines = deque()
    f = File(fPath)
    for line in f.backward():
        if re.search('Trajectory status', line):
            break
        lines.appendleft(line)
    outDictOne = {}
    outDictTwo = {}
    for line in lines:
        rex = re.search('(\d+)'+'\s+'+
                      '([a-zA-Z]+)'+'\s+'+
                      '(-?(?:0|[1-9]\d*)(?:\.\d*)?(?:[eE][+\-]?\d+)?)'+'\s+'+
                      '(\d+)\s*\n?$', 
                      line)
        if rex:
            if rex.group(2) in ['FINISHED','RUNNING','WAITING']:
                outDictOne[rex.group(2)] = outDictOne.get(rex.group(2), 0) + 1
                outDictOne['TOTAL'] = outDictOne.get('TOTAL', 0) + 1
                outDictTwo['TIME'] = outDictTwo.get('TIME', 0.0) + float(rex.group(3))
                outDictTwo['WORKUNITCOUNT'] = outDictTwo.get('WORKUNITCOUNT', 0) + int(rex.group(4))
    print_('\t' + '\t' + 'count')
    for key,val in sorted(outDictOne.items()):
        print_(key + '\t' + str(val))
    print_('\t' + 'count' + '\t' + 'average')
    for key,val in sorted(outDictTwo.items()):
        print_(key + '\t' + str(val) + '\t' + str(float(val)/outDictOne['TOTAL']))

if __name__=='__main__':
    getProgress(fPath=sys.argv[1])