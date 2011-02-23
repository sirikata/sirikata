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
//	memoize=true;
	rewrite=true;
	ASTLabelType=pANTLR3_BASE_TREE;
//	ASTLabelType=pANTRL3_COMMON_TREE;
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


    #ifndef __SIRIKATA_INSIDE_WHEN_PRED__
    #define __SIRIKATA_INSIDE_WHEN_PRED__
    static bool insideWhenPred = false;
    #endif
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
    : functionDeclaration
    | statement{ APPLEX(";"); }
    ;
	
// functions
functionDeclaration
    : ^( FUNC_DECL
            {
                APPLEX("function ");
            }
            Identifier
	        {
                APPLEX((const char*)$Identifier.text->chars);
                APPLEX("( ");
            } 
            formalParameterList 
            {
                APPLEX(" )");
                APPLEX("\n{\n");
            }
            functionBody
            {
                APPLEX("\n}");
            }
            
        );

functionExpression
	: ^( FUNC_EXPR 
            {
                APPLEX("function ");
            }
            (			
                Identifier
                {
                    APPLEX((const char*)$Identifier.text->chars);
                }
            )?  
            {
                APPLEX("( ");
            }
            formalParameterList 
            {
                APPLEX("  )");
                APPLEX("\n{\n");
            }
            functionBody
            {
                APPLEX("\n}");
            }
        ); 
	
formalParameterList
	: ^( FUNC_PARAMS 
            (id1=Identifier
                {
                    APPLEX((const char*)$id1.text->chars);
                    APPLEX(" ");
                }
            )?		
            (
                id2=Identifier
                {
                    APPLEX(", ");
                    APPLEX((const char*)$id2.text->chars);
                    APPLEX(" ");
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
	: {APPLEX(" {\n "); } statementList {  
            APPLEX(" }\n");
        }
	;
	
statementList
	:  ^(
            SLIST 
            (statement
                {
			        APPLEX("; \n");					  
                }
            )+
	    );
	
variableStatement
	:  ^( 
            VARLIST
            {
                APPLEX("var ");
            }
            variableDeclarationList
        );
	
variableDeclarationList
	: variableDeclaration 
        (
            {
                APPLEX(", ");
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
                APPLEX((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APPLEX(" = ");
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
                APPLEX("var ");
			}
            Identifier 
            {
                APPLEX((const char*)$Identifier.text->chars);
            }
            
            (
                {
                    APPLEX(" = ");
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
                
                APPLEX(" if ");
                APPLEX(" ( ");
            }
            expression 
            {
                APPLEX(" ) ");
            }
            statement 
            {
                APPLEX(" \n");
            }
            (
                {
                    APPLEX(" else ");
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
                APPLEX(" do ");  						  
            }    
	        statement
            {
                APPLEX("while ( " );      
            }
            expression 
            {
                APPLEX(" ) ");  
            }
        )
	;
	
whileStatement
	: ^(
            WHILE
            {
                APPLEX(" while ( ");
            }
            expression 
            {
                APPLEX(" ) "); 
            }
            statement
        )
	;
	
forStatement
	: ^(
            FOR 
            {
                APPLEX(" for ( ");
            }
            (^(FORINIT forStatementInitialiserPart))?
            {
                APPLEX(" ; ");
            }
            (^(FORCOND expression))? 
            {
                APPLEX(" ; ");
            }
            (^(FORITER expression))?  
            {
                APPLEX(" ) ");
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
            APPLEX(" for ( ");
        }

        forInStatementInitialiserPart 
        {
            APPLEX(" in ");
        }
        expression 
        {
            APPLEX(" ) ");
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
            APPLEX("continue ");
        } 
        (
            Identifier
            {
                APPLEX((const char*)$Identifier.text->chars);
            }
        )?
      )
	;

breakStatement
    : ^(
        BREAK
        {
            APPLEX("break ");
        }
        (
            Identifier
            {
                APPLEX((const char*)$Identifier.text->chars);
            }
        )?
        )
	;

returnStatement
    : ^(
        RETURN 
        {
            APPLEX("return ");
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
            APPLEX((const char*)$Identifier.text->chars);
            APPLEX(" : \n");
        }
        statement
        )
	;
	
switchStatement
    : ^(
        SWITCH 
        {
            APPLEX(" switch ( ");
        }
        expression 
        {
            APPLEX(" ) \n");
            APPLEX("{ \n");
        }
        caseBlock
        {
            APPLEX("} \n");
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
            APPLEX(" util.create_when( ");
            insideWhenPred = true;
            APPLEX(" [ util.create_quoted('");
        }
        whenPred
        {
            //FIXME: potential problem if last statement in array is
            //dollar syntax.
            APPLEX("')],\n");

            insideWhenPred = false;
            //open function for callback
            APPLEX("function(){ ");
              
        }
        functionBody
        {
            //close function for callback
            APPLEX(" }");
            //close create_when
            APPLEX(");");
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
            APPLEX(",");
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
			   $msgSendStatement::prev_program_string = program_string_lex;
       /* length of the program string */
			   $msgSendStatement::prev_program_len = $msgSendStatement::prev_program_string->len;
         pANTLR3_STRING_FACTORY factory = antlr3StringFactoryNew();
				 $msgSendStatement::init_program_string = factory->newRaw(factory);
				 $msgSendStatement::init_program_string->setS($msgSendStatement::init_program_string, program_string_lex);
       }

	     leftHandSideExpression 
				  {
					   unsigned int prev_program_len = $msgSendStatement::prev_program_len;

			       unsigned int  new_program_len = program_string_lex->len;
             $msgSendStatement::firstExprString = (char*)(malloc(new_program_len - prev_program_len + 1) );
						 memset($msgSendStatement::firstExprString, 0, (new_program_len - prev_program_len + 1));
						 memcpy($msgSendStatement::firstExprString, (char*)(program_string_lex->chars) + prev_program_len, (new_program_len - prev_program_len) );
						 
             $msgSendStatement::prev_program_len = new_program_len; 
						  //APPLEX(".sendMessage( ");
					}
				leftHandSideExpression 
				    {
						  unsigned int prev_program_len = $msgSendStatement::prev_program_len;
						  unsigned int new_program_len = program_string_lex->len;
              $msgSendStatement::secondExprString = (char*)(malloc(new_program_len - prev_program_len + 1) );
							memset($msgSendStatement::secondExprString, 0, new_program_len - prev_program_len + 1);
							memcpy($msgSendStatement::secondExprString, (char*)(program_string_lex->chars) + prev_program_len, (new_program_len - prev_program_len));

              pANTLR3_STRING init_program_string = $msgSendStatement::init_program_string;
              init_program_string->append(init_program_string, $msgSendStatement::secondExprString);
              init_program_string->append(init_program_string, ".sendMessage( ");
              init_program_string->append(init_program_string, $msgSendStatement::firstExprString);

							program_string_lex->setS(program_string_lex, init_program_string); 

						}      				
				(
				
				 {
					  APPLEX(", ");
					}
				memberExpression
			 		{
					  
					}	
				)?
						{
						  APPLEX(" ) ");
				    }

				)

				;

msgRecvStatement
 : ^(
	    MESSAGE_RECV
	    {
					  APPLEX("system.registerHandler( ");
			}

					memberExpression
     {
            APPLEX(", null");
					  APPLEX(", ");
					} 
     leftHandSideExpression

			)
    {
				  APPLEX(", null) ");  // No sender case
				}
   
 |^(
	    MESSAGE_RECV
	    {
					  APPLEX("system.registerHandler( ");
			}

					memberExpression
     {
            APPLEX(", null");
					  APPLEX(", ");
					} 
     leftHandSideExpression

					
					 {
						  APPLEX(", ");
						}

						memberExpression
					
				)
    {
				  APPLEX(") "); // Case with sender
				}



;

catchClause
	: ^(CATCH 
	    {
					  APPLEX(" catch ( ");
					}
	    Identifier 
					{
					  APPLEX((const char*)$Identifier.text->chars);
					  APPLEX(" ) ");

					}
					
					statementBlock)
	   
	;
	
finallyClause
	: ^( FINALLY 
	   {
				  APPLEX(" finally ");

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
							  APPLEX(" ");
							  APPLEX($assignmentExpression::op);
							  APPLEX(" ");
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
					  APPLEX(" ");
					  APPLEX($assignmentExpressionNoIn::op);
					  APPLEX(" ");
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
: Identifier { APPLEX((const char*)$Identifier.text->chars);} 
;

indexSuffix1
: expression
;

memberExpression
: primaryExpression
|functionExpression
| ^(DOT memberExpression { APPLEX("."); } propertyReferenceSuffix1 )
| ^(ARRAY_INDEX memberExpression { APPLEX("[ "); } indexSuffix1 { APPLEX(" ] "); })
| ^(NEW { APPLEX("new "); } memberExpression arguments)
| ^(DOT { APPLEX(".");} memberExpression) 
;

memberExpressionSuffix
	: indexSuffix
	| propertyReferenceSuffix 
	;

callExpression
 : ^(CALL memberExpression arguments) 
 | ^(ARRAY_INDEX callExpression {APPLEX("[ "); } indexSuffix1 { APPLEX(" ]"); })
 | ^(DOT callExpression { APPLEX(".");} propertyReferenceSuffix1)
;
	


callExpressionSuffix
	: arguments
	| indexSuffix
	| propertyReferenceSuffix
	;

arguments
	: ^(ARGLIST 
	       {
                 APPLEX(" ( ");  
               }
               (
                 assignmentExpression
	          (
			   {
					  APPLEX(", ");
					}
					  assignmentExpression
					   	
					)*
				
				)*
      )
				
				{
				  APPLEX(" ) ");
				}
	;
	

indexSuffix
	: ^(ARRAY_INDEX expression)
	;	
	
propertyReferenceSuffix
	: ^(DOT Identifier)
	;
	
assignmentOperator
	: ASSIGN|MULT_ASSIGN|DIV_ASSIGN | MOD_ASSIGN| ADD_ASSIGN | SUB_ASSIGN|LEFT_SHIFT_ASSIGN|RIGHT_SHIFT_ASSIGN|TRIPLE_SHIFT_ASSIGN|AND_ASSIGN|EXP_ASSIGN|OR_ASSIGN
	;

conditionalExpression
	: logicalORExpression 
	|^(
	    TERNARYOP
					{
					  APPLEX( " ( ");
					}
	    logicalORExpression
					{
					  APPLEX(" )  ? ( ");
					}
	    
				 assignmentExpression 
					{
						  APPLEX(" ) : ( ");
					}
						
						assignmentExpression
						{
						  APPLEX(" ) ");
						}

						)
				
	;

conditionalExpressionNoIn
 : logicalORExpressionNoIn
	|^(
	   TERNARYOP
				{
				  APPLEX(" ( ");
				}
	   logicalORExpressionNoIn 
				{
				  APPLEX(" ) ? ( ");
				}
    
				 assignmentExpressionNoIn 
			  {
					  APPLEX(" ) : ( ");
					}		
					assignmentExpressionNoIn
					{
					  APPLEX(" ) ");
					}
				)
	;


logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression { APPLEX(" && ");} bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression { APPLEX(" || "); } logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn{ APPLEX(" || ");}logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn {APPLEX(" && ");} bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression { APPLEX(" | "); } bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn {  APPLEX(" | ");} bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP e=bitwiseXORExpression { APPLEX(" ^ ");} bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP e=bitwiseXORExpressionNoIn { APPLEX(" ^ ");}bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND e=bitwiseANDExpression { APPLEX(" & ");} equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND e=bitwiseANDExpressionNoIn { APPLEX(" & ");} equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
	| ^(EQUALS e=equalityExpression { APPLEX(" == ");} relationalExpression)
	| ^(NOT_EQUALS e=equalityExpression {APPLEX(" != ");} relationalExpression)
	| ^(IDENT e=equalityExpression { APPLEX(" === ");} relationalExpression)
	| ^(NOT_IDENT e=equalityExpression {APPLEX(" !== ");} relationalExpression)
;

equalityExpressionNoIn
: relationalExpressionNoIn
| ^( EQUALS equalityExpressionNoIn { APPLEX(" == ");} relationalExpressionNoIn)
| ^( NOT_EQUALS equalityExpressionNoIn {  APPLEX(" != ");} relationalExpressionNoIn)
| ^( IDENT equalityExpressionNoIn { APPLEX(" === "); } relationalExpressionNoIn)
| ^( NOT_IDENT equalityExpressionNoIn { APPLEX(" !== ");} relationalExpressionNoIn)

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
				  APPLEX(" ");
				  APPLEX($relationalExpression::op );
				  APPLEX(" ");
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
						  APPLEX(" ");
						  APPLEX($relationalExpressionNoIn::op);
						  APPLEX(" ");
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
						  APPLEX(" ");
						  APPLEX($shiftExpression::op);
						  APPLEX(" ");
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
						  APPLEX(" + ");
						}
						multiplicativeExpression
					) 
	| ^(
	     SUB 
						e1=additiveExpression 
						 {
							  APPLEX(" - ");
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
					  APPLEX(" * ");
					 
					} 
					unaryExpression
					)
	| ^(DIV multiplicativeExpression { APPLEX(" / ");} unaryExpression)
	| ^(MOD multiplicativeExpression { APPLEX(" \% ");} unaryExpression)
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
				   DELETE          {  APPLEX("delete");}
       | VOID          {   APPLEX("void");}
       | TYPEOF        {  APPLEX("typeOf ");}
       | PLUSPLUS      {  APPLEX("++");}
       | MINUSMINUS    {  APPLEX("--");}
       | UNARY_PLUS    {  APPLEX("+");}
       | UNARY_MINUS   {  APPLEX("-");}
       | COMPLEMENT    {  APPLEX("~");}
       | NOT           {  APPLEX("!");}
	
					)

	    unaryExpression
				)
	;
	

postfixExpression
	: leftHandSideExpression ('++' { APPLEX("++");})?
	| leftHandSideExpression ('--'{ APPLEX("--"); })?
	;

primaryExpression
	: 'this' {APPLEX("this");}
	| Identifier 
	  { 
            APPLEX((const char*)$Identifier.text->chars);
	  }
        | dollarExpression
	| literal
	| arrayLiteral
	| objectLiteral
	| ^(PAREN { APPLEX("( "); } expression { APPLEX(" )");}) 
	;

dollarExpression
        : ^(DOLLAR_EXPRESSION
            {
                if (insideWhenPred)
                    APPLEX("'),");

            }
            Identifier
            {
                APPLEX((const char*)$Identifier.text->chars);

                if (insideWhenPred)
                   APPLEX(",util.create_quoted('");

            }
         )
         ;
        

        
// arrayLiteral definition.
arrayLiteral
  : ^(ARRAY_LITERAL {APPLEX("[ ]"); })
  | ^(ARRAY_LITERAL 

	     { APPLEX("[ "); }
       (assignmentExpression)
			 { APPLEX(" ]"); }
      )

  | ^(ARRAY_LITERAL
	       {
                 APPLEX("[ ");
		}
                assignmentExpression
      	
	       (
                 {
                   APPLEX(", ");
                 }
                 assignmentExpression
		)*
                {
                  APPLEX(" ] ");
                }
      )

       	;
       
// objectLiteral definition.
objectLiteral
  :^(OBJ_LITERAL {APPLEX("{ }");} )
  |^(OBJ_LITERAL 
	    { APPLEX("{ "); } 
            
            (propertyNameAndValue)

           {  APPLEX(" }"); }
    )
	|^(OBJ_LITERAL 
	   
				{ APPLEX("{ ");}
				propertyNameAndValue
				( 
				  { 
					  APPLEX(", "); 
					} 
				
				  propertyNameAndValue
				)*

      	{ 
				  APPLEX(" } "); 
				
				}

			)
				
	;
	
propertyNameAndValue
	: ^(NAME_VALUE 
	   propertyName 
			{	APPLEX(" : ");}
				assignmentExpression)
	;

propertyName
	: Identifier {  APPLEX((const char*)$Identifier.text->chars); }
	| StringLiteral
          {
              if (insideWhenPred)
              {
                  std::string escapedSequence = emerson_escapeSingleQuotes((const char*) $StringLiteral.text->chars);
                  APPLEX((const char*) escapedSequence.c_str());
              }
              else
              {
                  APPLEX((const char*)$StringLiteral.text->chars);  
              }
          }
	| NumericLiteral {APPLEX((const char*)$NumericLiteral.text->chars);}
	;

// primitive literal definition.
literal
	: 'null' {   APPLEX("null");}
	| 'true' {   APPLEX("true"); }
	| 'false'{  APPLEX("false");}
	| StringLiteral
          {
              if (insideWhenPred)
              {
                  std::string escapedSequence = emerson_escapeSingleQuotes(((const char*) $StringLiteral.text->chars));
                  APPLEX((const char*)(escapedSequence.c_str()));
              }
              else
              {
                  APPLEX((const char*)$StringLiteral.text->chars);  
              }
          }
//	| StringLiteral {APPLEX((const char*)$StringLiteral.text->chars);}
	| NumericLiteral {APPLEX((const char*)$NumericLiteral.text->chars);}
	;
	
