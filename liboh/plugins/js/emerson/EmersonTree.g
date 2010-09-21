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
		#include <antlr3.h>;
 
	 #define APP(s) program_string->append(program_string, s);

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
		:(sourceElement{printf("\n"); APP("\n"); })+  // omitting the LT 
	;
	
	sourceElement
	: functionDeclaration
	| statement{printf(";");  APP(";"); }
	;
	
// functions
functionDeclaration
	: ^( FUNC_DECL
	         {
										  printf("function ");
												APP("function ");
										}
	         Identifier
	         {
										  printf($Identifier.text->chars);
	           printf("( ");

												APP($Identifier.text->chars);
												APP("( ");
										} 
					formalParameterList 
					     {
										  printf(" )");
												printf("\n{\n");
            
												APP(" )");
												APP("\n{\n");

										}
					functionBody
					     {
            
										  printf("\n}");
										  APP("\n}");
            

										}
					 					
					)
	
	;
	
functionExpression
	: ^( FUNC_EXPR 
	     {
        printf("function ");
        APP("function ");

						}
						(			
						 Identifier
						 {
							  printf($Identifier.text->chars);
							  APP($Identifier.text->chars);
							}
						)?  
						{
						
									printf("( ");
									APP("( ");
						}
						formalParameterList 
						  {
								  printf("  )");
										printf("\n{\n");
          
										APP("  )");
										APP("\n{\n");

								}
						functionBody
						  {
								  printf("\n}");
								  APP("\n}");
								}
						)
	; 
	
formalParameterList
	: ^( FUNC_PARAMS 
	     (id1=Identifier
									{
						     printf("\%s ", $id1.text->chars);
						     APP($id1.text->chars);
											APP(" ");

									}
							)?		
						(
						 id2=Identifier
							{
						   printf(", \%s ", $id2.text->chars);
									APP(", ");
						   APP($id2.text->chars);
									APP(" ");
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
	: {printf(" {\n "); APP(" {\n "); } statementList { printf(" }\n"); 
	   APP(" }\n");
	   }
	;
	
statementList
	:  ^(
	      SLIST 
							(statement
							  {
			        printf("; \n");					  
			        APP("; \n");					  

									}
							)+
							
	
	    )
	;
	
variableStatement
	:  ^( 
	      VARLIST
							  {
									  printf("var ");
									  APP("var ");
									}
	      variableDeclarationList
					)
	;
	
variableDeclarationList
	: variableDeclaration 
	  (
      {
						  printf(", ");
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
							  printf($Identifier.text->chars);
							  APP($Identifier.text->chars);
							}
					
					(
					  {
							  printf(" = ");
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
	    Identifier 
					  {
							  printf($Identifier.text->chars);
							  APP($Identifier.text->chars);
							}
					
					(
					  {
							  printf(" = ");
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
					  printf(" if ");
							printf(" ( ");

							APP(" if ");
							APP(" ( ");


					}
	   expression 
				 {
					  printf(" ) ");
					  APP(" ) ");
							
					  //printf(" { \n");
					}
     
				statement 
				 {
					  //printf(" } \n");
					}
     
				(
				 {
					  printf("else");
					  APP("else");
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
        APP(" do ");  						  
						}    
	    statement
					{
					  printf("while ( " );      
					  APP("while ( " );      
					}

					expression 

					{
					  printf(" ) ");  
					  APP(" ) ");  
					}
					
					)
	;
	
whileStatement
	: ^(
	   WHILE
				  {
						  printf(" while ( ");
						  APP(" while ( ");
						}
	
	   expression 

			 {
			   printf(" ) "); 
			   APP(" ) "); 
			 }
			
			 statement
			)
	;
	
forStatement
	: ^(
	     FOR 
	     {
						  printf(" for ( ");
						  APP(" for ( ");
						}
	     (^(FORINIT forStatementInitialiserPart))?
						{
						  printf(" ; ");
						  APP(" ; ");
						}
						(^(FORCOND expression))? 
						{
						  printf(" ; ");
						  APP(" ; ");
						}
						(^(FORITER expression))?  
						
      {
						  printf(" ) ");
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
					  APP("continue ");
					} 
	   
				 (
					 Identifier
					 {
						  printf($Identifier.text->chars);
						  APP($Identifier.text->chars);
						}
					)?

				
				)
			

	;

breakStatement
	: ^(
	    BREAK
					{
					  printf("break ");
					  APP("break ");
					}
	     (
						Identifier
						{
						  printf($Identifier.text->chars);
						  APP($Identifier.text->chars);
						}
						)?
						
					)
	;

returnStatement
	: ^(
	    RETURN 
	     {
						  printf("return ");
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
						  
						  printf("\%s : \n", $Identifier.text->chars);
						  APP($Identifier.text->chars);
						  APP(" : \n");
						}
					statement
					
					)
	;
	
switchStatement
	: ^(
	    SWITCH 
	     {
						  printf(" switch ( ");
						  APP(" switch ( ");
								
						}

	    expression 
			   {
						  printf(" ) \n");
								printf("{ \n");

								APP(" ) \n");
								APP("{ \n");

						}
      
						
			  caseBlock

					 {
						  printf("} \n");
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
						  APP(".sendMessage( ");
						}
				leftHandSideExpression 
				  				
				(
				
				 {
					  printf(", ");
					  APP(", ");
					}
				memberExpression
			 			
						)?
						
				)

				{
				  printf(" )" );
				  APP(" )" );
				}
;

msgRecvStatement
 : ^(
	    MESSAGE_RECV
	    {
					  printf("system.registerHandler( callback = ");
					  APP("system.registerHandler( callback = ");
					}

					memberExpression
     {
					  printf(", null,"); // right now hardcoding the value to null
					  printf(", pattern = ");
					  APP(", pattern = ");
					} 
     leftHandSideExpression

					(
					 {
						  printf(", sender = ");
						  APP(", sender = ");
						}
						memberExpression
					)?
				)
    {
				  printf(") ");
				  APP(") ");
				}

;

catchClause
	: ^(CATCH 
	    {
					  printf(" catch ( ");
					  APP(" catch ( ");
					}
	    Identifier 
					{
					  printf("\%s ) ", $Identifier.text->chars);
					  APP($Identifier.text->chars);
					  APP(" ) ");

					}
					
					statementBlock)
	   
	;
	
finallyClause
	: ^( FINALLY 
	   {
				  printf(" finally ");
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
							  printf(" \%s ", $assignmentExpression::op);
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
					  printf(" \%s ", $assignmentExpressionNoIn::op);
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
	
memberExpression
 : primaryExpression
	| functionExpression
	| ^(
	     NEW 
						  {
								  printf("new ");
								  APP("new ");
								}
						memberExpression 
						
						arguments
					)

	| ^(DOT 
	      memberExpression
							  {
									  printf(".");
									  APP(".");
									}
							memberExpression
				)
	| ^(
	    ARRAY_INDEX 
					memberExpression 
					    {
           printf("[ ");
           APP("[ ");
									}
					memberExpression
					    {
									  printf(" ]");
									  APP(" ]");
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
								  APP(" ( ");  
								}
	    (
					  
					  assignmentExpression
					   	
					)*
				
				)
				{
				  printf(" ) ");
				  APP(" ) ");
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
					  APP( " ( ");
					}
	    logicalORExpression
					{
					  printf(" )  ? ( ");
					  APP(" )  ? ( ");
					}
	    
				 assignmentExpression 
					{
						  printf(" ) : ( ");
						  APP(" ) : ( ");
					}
						
						assignmentExpression
						{
						  printf(" ) ");
						  APP(" ) ");
						}

						)
				
	;

conditionalExpressionNoIn
 : logicalORExpressionNoIn
	|^(
	   TERNARYOP
				{
				  printf(" ( ");
				  APP(" ( ");
				}
	   logicalORExpressionNoIn 
				{
				  printf(" ) ? ( ");
				  APP(" ) ? ( ");
				}
    
				 assignmentExpressionNoIn 
			  {
					  printf(" ) : ( ");
					  APP(" ) : ( ");
					}		
					assignmentExpressionNoIn
					{
					  printf(" ) ");
					  APP(" ) ");
					}
				)
	;


logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression { printf(" && "); APP(" && ");} bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression { printf(" || "); APP(" || "); } logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn{ printf(" || "); APP(" || ");}logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn {printf(" && "); APP(" && ");} bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression { printf(" | "); APP(" | "); } bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn { printf(" | "); APP(" | ");} bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP e=bitwiseXORExpression { printf( " ^ "); APP(" ^ ");} bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP e=bitwiseXORExpressionNoIn { printf( " ^ ") ; APP(" ^ ");}bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND e=bitwiseANDExpression { printf( " & " ); APP(" & ");} equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND e=bitwiseANDExpressionNoIn { printf(" & "); APP(" & ");} equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
	| ^(EQUALS e=equalityExpression {printf(" == ") ; APP(" == ");} relationalExpression)
	| ^(NOT_EQUALS e=equalityExpression {printf(" != "); APP(" == ");} relationalExpression)
	| ^(IDENT e=equalityExpression { printf(" === "); APP(" === ");} relationalExpression)
	| ^(NOT_IDENT e=equalityExpression {printf(" !== "); APP(" === ");} relationalExpression)
;

equalityExpressionNoIn
: relationalExpressionNoIn
| ^( EQUALS equalityExpressionNoIn {printf(" == "); APP(" == ");} relationalExpressionNoIn)
| ^( NOT_EQUALS equalityExpressionNoIn { printf(" != ") ; APP(" != ");} relationalExpressionNoIn)
| ^( IDENT equalityExpressionNoIn { printf(" === "); APP(" === "); } relationalExpressionNoIn)
| ^( NOT_IDENT equalityExpressionNoIn { printf(" !== "); APP(" !== ");} relationalExpressionNoIn)

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
						  printf(" \%s ", $relationalExpressionNoIn::op);
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
						  printf(" \%s ", $shiftExpression::op);
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
						  printf(" + ");
						  APP(" + ");
						}
						multiplicativeExpression
					) 
	| ^(
	     SUB 
						e1=additiveExpression 
						 {
							  printf(" - ");
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
					  printf(" * ");
					  APP(" * ");
					 
					} 
					unaryExpression
					)
	| ^(DIV multiplicativeExpression {printf(" / "); APP(" / ");} unaryExpression)
	| ^(MOD multiplicativeExpression {printf(" \% "); APP(" \% ");} unaryExpression)
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
				   DELETE          { printf("delete"); APP("delete");}
       | VOID          { printf("void") ;  APP("void");}
       | TYPEOF        { printf("typeOf"); APP("typeOf");}
       | PLUSPLUS      { printf("++"); APP("++");}
       | MINUSMINUS    { printf("--"); APP("--");}
       | UNARY_PLUS    { printf("+"); APP("+");}
       | UNARY_MINUS   { printf("-"); APP("-");}
       | COMPLEMENT    { printf("~"); APP("~");}
       | NOT           { printf("!"); APP("!");}
	
					)

	    unaryExpression
				)
	;
	

postfixExpression
	: leftHandSideExpression '++'?
	| leftHandSideExpression '--'?
	;

primaryExpression
	: 'this' {printf("this"); APP("this");}
	| Identifier {printf($Identifier.text->chars); APP($Identifier.text->chars);}
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
								  APP("[ ");
								}
	       head=assignmentExpression? 
	
	     tail=assignmentExpression*)

        {
								  printf(" ] ");
								  APP(" ] ");

								}
	;
       
// objectLiteral definition.
objectLiteral
	:^(OBJ_LITERAL 
	   
				{ printf("{ "); APP("{ ");}
	   (head=propertyNameAndValue)? 
				(tail=propertyNameAndValue)*
				{ printf(" } "); APP(" } "); }
				
				)
	;
	
propertyNameAndValue
	: ^(NAME_VALUE 
	   propertyName 
			{	printf(" : "); APP(" : ");}
				assignmentExpression)
	;

propertyName
	: Identifier { printf($Identifier.text->chars); APP($Identifier.text->chars); }
	| StringLiteral { printf($StringLiteral.text->chars); APP($StringLiteral.text->chars);  }
	| NumericLiteral {printf($NumericLiteral.text->chars); APP($NumericLiteral.text->chars);}
	;

// primitive literal definition.
literal
	: 'null' { printf("null");  APP("null");}
	| 'true' { printf("true");  APP("true"); }
	| 'false'{ printf("false"); APP("false");}
	| StringLiteral {printf($StringLiteral.text->chars); APP($StringLiteral.text->chars);}
	| NumericLiteral {printf($NumericLiteral.text->chars); APP($NumericLiteral.text->chars);}
	;
	
