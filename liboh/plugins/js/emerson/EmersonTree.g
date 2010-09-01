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
  #include <stdlib.h>;
		#include <string.h>;
}

	program
	:^(PROG sourceElements)   
	                       
	;
	
 sourceElements
		:(sourceElement{printf("\n");})+  // omitting the LT 
	;
	
	sourceElement
	: functionDeclaration
	| statement{printf(";");}
	;
	
// functions
functionDeclaration
	: ^( FUNC_DECL
	         {
										  printf("function ");
										}
	         Identifier
	         {
										  printf($Identifier.text->chars);
	           printf("( ");
										} 
					formalParameterList 
					     {
										  printf(" )");
												printf("\n{\n");
										}
					functionBody
					     {
										  printf("\n}\n");
										}
					 					
					)
	
	;
	
functionExpression
	: ^( FUNC_EXPR 
	     {
        printf("function ");
						}
						(			
						 Identifier
						 {
							  printf($Identifier.text->chars);
							}
						)?  
						{
						
									printf("( ");
						}
						formalParameterList 
						  {
								  printf("  )");
										printf("\n{\n");
								}
						functionBody
						  {
								  printf("\n}\n");
								}
						)
	; 
	
formalParameterList
	: ^( FUNC_PARAMS 
	     (id1=Identifier
									{
						     printf("\%s ", $id1.text->chars);
									}
							)?		
						(
						 id2=Identifier
							{
							  
						   printf(", \%s ", $id2.text->chars);
							}
						
						)*
						
					)
;

functionBody
	: sourceElements
	| EMPTY_FUNC_BODY 
	;

// statements
statement
	: statementBlock
	| variableStatement
	//| emptyStatement
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
	| tryStatement
	| msgSendStatement
	| msgRecvStatement
	;
	
statementBlock
	: {printf(" {\n "); } statementList { printf(" }\n"); }
	;
	
statementList
	:  ^(
	      SLIST 
							(statement
							  {
			        printf("; \n");					  
									}
							)+
							
	
	    )
	;
	
variableStatement
	:  ^( 
	      VARLIST
							  {
									  printf("var ");
									}
	      variableDeclarationList
					)
	;
	
variableDeclarationList
	: variableDeclaration 
	  (
      {
						  printf(", ");
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
							  printf($Identifier.text->chars);
							}
					
					(
					  {
							  printf(" = ");
							}
					initialiser
					)?
					
					)
	;
	
variableDeclarationNoIn
	: 
	^(
	    VAR
	    Identifier 
					  {
							  printf($Identifier.text->chars);
							}
					
					(
					  {
							  printf(" = ");
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
					  printf(" if ");
							printf(" ( ");
					}
	   expression 
				 {
					  printf(" ) ");
					  //printf(" { \n");
					}
     
				statement 
				 {
					  //printf(" } \n");
					}
     
				(
				 {
					  printf("else");
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
        printf(" do ");  						  
						}    
	    statement
					{
					  printf("while ( " );      
					}

					expression 

					{
					  printf(" ) ");  
					}
					
					)
	;
	
whileStatement
	: ^(
	   WHILE
				  {
						  printf(" while ( ");
						}
	
	   expression 

			 {
			   printf(" ) "); 
			 }
			
			 statement
			)
	;
	
forStatement
	: ^(
	     FOR 
	     {
						  printf(" for ( ");
						}
	     (^(FORINIT forStatementInitialiserPart))?
						{
						  printf(" ; ");
						}
						(^(FORCOND expression))? 
						{
						  printf(" ; ");
						}
						(^(FORITER expression))?  
						
      {
						  printf(" ) ");
						}
						statement
					
					)
	;
	
forStatementInitialiserPart
	: expressionNoIn
	| ^(VARLIST variableDeclarationListNoIn)
	;
	
forInStatement
	: ^(FORIN forInStatementInitialiserPart expression statement)
	;
	
forInStatementInitialiserPart
	: leftHandSideExpression
	| ^(VAR variableDeclarationNoIn)
	;

continueStatement
	: ^(
	    CONTINUE 
	    {
					  printf("continue ");
					} 
	   
				 (
					 Identifier
					 {
						  printf($Identifier.text->chars);
						}
					)?

				
				)
			

	;

breakStatement
	: ^(
	    BREAK
					{
					  printf("break ");
					}
	     (
						Identifier
						{
						  printf($Identifier.text->chars);
						}
						)?
						
					)
	;

returnStatement
	: ^(
	    RETURN 
	     {
						  printf("return ");
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
						  printf(" : \n");
						}
					statement
					
					)
	;
	
switchStatement
	: ^(
	    SWITCH 
	     {
						  printf(" switch ( ");
						}

	    expression 
			   {
						  printf(" ) \n");
								printf("{ \n");
						}
      
						
			  caseBlock

					 {
						  printf("} \n");
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

tryStatement
	: ^(TRY 
	    
	    statementBlock 
					
					finallyClause?) 
	;
       

msgSendStatement
 : ^(
	     MESSAGE_SEND 
	     leftHandSideExpression 
				  {

						  printf(".sendMessage( ");
						}
				leftHandSideExpression 
				  				
				(
				
				 {
					  printf(", ");
					}
				memberExpression
			 			
						)?
						
				)

				{
				  printf(" )" );
				}
;

msgRecvStatement
 : ^(
	    MESSAGE_RECV
	    {
					  printf("system.registerHandler( callback = ");
					}

					memberExpression
     {
					  printf(", pattern = ");
					} 
     leftHandSideExpression

					(
					 {
						  printf(", sender = ");
						}
						memberExpression
					)?
				)
    {
				  printf(") ");
				}

;

catchClause
	: ^(CATCH 
	    {
					  printf(" catch ( ");
					}
	    Identifier 
					{
					  printf("\%s ) ", $Identifier.text->chars);
					}
					
					statementBlock)
	   
	;
	
finallyClause
	: ^( FINALLY 
	   {
				  printf(" finally ");
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
							  printf(" \%s ", $assignmentExpression::op);
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
					  printf(" \%s ", $assignmentExpression::op);
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
	
memberExpression
 : primaryExpression
	| functionExpression
	| ^(
	     NEW 
						  {
								  printf("new ");
								}
						memberExpression 
						
						arguments
					)

	| ^(DOT 
	      memberExpression
							  {
									  printf(".");
									}
							memberExpression
				)
	| ^(
	    ARRAY_INDEX 
					memberExpression 
					    {
           printf("[ ");
									}
					memberExpression
					    {
									  printf(" ]");
									}
					
					)

	;


memberExpressionSuffix
	: indexSuffix
	| propertyReferenceSuffix 
	;

callExpression
 : ^(CALL memberExpression arguments) 
	//|^(callExpressionSuffix callExpression callExpression)
	;
	


callExpressionSuffix
	: arguments
	| indexSuffix
	| propertyReferenceSuffix
	;

arguments
	: ^(ARGLIST 
	       {
								  printf(" ( ");  
								}
	    (
					  
					  assignmentExpression
					   	
					)*
				
				)
				{
				  printf(" ) ");
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
					  printf( " ( ");
					}
	    logicalORExpression
					{
					  printf(" )  ? ( ");
					}
	    
				 assignmentExpression 
					{
						  printf(" ) : ( ");
					}
						
						assignmentExpression
						{
						  printf(" ) ");
						}

						)
				
	;

conditionalExpressionNoIn
 : logicalORExpressionNoIn
	|^(
	   TERNARYOP
				{
				  printf(" ( ");
				}
	   logicalORExpressionNoIn 
				{
				  printf(" ) ? ( ");
				}
    
				 assignmentExpressionNoIn 
			  {
					  printf(" ) : ( ");
					}		
					assignmentExpressionNoIn
					{
					  printf(" ) ");
					}
				)
	;


logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression { printf(" && "); } bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression { printf(" || "); } logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn{ printf(" || "); }logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn {printf(" && "); } bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression { printf(" | "); } bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn { printf(" | "); } bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP e=bitwiseXORExpression { printf( " ^ ");} bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP e=bitwiseXORExpressionNoIn { printf( " ^ ") ;}bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND e=bitwiseANDExpression { printf( " & " );} equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND e=bitwiseANDExpressionNoIn { printf(" & "); } equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
	| ^(EQUALS e=equalityExpression {printf(" == ") ;} relationalExpression)
	| ^(NOT_EQUALS e=equalityExpression {printf(" != ");} relationalExpression)
	| ^(IDENT e=equalityExpression { printf(" === ");} relationalExpression)
	| ^(NOT_IDENT e=equalityExpression {printf(" !== ");} relationalExpression)
;

equalityExpressionNoIn
: relationalExpressionNoIn
| ^( EQUALS equalityExpressionNoIn {printf(" == ");} relationalExpressionNoIn)
| ^( NOT_EQUALS equalityExpressionNoIn { printf(" != ") ;} relationalExpressionNoIn)
| ^( IDENT equalityExpressionNoIn { printf(" === "); } relationalExpressionNoIn)
| ^( NOT_IDENT equalityExpressionNoIn { printf(" !== "); } relationalExpressionNoIn)

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
				  printf( " \%s ", $relationalExpression::op );
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
						  printf(" \%s ", $relationalExpressionNoIn::op);
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
						  printf(" \%s ", $shiftExpression::op);
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
						  printf(" + ");
						}
						multiplicativeExpression
					) 
	| ^(
	     SUB 
						e1=additiveExpression 
						 {
							  printf(" - ");
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
					{printf(" * ");} 
					unaryExpression
					)
	| ^(DIV multiplicativeExpression {printf(" / ");} unaryExpression)
	| ^(MOD multiplicativeExpression {printf(" \% ");} unaryExpression)
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
				   DELETE          { printf("delete"); }
       | VOID          { printf("void") ;  }
       | TYPEOF        { printf("typeOf"); }
       | PLUSPLUS      { printf("++"); }
       | MINUSMINUS    { printf("--"); }
       | UNARY_PLUS    { printf("+"); }
       | UNARY_MINUS   { printf("-"); }
       | COMPLEMENT    { printf("~"); }
       | NOT           { printf("!"); }
	
					)

	    unaryExpression
				)
	;
	

postfixExpression
	: leftHandSideExpression '++'?
	| leftHandSideExpression '--'?
	;

primaryExpression
	: 'this' {printf("this");}
	| Identifier {printf($Identifier.text->chars);}
	| literal
	| arrayLiteral
	| objectLiteral
	| expression
	;
	
// arrayLiteral definition.
arrayLiteral
	: ^(ARRAY_LITERAL
	       {
								  printf("[ ");
								}
	       head=assignmentExpression? 
	
	     tail=assignmentExpression*)

        {
								  printf(" ] ");

								}
	;
       
// objectLiteral definition.
objectLiteral
	:^(OBJ_LITERAL 
	   
				{ printf("{ "); }
	   (head=propertyNameAndValue)? 
				(tail=propertyNameAndValue)*
				{ printf(" } "); }
				
				)
	;
	
propertyNameAndValue
	: ^(NAME_VALUE 
	   propertyName 
			{	printf(" : "); }
				assignmentExpression)
	;

propertyName
	: Identifier { printf($Identifier.text->chars); }
	| StringLiteral { printf($StringLiteral.text->chars);  }
	| NumericLiteral {printf($NumericLiteral.text->chars); }
	;

// primitive literal definition.
literal
	: 'null' { printf("null");  }
	| 'true' { printf("true");  }
	| 'false'{ printf("false"); }
	| StringLiteral {printf($StringLiteral.text->chars); }
	| NumericLiteral {printf($NumericLiteral.text->chars); }
	;
	
