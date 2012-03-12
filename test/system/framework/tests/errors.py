#!/usr/bin/python

'''
Returned when we call performErrorCheck on an Error object.
'''
class ErrorReturn:
    def __init__(self, errorExists, errorMessage):
        self.errorExists = errorExists;
        self.errorMessage = errorMessage;

    def getErrorExists(self):
        return self.errorExists;
    def getErrorMessage(self):
        return self.errorMessage;


class Error:

    '''
    @param errorName {String} The name of this error check.  Will be
    printed to output on success or failure.  To avoid confustion, try
    to make unique from other names.

    @param errorCheckFunc {Function} function that takes a single
    argument: the file text to check through.  Function should return
    an ErrorReturn object with TRUE as its errorExists parameter if
    there is an error with the output and FALSE if there is no error
    in the output.

    '''
    def __init__(self,errorName, errorCheckFunc):
        self.errorName = errorName;
        self.errorCheckFunc = errorCheckFunc;


    def getName(self):
        return self.errorName;

    '''
    @param {String} runOutputText Text output printed from running the
    trial.

    @return {bool} TRUE if there is an error in the output, FALSE if
    there is not.
    '''
    def performErrorCheck(self,runOutputText):
        return self.errorCheckFunc(runOutputText);


'''
@param toCheckFor {String}

Returns a lambda expression that takes in a single text argument.  If
the text argument the lambda takes in contains toCheckFor, then the
lambda expression returns TRUE.  Otherwise, returns FALSE.

'''
def lambda_checkForTextError(toCheckFor):
    return lambda text : lambda_checkForTextErrorHelper(text,toCheckFor);

def lambda_checkForTextErrorHelper(text,toCheckFor):
    if (text.find(toCheckFor) != -1):
        return ErrorReturn(True, 'Found instance of "' + toCheckFor + '" in output file');

    return ErrorReturn(False,'');

'''
@see lambda_checkForTextError.  Except, in this case, we throw an
error if we *do not* see toCheckFor in text.
'''
def lambda_checkForNotInTextError(toCheckFor):
    return lambda text : lambda_checkForNotInTextErrorHelper(text,toCheckFor);

def lambda_checkForNotInTextErrorHelper(text,toCheckFor):
    if (text.find(toCheckFor) == -1):
        return ErrorReturn(True, 'Could not find instance of "' + toCheckFor + '" in output file');

    return ErrorReturn(False,'');



'''
All of these conditions have to be true for there to be an error.
'''
def lambda_andError(*args,**kwargs):
    return lambda text: reduce( lambda x,y: lambda_andErrorHelper(x(text), y(text)), args);

'''
 @param lhs {ErrorReturn}
 @param rhs {ErrorReturn}

 @return {ErrorReturn} If both lhs and rhs error returns have true for
 their errorExists, then return a new ErrorReturn that combines their
 error messages and has errorExists set to true.  Otherwise, return a
 new ErrorReturn with no error message and errorExists set to False.
'''
def lambda_andErrorHelper(lhs, rhs):
    if (lhs.getErrorExists() and rhs.getErrorExists):
        return ErrorReturn (True, lhs.getErrorMessage() + ' AND ' + rhs.getErrorMessage());

    return ErrorReturn (False,'');


'''
One of these conditions have to be true for there to be an error.
'''
def lambda_orError(*args,**kwargs):
    return lambda text: reduce( lambda x,y: lambda_orErrorHelper(x(text), y(text)), args);


'''
 @param lhs {ErrorReturn}
 @param rhs {ErrorReturn}

 @return {ErrorReturn} If both lhs and rhs error returns have true for
 their errorExists and we mash their error messages together, if only
 one of the two sides is an error, then we just return whichever had
 an error.  If neither of them had an error, just return a new
 ErrorReturn with no error message and errorExists set to False.
'''
def lambda_orErrorHelper(lhs,rhs):
    if (lhs.getErrorExists() and rhs.getErrorExists):
        return ErrorReturn (True, lhs.getErrorMessage() + ' ;;; ' + rhs.getErrorMessage());
    if (lhs.getErrorExists()):
        return lhs;
    if (rhs.getErrorExists()):
        return rhs;

    return ErrorReturn (False,'');


UnitTestFailError      = Error("Unit Test Fail Error", lambda_checkForTextError("UNIT_TEST_FAIL"));
ExceptionError         = Error("Emerson Exception Error", lambda_checkForTextError("Exception"));


'''
Testing the testers.
'''
if __name__ == "__main__":

    text = 'behram is writing error checks';

    errorFunc  =  lambda_checkForTextError('behram');
    errorFunc2 =  lambda_checkForTextError('behraM');
    # print('\n\n');
    # print(errorFunc(text));
    # print('\n\n');
    # print(errorFunc2(text));
    # print('\n\n');

    errorFunc3 = lambda_andError(errorFunc,  errorFunc);
    print('\n\n');
    print(errorFunc3(text).getErrorMessage());
    print('\n\n');
