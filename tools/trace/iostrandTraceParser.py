#!/usr/bin/python

'''
Reads file generated through
Sirikata::Network::IOService::reportAllStats(filename,detailed) and
parses its contents to track how long callbacks actually take.
Performs some basic statistics on time callbacks take (for now: avg,
max, median).  Existing file can easily be expanded further to perform
additional operations (eg. plot, quartiles, etc.)
'''


import sys;
import re as RegExp;
import math;

#transitions linearly from one state to the next
IOSERVICE_WAITING_FIRST_LINE = 0;
IOSERVICE_READING_HEADER     = 1;
IOSERVICE_READING_SAMPLES    = 2;


##From
##http://code.activestate.com/recipes/52304-static-methods-aka-class-methods-in-python/
class Callable:
    def __init__(self,anycallable):
        self.__call__ = anycallable;


#will fill this in later
class SampleTag():

    
    def __init__(self,tag,time):
        '''
        @param {String} tag
        @param {Number} time (in seconds).
        '''
        self.tag  = tag;
        self.time = time;

    def debugPrintSample(self):
        print('\n------------------\n');
        print('\ttag:     ' + self.tag);
        print('\ttime(s): ' + str(self.time));
        print('\n\n');
        
    def parseSample(line):
        '''
        For strings with the following format, returns SampleTag
        sample: OgreRenderer::parseMeshWork more 157.173ms

        Otherwise, returns None.
        '''
        #starts with "sample: "
        #anything in middle
        #two spaces
        #number+
        #one optional decimpal
        #number+
        #[m,u]s
        #2e is hex for '.'
        isTagExpr = 'sample:\s.*?\s\s\d+[\\x2e]?\d+[m,u]?s';
        allMatches = RegExp.findall(isTagExpr,line);
        if (len(allMatches) == 0):
            return None;

        #tag name
        tagNameExpr = 'sample:\s(.*?)\s\s\d+[\\x2e]?\d+[m,u]?s';
        tagNameMatches = RegExp.findall(tagNameExpr,line);
        if (len(tagNameMatches) != 1):
            errString =  '\n\nError: got ' + str(len(tagNameMatches));
            errString += ' tag name matches for line ' + line + '\n\n';
            print(errString);
            assert(False);

        tagName = tagNameMatches[0];

        #parse out time
        timeNumberExpr    = 'sample:\s.*?\s\s(\d+[\\x2e]?\d+)[m,u]?s';
        timeNumberMatches = RegExp.findall(timeNumberExpr,line);
        if (len(timeNumberMatches) != 1):
            errString =  '\n\nError: got ' + str(len(timeNumberMatches));
            errString += ' time number matches for line ' + line + '\n\n';
            print(errString);
            assert(False);

        timeNumber = float(timeNumberMatches[0]);

        #parse out time units
        timeUnitExpr     = 'sample:\s.*?\s\s\d+[\\x2e]?\d+([m,u]?s)';
        timeUnitMatches  =  RegExp.findall(timeUnitExpr,line);
        if (len(timeUnitMatches) != 1):
            errString =  '\n\nError: got ' + str(len(timeUnitMatches));
            errString += ' time unit matches for line ' + line + '\n\n';
            print(errString);
            assert(False);

        timeUnit = timeUnitMatches[0];
        multiplier = 1;
        if(timeUnit == 's'):
            pass;
        elif(timeUnit == 'us'):
            multiplier = .000001;
        elif(timeUnit == 'ms'):
            multiplier = .001
        else:
            print('\n\nUnknown time unit: ' + timeUnit + '\n\n');
            assert(False);

        return SampleTag(tagName,multiplier*timeNumber);

    
    #make parseSample a static member of Sampletag.
    parseSample = Callable(parseSample);
        
class IOService():

    def __init__ (self):
        self.header = '';
        #each of type SampleTag
        self.samples = [];
        self.state   = IOSERVICE_WAITING_FIRST_LINE;

    def printStatistics(self):
        total = 0.;
        maxSample =  SampleTag('noTag',0);
        for s in self.samples:
            total += s.time;
            if (maxSample.time < s.time):
                maxSample.time = s.time;
                maxSample.tag  = s.tag;
                
        print('\n');
        print('-----------------');
        print(self.header);
        print('\n');
        avgTimeMsg = 'No samples: division by zero prevents avg';
        maxTimeMsg = 'No samples: max meaningless';
        medianTimeMsg = 'No samples: median meaningless';
        if (len(self.samples) != 0):
            avgTimeMsg = str(total/float(len(self.samples)));
            maxTimeMsg = str(maxSample.time) + '  for  ' + maxSample.tag;
            timeList = [x.time for x in self.samples];
            timeList.sort();
            medianIndex = int(math.floor(len(timeList)/2));
            medianTimeMsg = str(timeList[medianIndex]);
            
            
        print('Avg:    ' + avgTimeMsg);
        print('Max:    ' + maxTimeMsg);
        print('Median: ' + medianTimeMsg);
        print('\n\n');
        
    def addLineOfText(self,textToAdd):
        '''
        @return {bool} True if this text should be added to this
        IOService, False otherwise.
        '''
        if (self.state == IOSERVICE_WAITING_FIRST_LINE):

            if (textToAdd.find('==============') != -1):
                self.state = IOSERVICE_READING_HEADER;
                
            return True;
                
        elif(self.state == IOSERVICE_READING_HEADER):

            #done reading: this is not part of our ioservice
            if (textToAdd.find('==============') != -1):
                return False;

            #state transition when report sample size
            if (textToAdd.find('Sample size: ') != -1):
                self.state = IOSERVICE_READING_SAMPLES

            self.header += textToAdd + '\n';
            return True;
            

        elif(self.state == IOSERVICE_READING_SAMPLES):

            #done reading: this is not part of our ioservice
            if (textToAdd.find('==============') != -1):
                return False;

            sTag = SampleTag.parseSample(textToAdd);
            if (sTag != None):
                self.samples.append(sTag);

            return True;
            
        else:
            print('\n\nError: in unknown state\n\n');
            assert(False);
            


            
def parseFile(filename):

    filer = open (filename,'r');
    allIOServices = []
    allIOServices.append(IOService());

    for line in filer:
        #if addition of line of text fails for current ioservice (ie,
        #it's the beginning of a trace for a new ioservice, then
        #create a new IOService object.
        if(not allIOServices[-1].addLineOfText(line)):
            #create new ioservice, and append line of text to it.
            allIOServices.append(IOService());
            if (not allIOServices[-1].addLineOfText(line)):
                print('\n\nError.  No IOService will claim line: ' + line);
                print('\n\n');
                assert(False);

    #basic print out at end of what averaged out to longest.
    for s in allIOServices:
        s.printStatistics();


if __name__ == "__main__":

    if (len(sys.argv) != 2):
        print('Usage error: requires name of file to parse');
    else:
        parseFile(sys.argv[1]);


