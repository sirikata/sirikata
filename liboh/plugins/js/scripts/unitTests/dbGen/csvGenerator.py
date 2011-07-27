#!/usr/bin/python

import csvConstructorInfo

class CSVGenerator:
    def __init__(self, entityConstructorInfo=[]):
        self.allEntities = entityConstructorInfo;

    def addEntity(self,constInfo):
        self.allEntities.append(constInfo);

    def addArrayOfEntities(self,arrayConstInfo):
        for s in arrayConstInfo:
            self.addEntity(s);
        
    def addDefaultEntity(self):
        self.addEntity(CSVConstructorInfo());
        
    def writeDB(self,filenameToWriteTo):
        fileOut = open(filenameToWriteTo,'w');
        
        fileOut.write(csvConstructorInfo.getCSVHeaderAsCSVString());
        fileOut.write('\n');
        for s in self.allEntities:
            fileOut.write(s.getConstructorRow());
            fileOut.write('\n');

        fileOut.flush();
        fileOut.close();

