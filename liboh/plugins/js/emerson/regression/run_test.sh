# load files from the tests directory and run each of them

ECHO="echo"
DIFF="diff"
RM="rm"


EMERSON_EXEC="../../../../../build/cmake/emerson"
TEST_DIR="./tests"
REF_DIR="./refs"

TESTS=`ls $TEST_DIR`


$RM *.em.out

for file in $TESTS
 do
   $EMERSON_EXEC "$TEST_DIR/$file" 2> "$file.out"
    if [ $? -ne 0 ]
      then
         $DIFF "$file.out" "$REF_DIR/$file.ref" > "$file.diff" 
         $ECHO " $file FAILED" 
    else
      $ECHO "$file PASSED"
    fi

 done 


