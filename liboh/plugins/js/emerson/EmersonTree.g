/*
  Copyright 2008 Chris Lambrou.
  All rights reserved.
*/

// This is the Tree grammar for Emerson language

tree grammar EmersonTree;

options
{
	//output=template;
	backtrack=true;
//	memoize=true;
	rewrite=true;
	ASTLabelType=pANTLR3_BASE_TREE;
//	ASTLabelType=pANTRL3_COMMON_TREE;
	tokenVocab=Emerson;
 language = C;
}

@header
{
    #include <stdlib.h>
    #include <string.h>
    #include <antlr3.h>
    #include "EmersonUtil.h"
    #define APP(s) program_string->append(program_string, s);
    
    //	pANTLR3_STRING program_string;

    #ifndef __SIRIKATA_INSIDE_WHEN_PRED__
    #define __SIRIKATA_INSIDE_WHEN_PRED__
    static bool insideWhenPred = false;
    #endif
}

@members
{
    pANTLR3_STRING program_string;
}


program returns [pANTLR3_STRING  s]
	:^(PROG 
            {
                pANTLR3_STRING_FACTORY factory = antlr3StringFactoryNew();
                program_string = factory->newRaw(factory);
            }
            sourceElements
     )
        {
            s = program_string;
        }
	;

sourceElements
    :(sourceElement{APP("\n"); })+  // omitting the LT 
    ;
	
sourceElement
    : functionDeclaration
    | statement{ APP(";"); }
    ;
	
// functions
functionDeclaration
    : ^( FUNC_DECL
            {
                APP("function ");
            }
            Identifier
	        {
                APP((const char*)$Identifier.text->chars);
                APP("( ");
            } 
            formalParameterList 
            {
                APP(" )");
                APP("\n{\n");
            }
            functionBody
            {
                APP("\n}");
            }
            
        );

functionExpression
	: ^( FUNC_EXPR 
            {
                APP("function ");
            }
            (			
                Identifier
                {
                    APP((const char*)$Identifier.text->chars);
                }
            )?  
            {
                APP("( ");
            }
            formalParameterList 
            {
                APP("  )");
                APP("\n{\n");
            }
            functionBody
            {
                APP("\n}");
            }
        ); 
	
formalParameterList
	: ^( FUNC_PARAMS 
            (id1=Identifier
                {
                    APP((const char*)$id1.text->chars);
                    APP(" ");
                }
            )?		
            (
                id2=Identifier
                {
                    APP(", ");
                    APP((const char*)$id2.text->chars);
                    APP(" ");
                }
                
            )*
        );

functionBody
	: sourceElements
	| EMPTY_FUNC_BODY 
	;

// statements
statement
    : statementBlock
    | variableStatement
    | expressionStatement
    | ifStatement
    | iterationStatement
    | continueStatement
    | breakStatement
    | returnStatement
    | withStatement
    | labelledStatement
    | switchStatement
    | throwStatement
    | whenStatement
    | tryStatement
    | msgSendStatement
    | msgRecvStatement
	;
	
statementBlock
	: {APP(" {\n "); } statementList {  
            APP(" }\n");
        }
	;
	
statementList
	:  ^(
            SLIST 
            (statement
                {
			        APP("; \n");					  
                }
            )+
	    );
	
variableStatement
	:  ^( 
            VARLIST
            {
                APP("var ");
            }
            variableDeclarationList
        );
	
variableDeclarationList
	: variableDeclaration 
        (
            {
                APP(", ");
            }	
            variableDeclaration
        )*
	;
	
variableDeclarationListNoIn
	: variableDeclarationNoIn+
	;
	
variableDeclaration
	: ^(
            VAR
            Identifier 
            {
                APP((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APP(" = ");
                }
                initialiser
            )?
        )
	;
	
variableDeclarationNoIn
	: 
        ^(
            VAR
			{
                APP("var ");
			}
            Identifier 
            {
                APP((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APP(" = ");
                }
                initialiserNoIn
            )?
					
        )
	;
	
initialiser
	: assignmentExpression 
	;
	
initialiserNoIn
	: assignmentExpressionNoIn
	;
	
	/*
emptyStatement
	: 
	;
	*/
	
expressionStatement
	: expression
	;
	
ifStatement
	: ^(IF 
            {
                
                APP(" if ");
                APP(" ( ");
            }
            expression 
            {
                APP(" ) ");
            }
            statement 
            {
                APP(" \n");
            }
            (
                {
                    APP(" else ");
                }
                statement
                
            )?
        )
	;
	
iterationStatement
	: doWhileStatement
	| whileStatement
	| forStatement
	| forInStatement
	;
	
doWhileStatement
	: ^( 
            DO
            {
                APP(" do ");  						  
            }    
	        statement
            {
                APP("while ( " );      
            }
            expression 
            {
                APP(" ) ");  
            }
        )
	;
	
whileStatement
	: ^(
            WHILE
            {
                APP(" while ( ");
            }
            expression 
            {
                APP(" ) "); 
            }
            statement
        )
	;
	
forStatement
	: ^(
            FOR 
            {
                APP(" for ( ");
            }
            (^(FORINIT forStatementInitialiserPart))?
            {
                APP(" ; ");
            }
            (^(FORCOND expression))? 
            {
                APP(" ; ");
            }
            (^(FORITER expression))?  
            {
                APP(" ) ");
            }
            statement
        )
	;
	
forStatementInitialiserPart
    : expressionNoIn
    | ^(VARLIST variableDeclarationListNoIn)
    ;
	
forInStatement
	: ^(
        FORIN 
        {
            APP(" for ( ");
        }

        forInStatementInitialiserPart 
        {
            APP(" in ");
        }
        expression 
        {
            APP(" ) ");
        }
        statement
    )
	;
	
forInStatementInitialiserPart
	: leftHandSideExpression
	| ^(VAR variableDeclarationNoIn)
	;

continueStatement
    : ^(
        CONTINUE 
        {
            APP("continue ");
        } 
        (
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
            }
        )?
      )
	;

breakStatement
    : ^(
        BREAK
        {
            APP("break ");
        }
        (
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
            }
        )?
        )
	;

returnStatement
    : ^(
        RETURN 
        {
            APP("return ");
        }
        (		
            expression
        )?
       )
	;
	
withStatement
    : ^(WITH expression statement)
    ;

labelledStatement
    : ^( LABEL 
        Identifier 
        {
            APP((const char*)$Identifier.text->chars);
            APP(" : \n");
        }
        statement
        )
	;
	
switchStatement
    : ^(
        SWITCH 
        {
            APP(" switch ( ");
        }
        expression 
        {
            APP(" ) \n");
            APP("{ \n");
        }
        caseBlock
        {
            APP("} \n");
        }
       )
	;
	
caseBlock
    : (caseClause)* (defaultClause*)? (caseClause*)?
    ;

caseClause
    : ^( CASE expression statementList?)
    ;
	
defaultClause
    :^(DEFAULT statementList?)
    ;
	
throwStatement
    : ^(THROW expression)
    ;

whenStatement
    : ^(WHEN
        {
            APP(" util.create_when( ");
            insideWhenPred = true;
            APP(" util.create_when_pred( ['");
        }
        whenPred
        {
            //FIXME: potential problem if last statement in array is
            //dollar syntax.
            //FIXME: still have to escape ' if in when predicate
            //close when predicate
            APP("'])\n");
            
            //comma for end of predicate expression
            //[ for beginning of array of watched statements
            APP(",[");
        }
        whenCheckedListFirst
        {
            insideWhenPred = false;
            //end the array of watched variables.  then end the actual
            //util.create_when statement with parenthesis and semi-colon.
            APP("]);");
        }
        expression
        {
        }
    )
    ;

    
whenPred
    : ^(WHEN_PRED
        expression
    )
    ;

whenCheckedListFirst
    : ^(WHEN_CHECKED_LIST_FIRST
        {
        }
        expression
        {
            //expression will automatically fill in correct values here
        }
        (whenCheckedListSubsequent
        {
        })?
    )
    ;    


whenCheckedListSubsequent
    : ^(WHEN_CHECKED_LIST_SUBSEQUENT
        {
            APP(",");
        }
        expression
        {
            //expression will automatically fill in correct values here
        }
        (whenCheckedListSubsequent
        {
        })*
        )
    ;
        
    

tryStatement
    : ^(TRY 
	      statementBlock 
          finallyClause?
        ) 
	;
       

msgSendStatement
scope{
  pANTLR3_STRING prev_program_string;
	unsigned int  prev_program_len;
	char* firstExprString;
	char* secondExprString;
  pANTLR3_STRING init_program_string;

}
 : ^(
	     MESSAGE_SEND 
       /* A little hack for the things to work */
       
			 {
			 /* Save the program string here */
			   $msgSendStatement::prev_program_string = program_string;
       /* length of the program string */
			   $msgSendStatement::prev_program_len = $msgSendStatement::prev_program_string->len;
         pANTLR3_STRING_FACTORY factory = antlr3StringFactoryNew();
				 $msgSendStatement::init_program_string = factory->newRaw(factory);
				 $msgSendStatement::init_program_string->setS($msgSendStatement::init_program_string, program_string);
       }

	     leftHandSideExpression 
				  {
					   unsigned int prev_program_len = $msgSendStatement::prev_program_len;

			       unsigned int  new_program_len = program_string->len;
             $msgSendStatement::firstExprString = (char*)(malloc(new_program_len - prev_program_len + 1) );
						 memset($msgSendStatement::firstExprString, 0, (new_program_len - prev_program_len + 1));
						 memcpy($msgSendStatement::firstExprString, (char*)(program_string->chars) + prev_program_len, (new_program_len - prev_program_len) );
						 
             $msgSendStatement::prev_program_len = new_program_len; 
						  //APP(".sendMessage( ");
					}
				leftHandSideExpression 
				    {
						  unsigned int prev_program_len = $msgSendStatement::prev_program_len;
						  unsigned int new_program_len = program_string->len;
              $msgSendStatement::secondExprString = (char*)(malloc(new_program_len - prev_program_len + 1) );
							memset($msgSendStatement::secondExprString, 0, new_program_len - prev_program_len + 1);
							memcpy($msgSendStatement::secondExprString, (char*)(program_string->chars) + prev_program_len, (new_program_len - prev_program_len));

              pANTLR3_STRING init_program_string = $msgSendStatement::init_program_string;
              init_program_string->append(init_program_string, $msgSendStatement::secondExprString);
              init_program_string->append(init_program_string, ".sendMessage( ");
              init_program_string->append(init_program_string, $msgSendStatement::firstExprString);

							program_string->setS(program_string, init_program_string); 

						}      				
				(
				
				 {
					  APP(", ");
					}
				memberExpression
			 		{
					  
					}	
				)?
						{
						  APP(" ) ");
				    }

				)

				;

msgRecvStatement
 : ^(
	    MESSAGE_RECV
	    {
					  APP("system.registerHandler( ");
			}

					memberExpression
     {
            APP(", null");
					  APP(", ");
					} 
     leftHandSideExpression

			)
    {
				  APP(", null) ");  // No sender case
				}
   
 |^(
	    MESSAGE_RECV
	    {
					  APP("system.registerHandler( ");
			}

					memberExpression
     {
            APP(", null");
					  APP(", ");
					} 
     leftHandSideExpression

					
					 {
						  APP(", ");
						}

						memberExpression
					
				)
    {
				  APP(") "); // Case with sender
				}



;

catchClause
	: ^(CATCH 
	    {
					  APP(" catch ( ");
					}
	    Identifier 
					{
					  APP((const char*)$Identifier.text->chars);
					  APP(" ) ");

					}
					
					statementBlock)
	   
	;
	
finallyClause
	: ^( FINALLY 
	   {
				  APP(" finally ");

				}
	   statementBlock 
				
				)
	;

// expressions
expression
	: ^(EXPR_LIST assignmentExpression+)
	;
	
expressionNoIn
	: ^(EXPR_LIST assignmentExpressionNoIn+)

	;
	

assignmentExpression
scope
{
  char* op;
}

	: ^(COND_EXPR conditionalExpression)
	| ^(
	    (
					ASSIGN               { $assignmentExpression::op = " = ";    }
					|MULT_ASSIGN         { $assignmentExpression::op = " *= ";  }
					|DIV_ASSIGN          { $assignmentExpression::op = " /= ";  }
					| MOD_ASSIGN         { $assignmentExpression::op = " \%= ";  }
					| ADD_ASSIGN         { $assignmentExpression::op = " += ";  } 
					| SUB_ASSIGN         { $assignmentExpression::op = " -= ";  } 
					|LEFT_SHIFT_ASSIG    { $assignmentExpression::op = " <<= "; }
					|RIGHT_SHIFT_ASSIGN  { $assignmentExpression::op = " >>= "; }
					|TRIPLE_SHIFT_ASSIGN { $assignmentExpression::op = "  >>>= "; }
					|AND_ASSIGN          { $assignmentExpression::op = " &= "; }
					|EXP_ASSIGN          { $assignmentExpression::op  = " ^= "; }
					|OR_ASSIGN           { $assignmentExpression::op = " |= "; } 
					)

					leftHandSideExpression 
					  {
							  APP(" ");
							  APP($assignmentExpression::op);
							  APP(" ");
							}
							
							assignmentExpression
						 	
						)
	
	;
	
assignmentExpressionNoIn

scope
{
  char* op;
}
	: ^(COND_EXPR_NOIN conditionalExpressionNoIn)
 | ^(
	    (
					ASSIGN               { $assignmentExpressionNoIn::op = " = ";    }
					|MULT_ASSIGN         { $assignmentExpressionNoIn::op = " *= ";  }
					|DIV_ASSIGN          { $assignmentExpressionNoIn::op = " /= ";  }
					| MOD_ASSIGN         { $assignmentExpressionNoIn::op = " \%= ";  }
					| ADD_ASSIGN         { $assignmentExpressionNoIn::op = " += ";  } 
					| SUB_ASSIGN         { $assignmentExpressionNoIn::op = " -= ";  } 
					|LEFT_SHIFT_ASSIG    { $assignmentExpressionNoIn::op = " <<= "; }
					|RIGHT_SHIFT_ASSIGN  { $assignmentExpressionNoIn::op = " >>= "; }
					|TRIPLE_SHIFT_ASSIGN { $assignmentExpressionNoIn::op = "  >>>= "; }
					|AND_ASSIGN          { $assignmentExpressionNoIn::op = " &= "; }
					|EXP_ASSIGN          { $assignmentExpressionNoIn::op  = " ^= "; }
					|OR_ASSIGN           { $assignmentExpressionNoIn::op = " |= "; } 
					)
     
					leftHandSideExpression
     {
					  APP(" ");
					  APP($assignmentExpressionNoIn::op);
					  APP(" ");
					}

					assignmentExpressionNoIn
   )
	;
	
leftHandSideExpression
	: callExpression
	| newExpression
	;
	
newExpression
	: memberExpression
	| ^( NEW newExpression)
	;
	

propertyReferenceSuffix1
: Identifier { APP((const char*)$Identifier.text->chars);} 
;

indexSuffix1
: expression
;

memberExpression
: primaryExpression
|functionExpression
| ^(DOT memberExpression { APP("."); } propertyReferenceSuffix1 )
| ^(ARRAY_INDEX memberExpression { APP("[ "); } indexSuffix1 { APP(" ] "); })
| ^(NEW { APP("new "); } memberExpression arguments)
| ^(DOT { APP(".");} memberExpression) 
;

memberExpressionSuffix
	: indexSuffix
	| propertyReferenceSuffix 
	;

callExpression
 : ^(CALL memberExpression arguments) 
 | ^(ARRAY_INDEX callExpression {APP("[ "); } indexSuffix1 { APP(" ]"); })
 | ^(DOT callExpression { APP(".");} propertyReferenceSuffix1)
;
	


callExpressionSuffix
	: arguments
	| indexSuffix
	| propertyReferenceSuffix
	;

arguments
	: ^(ARGLIST 
	       {
                 APP(" ( ");  
               }
               (
                 assignmentExpression
	          (
			   {
					  APP(", ");
					}
					  assignmentExpression
					   	
					)*
				
				)*
      )
				
				{
				  APP(" ) ");
				}
	;
	

indexSuffix
	: ^(ARRAY_INDEX expression)
	;	
	
propertyReferenceSuffix
	: ^(DOT Identifier)
//        | ^(DOT
//                dollarExpression
//           )
	;
	
assignmentOperator
	: ASSIGN|MULT_ASSIGN|DIV_ASSIGN | MOD_ASSIGN| ADD_ASSIGN | SUB_ASSIGN|LEFT_SHIFT_ASSIGN|RIGHT_SHIFT_ASSIGN|TRIPLE_SHIFT_ASSIGN|AND_ASSIGN|EXP_ASSIGN|OR_ASSIGN
	;

conditionalExpression
	: logicalORExpression 
	|^(
	    TERNARYOP
					{
					  APP( " ( ");
					}
	    logicalORExpression
					{
					  APP(" )  ? ( ");
					}
	    
				 assignmentExpression 
					{
						  APP(" ) : ( ");
					}
						
						assignmentExpression
						{
						  APP(" ) ");
						}

						)
				
	;

conditionalExpressionNoIn
 : logicalORExpressionNoIn
	|^(
	   TERNARYOP
				{
				  APP(" ( ");
				}
	   logicalORExpressionNoIn 
				{
				  APP(" ) ? ( ");
				}
    
				 assignmentExpressionNoIn 
			  {
					  APP(" ) : ( ");
					}		
					assignmentExpressionNoIn
					{
					  APP(" ) ");
					}
				)
	;


logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression { APP(" && ");} bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression { APP(" || "); } logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn{ APP(" || ");}logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn {APP(" && ");} bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression { APP(" | "); } bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn {  APP(" | ");} bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP e=bitwiseXORExpression { APP(" ^ ");} bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP e=bitwiseXORExpressionNoIn { APP(" ^ ");}bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND e=bitwiseANDExpression { APP(" & ");} equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND e=bitwiseANDExpressionNoIn { APP(" & ");} equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
	| ^(EQUALS e=equalityExpression { APP(" == ");} relationalExpression)
	| ^(NOT_EQUALS e=equalityExpression {APP(" != ");} relationalExpression)
	| ^(IDENT e=equalityExpression { APP(" === ");} relationalExpression)
	| ^(NOT_IDENT e=equalityExpression {APP(" !== ");} relationalExpression)
;

equalityExpressionNoIn
: relationalExpressionNoIn
| ^( EQUALS equalityExpressionNoIn { APP(" == ");} relationalExpressionNoIn)
| ^( NOT_EQUALS equalityExpressionNoIn {  APP(" != ");} relationalExpressionNoIn)
| ^( IDENT equalityExpressionNoIn { APP(" === "); } relationalExpressionNoIn)
| ^( NOT_IDENT equalityExpressionNoIn { APP(" !== ");} relationalExpressionNoIn)

;
	

relationalOps
: LESS_THAN {   $relationalExpression::op = "<" ;}
| GREATER_THAN { $relationalExpression::op = ">" ;}
| LESS_THAN_EQUAL {$relationalExpression::op = "<=" ;} 
| GREATER_THAN_EQUAL { $relationalExpression::op = ">=" ;}
| INSTANCE_OF {$relationalExpression::op = "instanceOf" ;}
| IN {$relationalExpression::op = "in" ;} 
;

relationalExpression
scope
{
  char* op;
}

	: shiftExpression 
	| 
	^(
	   relationalOps 
				e=relationalExpression
				{
				  APP(" ");
				  APP($relationalExpression::op );
				  APP(" ");
				}
				shiftExpression
			) 
	;

relationalOpsNoIn
: LESS_THAN {  $relationalExpressionNoIn::op = "<" ;}
| GREATER_THAN { $relationalExpressionNoIn::op = ">"; }
| LESS_THAN_EQUAL { $relationalExpressionNoIn::op = "<= " ;}
| GREATER_THAN_EQUAL { $relationalExpressionNoIn::op = ">=" ;}
| INSTANCE_OF    { $relationalExpressionNoIn::op = "instanceOf" ;}
;

relationalExpressionNoIn
scope
{
  char* op;
}

	: shiftExpression 
	| 	^(
	     relationalOpsNoIn
						relationalExpressionNoIn
						{
						  APP(" ");
						  APP($relationalExpressionNoIn::op);
						  APP(" ");
						}
						shiftExpression
				)
	   
	;

shiftOps
: LEFT_SHIFT  {$shiftExpression::op = "<<" ;}
| RIGHT_SHIFT { $shiftExpression::op = ">>" ;} 
| TRIPLE_SHIFT { $shiftExpression::op = ">>>"; }
;

shiftExpression
scope
{
 char* op; 
}
	: additiveExpression
	| ^(shiftOps 
	    e=shiftExpression 
					 {
						  APP(" ");
						  APP($shiftExpression::op);
						  APP(" ");
						}
					additiveExpression
					)
 ;	



additiveExpression
	: multiplicativeExpression
	| ^(
	     ADD 
						e1=additiveExpression
						{
						  APP(" + ");
						}
						multiplicativeExpression
					) 
	| ^(
	     SUB 
						e1=additiveExpression 
						 {
							  APP(" - ");
							}
						multiplicativeExpression
					) 
	;


multiplicativeExpression

/*
@init
{
  printf(" ( ");
}
@after
{
  printf(" ) ");
}
*/
	: unaryExpression
	| ^( MULT 
	    multiplicativeExpression 
					{
					  APP(" * ");
					 
					} 
					unaryExpression
					)
	| ^(DIV multiplicativeExpression { APP(" / ");} unaryExpression)
	| ^(MOD multiplicativeExpression { APP(" \% ");} unaryExpression)
	;

unaryOps
: DELETE 
| VOID
| TYPEOF
| PLUSPLUS
| MINUSMINUS
| UNARY_PLUS
| UNARY_MINUS
| COMPLEMENT
| NOT
;


unaryExpression
	: ^(POSTEXPR postfixExpression)
	| ^(
	
	    (
				   DELETE          {  APP("delete");}
       | VOID          {   APP("void");}
       | TYPEOF        {  APP("typeOf ");}
       | PLUSPLUS      {  APP("++");}
       | MINUSMINUS    {  APP("--");}
       | UNARY_PLUS    {  APP("+");}
       | UNARY_MINUS   {  APP("-");}
       | COMPLEMENT    {  APP("~");}
       | NOT           {  APP("!");}
	
					)

	    unaryExpression
				)
	;
	

postfixExpression
	: leftHandSideExpression ('++' { APP("++");})?
	| leftHandSideExpression ('--'{ APP("--"); })?
	;

primaryExpression
	: 'this' {APP("this");}
	| Identifier 
	  { 
            APP((const char*)$Identifier.text->chars);
	  }
        | dollarExpression
	| literal
	| arrayLiteral
	| objectLiteral
	| ^(PAREN { APP("( "); } expression { APP(" )");}) 
	;

dollarExpression
        : ^(DOLLAR_EXPRESSION
            {
                APP("\",");
                APP(" util.dollar_expression(null,");
            }
            Identifier
            {
                APP((const char*)$Identifier.text->chars);
                APP("),\"");
            }
         )
         ;
        

        
// arrayLiteral definition.
arrayLiteral
  : ^(ARRAY_LITERAL {APP("[ ]"); })
  | ^(ARRAY_LITERAL 

	     { APP("[ "); }
       (assignmentExpression)
			 { APP(" ]"); }
      )

  | ^(ARRAY_LITERAL
	       {
                 APP("[ ");
		}
                assignmentExpression
      	
	       (
                 {
                   APP(", ");
                 }
                 assignmentExpression
		)*
                {
                  APP(" ] ");
                }
      )

       	;
       
// objectLiteral definition.
objectLiteral
  :^(OBJ_LITERAL {APP("{ }");} )
  |^(OBJ_LITERAL 
	    { APP("{ "); } 
            
            (propertyNameAndValue)

           {  APP(" }"); }
    )
	|^(OBJ_LITERAL 
	   
				{ APP("{ ");}
				propertyNameAndValue
				( 
				  { 
					  APP(", "); 
					} 
				
				  propertyNameAndValue
				)*

      	{ 
				  APP(" } "); 
				
				}

			)
				
	;
	
propertyNameAndValue
	: ^(NAME_VALUE 
	   propertyName 
			{	APP(" : ");}
				assignmentExpression)
	;

propertyName
	: Identifier {  APP((const char*)$Identifier.text->chars); }
	| StringLiteral
          {
              if (insideWhenPred)
              {
                  String escapedSequence = emerson_escapeSingleQuotes((const char*) $StringLiteral.text->chars);
                  APP((const char*) escapedSequence.c_str());
              }
              else
              {
                  APP((const char*)$StringLiteral.text->chars);  
              }
          }
	| NumericLiteral {APP((const char*)$NumericLiteral.text->chars);}
	;

// primitive literal definition.
literal
	: 'null' {   APP("null");}
	| 'true' {   APP("true"); }
	| 'false'{  APP("false");}
	| StringLiteral
          {
              if (insideWhenPred)
              {
                  String escapedSequence = emerson_escapeSingleQuotes(((const char*) $StringLiteral.text->chars));
                  APP((const char*)(escapedSequence.c_str()));
              }
              else
              {
                  APP((const char*)$StringLiteral.text->chars);  
              }
          }
//	| StringLiteral {APP((const char*)$StringLiteral.text->chars);}
	| NumericLiteral {APP((const char*)$NumericLiteral.text->chars);}
	;
	
