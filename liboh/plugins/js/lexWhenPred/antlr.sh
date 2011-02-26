#!/bin/sh

EMERSON_FOLDER="../emerson"
LIB=$EMERSON_FOLDER"/antlr/lib/"
ANTLR_LIB="$LIB/antlr-3.1.3.jar"
ST_LIB="$LIB/stringtemplate-3.2.jar"
SRC_DIR="./"

CP="$ANTLR_LIB:$ST_LIB"
java -cp "$CP" org.antlr.Tool "$SRC_DIR/LexWhenPred.g"
java -cp "$CP" org.antlr.Tool "$SRC_DIR/LexWhenPredTree.g"


# move the c files to C++

mv LexWhenPredTree.c   LexWhenPredTree.cpp
mv LexWhenPredLexer.c  LexWhenPredLexer.cpp
mv LexWhenPredParser.c LexWhenPredParser.cpp



