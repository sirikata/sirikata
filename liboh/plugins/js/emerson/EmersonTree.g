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
	ASTLabelType=pANTRL3_COMMON_TREE;
	tokenVocab=Emerson;
 language = C;
}

	program
	:^(PROG sourceElements) // omitting LT and EOF
;
	
 sourceElements
	:sourceElement+  // omitting the LT 
	;
	
	sourceElement
	: functionDeclaration
	| statement
	;
	
// functions
functionDeclaration
	: ^( FUNC_DECL Identifier formalParameterList functionBody)
	;
	
functionExpression
	: ^( FUNC_EXPR Identifier?  formalParameterList functionBody)
	;
	
formalParameterList
	: ^( FUNC_PARAMS Identifier? Identifier*)
	;

functionBody
	: (sourceElements)
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
	;
	
statementBlock
	:  statementList
	;
	
statementList
	:  ^(SLIST statement+)
	;
	
variableStatement
	:  ^( VARLIST variableDeclarationList)
	;
	
variableDeclarationList
	: variableDeclaration+
	;
	
variableDeclarationListNoIn
	: variableDeclarationNoIn+
	;
	
variableDeclaration
	: ^(VAR Identifier initialiser?)
	;
	
variableDeclarationNoIn
	: ^(VAR Identifier initialiserNoIn?)
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
	: ^(IF expression statement statement?)
	;
	
iterationStatement
	: doWhileStatement
	| whileStatement
	| forStatement
	| forInStatement
	;
	
doWhileStatement
	: ^( DO  statement expression )
	;
	
whileStatement
	: ^(WHILE  expression statement)
	;
	
forStatement
	: ^(FOR (^(FORINIT forStatementInitialiserPart))?   (^(FORCOND expression))?  (^(FORITER expression))?  statement)
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
	: ^(CONTINUE Identifier?)
	;

breakStatement
	: ^(BREAK Identifier?)
	;

returnStatement
	: ^(RETURN expression?)
	;
	
withStatement
	: ^(WITH expression statement)
	;

labelledStatement
	: ^( LABEL Identifier statement)
	;
	
switchStatement
	: ^(SWITCH expression caseBlock)
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
	: ^(TRY statementBlock finallyClause?) 
	;
       
catchClause
	: ^(CATCH Identifier statementBlock)
	;
	
finallyClause
	: ^( FINALLY statementBlock )
	;

// expressions
expression
	: ^(EXPR_LIST assignmentExpression+)
	;
	
expressionNoIn
	: ^(EXPR_LIST assignmentExpressionNoIn+)

	;
	
assignmentExpression
	: ^(COND_EXPR conditionalExpression)
	| ^(assignmentOperator  leftHandSideExpression assignmentExpression)
	;
	
assignmentExpressionNoIn
	: conditionalExpressionNoIn
	| ^(assignmentOperator leftHandSideExpression assignmentExpressionNoIn ) 
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
	| ^(NEW memberExpression arguments) 
	| ^(memberExpressionSuffix memberExpression memberExpression)
	;


memberExpressionSuffix
	: indexSuffix
	| propertyReferenceSuffix 
	;

callExpression
 : ^(CALL memberExpression arguments)
	|^(callExpressionSuffix callExpression callExpression)
	;
	
callExpressionSuffix
	: arguments
	| indexSuffix
	| propertyReferenceSuffix
	;

arguments
	: ^(ARGLIST assignmentExpression*)
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
	: ^(logicalORExpression (assignmentExpression assignmentExpression)? )
	;

conditionalExpressionNoIn
	:^(logicalORExpressionNoIn (assignmentExpressionNoIn assignmentExpressionNoIn)?)
	;


logicalANDExpression
	: bitwiseORExpression
	|^(AND logicalANDExpression bitwiseORExpression)
	;


logicalORExpression
	: logicalANDExpression
	|^(OR logicalORExpression logicalANDExpression)
	;
	
logicalORExpressionNoIn
	: logicalANDExpressionNoIn
	|^(OR logicalORExpressionNoIn logicalANDExpressionNoIn) 
	;
	

logicalANDExpressionNoIn
	: bitwiseORExpressionNoIn 
	|^(AND logicalANDExpressionNoIn bitwiseORExpressionNoIn) 
	;
	
bitwiseORExpression
	: bitwiseXORExpression 
	|^(BIT_OR bitwiseORExpression bitwiseXORExpression)
	;
	
bitwiseORExpressionNoIn
	: bitwiseXORExpressionNoIn 
	|^( BIT_OR bitwiseORExpressionNoIn bitwiseXORExpressionNoIn)
	;
	
bitwiseXORExpression
: bitwiseANDExpression 
| ^( EXP bitwiseXORExpression bitwiseANDExpression)
;
	
bitwiseXORExpressionNoIn
	: bitwiseANDExpressionNoIn
	|^( EXP bitwiseXORExpressionNoIn bitwiseANDExpressionNoIn) 
	;
	
bitwiseANDExpression
	: equalityExpression
	| ^(BIT_AND bitwiseANDExpression equalityExpression) 
	;
	
bitwiseANDExpressionNoIn
	: equalityExpressionNoIn 
	| ^(BIT_AND bitwiseANDExpressionNoIn equalityExpressionNoIn)
	;
	
equalityExpression
	: relationalExpression
	|^(equalityOps equalityExpression relationalExpression)
	;

equalityOps
: EQUALS
| NOT_EQUALS
| IDENT
| NOT_IDENT
;

equalityExpressionNoIn
: relationalExpressionNoIn
|^(equalityOps equalityExpressionNoIn relationalExpressionNoIn)
;
	

relationalOps
: LESS_THAN
| GREATER_THAN
| LESS_THAN_EQUAL
| GREATER_THAN_EQUAL
| INSTANCE_OF
| IN
;

relationalExpression
	: shiftExpression 
	|^(relationalOps relationalExpression shiftExpression) 
	;

relationalOpsNoIn
: LESS_THAN
| GREATER_THAN
| LESS_THAN_EQUAL
| GREATER_THAN_EQUAL
| INSTANCE_OF
;

relationalExpressionNoIn
	: shiftExpression 
	| ^(relationalOpsNoIn relationalExpressionNoIn shiftExpression)
	;

shiftOps
: LEFT_SHIFT
| RIGHT_SHIFT
| TRIPLE_SHIFT
;

shiftExpression
	: additiveExpression
	| ^(shiftOps shiftExpression additiveExpression)
 ;	


addOps
: ADD
| SUB
;


additiveExpression
	: multiplicativeExpression
	| ^(addOps additiveExpression multiplicativeExpression) 
	;

multOps
: MULT
| DIV
| MOD
;

multiplicativeExpression
	: unaryExpression
	| ^(multOps multiplicativeExpression unaryExpression)
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
	| ^(unaryOps unaryExpression)
	;
	

postfixExpression
	: leftHandSideExpression '++'?
	| leftHandSideExpression '--'?
	;

primaryExpression
	: 'this'
	| Identifier
	| literal
	| arrayLiteral
	| objectLiteral
	| expression
	;
	
// arrayLiteral definition.
arrayLiteral
	: ^(ARRAY_LITERAL assignmentExpression? assignmentExpression*)
	;
       
// objectLiteral definition.
objectLiteral
	:^(OBJ_LITERAL propertyNameAndValue? propertyNameAndValue*)
	;
	
propertyNameAndValue
	: ^(NAME_VALUE propertyName assignmentExpression)
	;

propertyName
	: Identifier
	| StringLiteral
	| NumericLiteral
	;

// primitive literal definition.
literal
	: 'null'
	| 'true'
	| 'false'
	| StringLiteral
	| NumericLiteral
	;
	
