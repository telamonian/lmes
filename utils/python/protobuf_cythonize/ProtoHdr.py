from ProtoClass import ProtoClass
import os
import re
import sys

class ProtoHdr(object):
    def __init__(self, fpath, namespace):
        self.classes = []
        self.namespace = namespace
        
        # read in all of the lines from the .pb.h file
        with open(fpath) as f:
            self.lines = f.readlines()
        
        # assemble a list of tuples of form (src line number with class definition, semi_qualified_class_name)
        classNfos = []
        for i,line in enumerate(self.lines):
            search = re.search('^class\s+(\S+)\s.*:.*',line)
            if search:
                classNfos.append((i, search.group(1)))
        
        # initialize a list of class objects with className and the lines making up the definition
        for i,className in classNfos:
            while not re.search('// accessors --------',self.lines[i]):
                i+=1
            j = i
            i+=1
            while not re.search('^\s+private:',self.lines[i]):
                i+=1
            self.classes.append(ProtoClass(classLines=self.lines[j:i], className=className, hdrName=os.path.split(fpath)[-1], namespace=self.namespace))
