# load files from the tests directory and run each of them

ECHO="echo"
DIFF="diff"



EMERSON_EXEC="../../../../../build/cmake/emerson"
TEST_DIR="./tests"
REF_DIR="./refs"

TESTS=`ls $TEST_DIR`

for file in $TESTS
 do
   $ECHO "Testing $file"
   $EMERSON_EXEC "$TEST_DIR/$file" 2> "$file.out"
	 if [ $? -ne 0 ]
	   then
	     $DIFF "$file.out" "$REF_DIR/$file.ref" > "$file.diff" 
		fi

 done 
