#!/bin/sh


LIB="antlr/lib/"
ANTLR_LIB="$LIB/antlr-3.1.3.jar"
ST_LIB="$LIB/stringtemplate-3.2.jar"
SRC_DIR="./"

CP="$ANTLR_LIB:$ST_LIB"
java -cp "$CP" org.antlr.Tool "$SRC_DIR/Emerson.g"
java -cp "$CP" org.antlr.Tool "$SRC_DIR/EmersonTree.g"


# move the c files to C++

mv EmersonTree.c EmersonTree.cpp
mv EmersonLexer.c EmersonLexer.cpp
mv EmersonParser.c EmersonParser.cpp



