#!/usr/bin/python

import entity

class CSVGenerator:
    def __init__(self, entityConstructorInfo=[]):
        self.allEntities = entityConstructorInfo;

    def add(self,constInfo):
        self.allEntities.append(constInfo)

    def write(self,filenameToWriteTo):
        fileOut = open(filenameToWriteTo,'w');

        fileOut.write(entity.getCSVHeaderAsCSVString());
        fileOut.write('\n');
        for s in self.allEntities:
            fileOut.write(s.getConstructorRow());
            fileOut.write('\n');

        fileOut.flush();
        fileOut.close();
