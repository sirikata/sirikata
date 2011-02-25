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
                APPLEX("\n");
                //open when_watched_list
                APPLEX("util.create_when_watched_list([");
            }
            sourceElements
            {
                //close when_watched_list
                APPLEX("]);");
            }
          )
        {
            s = program_string_lex;
        }
	;

sourceElements
    :(
        sourceElement
        {
            //closes the last paren associated with util.create_when_watched_item
            APPLEX(")\n");
        }
      )+  // omitting the LT 
    ;
	
sourceElement
    :
    (
        whenPredStatement{ }
    )
    ;


// statements
whenPredStatement
        : ^(
            WHEN_PRED_BLOCK
            {
                APPLEX("\nutil.create_when_watched_item(['");
            }
            id1=identifier
            {
                APPLEX("]");
            }
            (
                {
                    APPLEX("),");
                }
                whenPredStatement
                {
                }
            )*
           )
	;

identifier
        : ^(
            IDENTIFIER
            {
            }
            Identifier
            {
                APPLEX((const char*)$Identifier.text->chars);
                APPLEX("'");
            }
            (
                {
                    APPLEX(",'");
                }
                dottedIdentifier
                {
                }
            )*
           )
        ;

            
dottedIdentifier
        : ^( 
            DOTTED_IDENTIFIER
            {
            }
            id1=Identifier
            {
                APPLEX((const char*)$id1.text->chars);
                APPLEX("'");   
            }
            (
                {
                    APPLEX(",'");
                }
                dottedIdentifier
                {
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
	
