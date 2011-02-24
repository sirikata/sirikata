/*
  Copyright 2008 Chris Lambrou.
  All rights reserved.
*/

// This is the Tree grammar for Emerson language

tree grammar LexWhenPredTree;

options
{
	//output=template;
	backtrack=true;
	rewrite=true;
	ASTLabelType=pANTLR3_BASE_TREE;
	tokenVocab=LexWhenPred;
        language = C;
}

@header
{
    #include <stdlib.h>
    #include <string.h>
    #include <antlr3.h>
    #include "../emerson/Util.h"; 

    #define APPLEX(s) program_string_lex->append(program_string_lex, s);           
}

@members
{
    pANTLR3_STRING program_string_lex;
}


program returns [pANTLR3_STRING  s]
	:^(PROG 
            {
                pANTLR3_STRING_FACTORY factory = antlr3StringFactoryNew();
                program_string_lex = factory->newRaw(factory);
            }
            sourceElements
     )
        {
            s = program_string_lex;
        }
	;

sourceElements
    :(sourceElement{APPLEX("\n"); })+  // omitting the LT 
    ;
	
sourceElement
    : whenPredStatement{ }
    ;
	

// statements
whenPredStatement
        : ^(
            WHEN_PRED_BLOCK
            {
                APPLEX("\n\nParsing when pred block: \n");
            }
            (id1=Identifier
            {
                APPLEX("\n\nThis is an identifier I found: ");
                APPLEX((const char*)$id1.text->chars);
                APPLEX("\n\n");
            }
            )
            (id2=Identifier
            {
                APPLEX("\n\nThis is an identifier I found: ");
                APPLEX((const char*)$id2.text->chars);
                APPLEX("\n\n");
            }
            )*
           )
	;


// primitive literal definition.
literal
        : StringLiteral
          {
          }
	| NumericLiteral
          {
          }
	;
	
