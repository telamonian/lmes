#!/usr/bin/env python

from collections import deque
import csv
import os,sys
from six import print_
import re

# class File(file):
#     """ An helper class for file reading  """
#
#     def __init__(self, *args, **kwargs):
#         super(File, self).__init__(*args, **kwargs)
#         self.BLOCKSIZE = 4096
#
#     def head(self, lines_2find=1):
#         self.seek(0)                            #Rewind file
#         return [super(File, self).next() for x in xrange(lines_2find)]
#
#     def tail(self, lines_2find=1):
#         self.seek(0, 2)                         #Go to end of file
#         bytes_in_file = self.tell()
#         lines_found, total_bytes_scanned = 0, 0
#         while (lines_2find + 1 > lines_found and
#                bytes_in_file > total_bytes_scanned):
#             byte_block = min(
#                 self.BLOCKSIZE,
#                 bytes_in_file - total_bytes_scanned)
#             self.seek( -(byte_block + total_bytes_scanned), 2)
#             total_bytes_scanned += byte_block
#             lines_found += self.read(self.BLOCKSIZE).count('\n')
#         self.seek(-total_bytes_scanned, 2)
#         line_list = list(self.readlines())
#         return line_list[-lines_2find:]
#
#     def backward(self):
#         self.seek(0, 2)                         #Go to end of file
#         blocksize = self.BLOCKSIZE
#         last_row = ''
#         while self.tell() != 0:
#             try:
#                 self.seek(-blocksize, 1)
#             except IOError:
#                 blocksize = self.tell()
#                 self.seek(-blocksize, 1)
#             block = self.read(blocksize)
#             self.seek(-blocksize, 1)
#             rows = block.split('\n')
#             rows[-1] = rows[-1] + last_row
#             while rows:
#                 last_row = rows.pop(-1)
#                 if rows and last_row:
#                     yield last_row
#         yield last_row

def ReverseReadline(filename):
    with open(filename) as qfile:
        qfile.seek(0, os.SEEK_END)
        position = qfile.tell()
        line = ''
        while position >= 0:
            qfile.seek(position)
            next_char = qfile.read(1)
            if next_char == "\n":
                yield line[::-1]
                line = ''
            else:
                line += next_char
            position -= 1
        yield line[::-1]

def getProgress(fPath):
    lines = deque()
    # f = File(fPath)
    for line in ReverseReadline(fPath): #f.backward():
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
                
    outputOne = []
    outputOne.append(',count')
    for key,val in sorted(outDictOne.items()):
        outputOne.append('%s,%s' % (key, val))
    
    outputTwo = []
    outputTwo.append(',count,average')
    for key,val in sorted(outDictTwo.items()):
        outputTwo.append('%s,%s,%s' % (key, val, float(val)/outDictOne['TOTAL']))
    
    return outputOne,outputTwo

def pretty_print(input, **options):
    """
    Pretty print a CSV file
    """

    #function specific options
    new_delimiter           = options.pop("new_delimiter", " | ")
    border                  = options.pop("border", True)
    border_vertical_left    = options.pop("border_vertical_left", "| ")
    border_vertical_right   = options.pop("border_vertical_right", " |")
    border_horizontal       = options.pop("border_horizontal", "-")
    border_corner_tl        = options.pop("border_corner_tl", "+ ")
    border_corner_tr        = options.pop("border_corner_tr", " +")
    border_corner_bl        = options.pop("border_corner_bl", border_corner_tl)
    border_corner_br        = options.pop("border_corner_br", border_corner_tr)
    header                  = options.pop("header", True)
    border_header_separator = options.pop("border_header_separator", border_horizontal)
    border_header_left      = options.pop("border_header_left", border_corner_tl)
    border_header_right     = options.pop("border_header_right", border_corner_tr)
    newline                 = options.pop("newline", "\n")

    column_max_width = {} #key:column number, the max width of each column
    num_rows = 0 #the number of rows

    reader=csv.reader(input, **options)
    for row in reader:
        num_rows += 1
        for col_number, column in enumerate(row):
            width = len(column)
            try:
                if width > column_max_width[col_number]:
                    column_max_width[col_number] = width
            except KeyError:
                column_max_width[col_number] = width

    max_columns = max(column_max_width.keys()) + 1 #the max number of columns (having rows with different number of columns is no problem)

    if max_columns > 1:
        total_length = sum(column_max_width.values()) + len(new_delimiter) * (max_columns - 1)
        left = border_vertical_left if border is True else ""
        right = border_vertical_right if border is True else ""
        left_header = border_header_left if border is True else ""
        right_header = border_header_right if border is True else ""
    
        reader=csv.reader(input, **options)
        for row_number, row in enumerate(reader):
            max_index = len(row) - 1
            for index in range(max_columns):
                if index > max_index:
                    row.append(' ' * column_max_width[index]) #append empty columns
                else:
                    diff = column_max_width[index] - len(row[index])
                    row[index] = row[index] + ' ' * diff #append spaces to fit the max width

            if row_number==0 and border is True: #draw top border
                print_(border_corner_tl + border_horizontal * total_length + border_corner_tr + newline)
            print_(left + new_delimiter.join(row) + right + newline) #print the new row
            if row_number==0 and header is True: #draw header's separator
                print_(left_header + border_header_separator * total_length + right_header + newline)
            if row_number==num_rows-1 and border is True: #draw bottom border
                print_(border_corner_bl + border_horizontal * total_length + border_corner_br)

if __name__=='__main__':
    outputOne,outputTwo = getProgress(fPath=sys.argv[1])
    pretty_print(outputOne)
    pretty_print(outputTwo)