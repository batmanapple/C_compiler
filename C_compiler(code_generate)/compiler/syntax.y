/* ================================================================== */
/* 1. Definitions Section (Prologue) */
/* ================================================================== */
%{
	#define YYSTYPE_IS_DECLARED  1
	typedef  long YYSTYPE;

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include "type.h"

	extern int line_no, syntax_err;
// External variables from lexer (line number, error flag)
	extern A_NODE *root;
// The root of the Abstract Syntax Tree (AST)
	extern A_ID *current_id;
// Pointer to the current identifier in the symbol table
	extern int current_level;		    // Current scope level
	extern A_TYPE *int_type;
// Predefined integer type

	/* === Function prototypes from synfun.c === */
	A_NODE			*makeNode (NODE_NAME,A_NODE *,A_NODE *,A_NODE *);
	A_NODE			*makeNodeList (NODE_NAME,A_NODE *,A_NODE *);
	A_ID			*makeIdentifier(char *);
	A_ID			*makeDummyIdentifier();
	A_TYPE			*makeType(T_KIND);
	A_SPECIFIER		*makeSpecifier(A_TYPE *,S_KIND);
	A_ID			*searchIdentifier(char *,A_ID *);
	A_ID			*searchIdentifierAtCurrentLevel(char *,A_ID *);
	A_SPECIFIER		*updateSpecifier(A_SPECIFIER *, A_TYPE *, S_KIND);
	void			checkForwardReference();
	void			setDefaultSpecifier(A_SPECIFIER *);
	A_ID			*linkDeclaratorList(A_ID *,A_ID *) ;
	A_ID			*getIdentifierDeclared(char *);
	A_TYPE			*getTypeOfStructOrEnumRefIdentifier(T_KIND,char *,ID_KIND);
	A_ID			*setDeclaratorInit(A_ID *,A_NODE *);
	A_ID			*setDeclaratorKind(A_ID *,ID_KIND);
	A_ID			*setDeclaratorType(A_ID *,A_TYPE *);
	A_ID			*setDeclaratorElementType(A_ID *,A_TYPE *);
	A_ID			*setDeclaratorTypeAndKind(A_ID *,A_TYPE *,ID_KIND);
	A_ID			*setDeclaratorListSpecifier(A_ID *,A_SPECIFIER *);
	A_ID			*setFunctionDeclaratorSpecifier(A_ID *, A_SPECIFIER *);
	A_ID			*setFunctionDeclaratorBody(A_ID *, A_NODE *);
	A_ID			*setParameterDeclaratorSpecifier(A_ID *, A_SPECIFIER *);
	A_ID			*setStructDeclaratorListSpecifier(A_ID *, A_TYPE *);
	A_TYPE			*setTypeNameSpecifier(A_TYPE *, A_SPECIFIER *);
	A_TYPE			*setTypeElementType(A_TYPE *,A_TYPE *);
	A_TYPE			*setTypeField(A_TYPE *,A_ID *);
	A_TYPE			*setTypeExpr(A_TYPE *,A_NODE *);
	A_TYPE			*setTypeAndKindOfDeclarator(A_TYPE *,ID_KIND,A_ID *);
	A_TYPE			*setTypeStructOrEnumIdentifier(T_KIND,char *,ID_KIND);
	BOOLEAN			isNotSameFormalParameters(A_ID *, A_ID *);
	BOOLEAN			isNotSameType(A_TYPE *, A_TYPE *);
	BOOLEAN			isPointerOrArrayType(A_TYPE *);
	void			syntax_error();
	void			initialize();

	/* === Prototypes for lexer/parser functions === */
	int yylex();
	void yyerror(char *s);
%}

/* ================================================================== */
/* 2. Yacc Declarations Section */
/* ================================================================== */

/* Define the starting rule for the grammar */
%start program

/* Declare all terminal symbols (tokens) provided by the lexer */
%token	IDENTIFIER TYPE_IDENTIFIER FLOAT_CONSTANT INTEGER_CONSTANT
		CHARACTER_CONSTANT STRING_LITERAL PLUS MINUS PLUSPLUS
		MINUSMINUS BAR AMP BARBAR AMPAMP ARROW
		SEMICOLON LSS GTR LEQ GEQ EQL NEQ DOTDOTDOT
		LP RP LB RB LR RR PERIOD COMMA EXCL STAR SLASH PERCENT ASSIGN
		COLON AUTO_SYM STATIC_SYM TYPEDEF_SYM
		STRUCT_SYM ENUM_SYM SIZEOF_SYM UNION_SYM
		IF_SYM ELSE_SYM WHILE_SYM DO_SYM FOR_SYM CONTINUE_SYM
		BREAK_SYM RETURN_SYM SWITCH_SYM CASE_SYM DEFAULT_SYM

/* ================================================================== */
/* 3. Grammar Rules Section */
/* ================================================================== */
%%

/* The top-level rule, representing the entire program */
program
	: translation_unit
	  /* Create the 
root node of the AST and check for forward references */
	  {root=makeNode(N_PROGRAM,NIL,(A_NODE*)$1,NIL); checkForwardReference();}
	;
/* A translation unit is a sequence of external declarations */
translation_unit
	: external_declaration					{$$=$1;}
	| translation_unit external_declaration	{$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$2);}
	;
/* An external declaration is either a function definition or a global declaration */
external_declaration
	: function_definition					{$$=$1;}
	| declaration							{$$=$1;}
	;
/* Rules for defining a function */
function_definition
	: declaration_specifiers declarator		{$$=(long)setFunctionDeclaratorSpecifier((A_ID*)$2,(A_SPECIFIER*)$1);}
	  compound_statement					{$$=(long)setFunctionDeclaratorBody((A_ID*)$3,(A_NODE*)$4);current_id=(A_ID*)$2;}
	;
/* Optional list of declarations (e.g., inside a compound statement) */
declaration_list_opt
	:										{$$=NIL;}
	| declaration_list						{$$=$1;}
	;
/* A list of one or more declarations */
declaration_list
	: declaration							{$$=$1;}
	| declaration_list declaration			{$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$2);}
	;
/* A single declaration (e.g., "int a, b;") */
declaration
	: declaration_specifiers init_declarator_list_opt SEMICOLON	
	  {$$=(long)setDeclaratorListSpecifier((A_ID*)$2,(A_SPECIFIER*)$1);}
	;
/* Specifiers for a declaration (e.g., "static int") */
declaration_specifiers
	: type_specifier						{$$=(long)makeSpecifier((A_TYPE*)$1,0);}
	| storage_class_specifier				{$$=(long)makeSpecifier(0,(S_KIND)$1);}
	| type_specifier declaration_specifiers	{$$=(long)updateSpecifier((A_SPECIFIER*)$2,(A_TYPE*)$1,0);}
	| storage_class_specifier declaration_specifiers {$$=(long)updateSpecifier((A_SPECIFIER*)$2,0,(S_KIND)$1);}
	;
/* Storage class keywords */
storage_class_specifier
	: AUTO_SYM		{$$=S_AUTO;}
	| STATIC_SYM	{$$=S_STATIC;}
	| TYPEDEF_SYM	{$$=S_TYPEDEF;}
	;

/* Optional list of initialized declarators (e.g., "a=1, b") */
init_declarator_list_opt
	:							{$$=(long)makeDummyIdentifier();}
	| init_declarator_list		{$$=$1;}
	;
/* List of one or more initialized declarators */
init_declarator_list
	: init_declarator								{$$=$1;}
	| init_declarator_list COMMA init_declarator	{$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$3);}
	;
/* A declarator, optionally with an initializer */
init_declarator
	: declarator					{$$=$1;}
	| declarator ASSIGN initializer {$$=(long)setDeclaratorInit((A_ID*)$1,(A_NODE*)$3);}
	;
/* An initializer (e.g., "= 10" or "= {1, 2}") */
initializer
	: constant_expression			{$$=(long)makeNode(N_INIT_LIST_ONE,NIL,(A_NODE*)$1,NIL);}
	| LR initializer_list RR		{$$=$2;} /* For aggregate types */
	;
/* A list of initializers for arrays or structs */
initializer_list
	: initializer						 {$$=(long)makeNode(N_INIT_LIST,(A_NODE*)$1,NIL,makeNode(N_INIT_LIST_NIL,NIL,NIL,NIL));}
	| initializer_list COMMA initializer {$$=(long)makeNodeList(N_INIT_LIST,(A_NODE*)$1,(A_NODE*)$3);}
	;
/* Specifies a type */
type_specifier
	: struct_type_specifier				 {$$ = $1;}
	| enum_type_specifier				 {$$ = $1;}
	|
TYPE_IDENTIFIER					 {$$ = $1;} /* A typedef name */
	;

/* Rules for 'struct' or 'union' type specifiers */
struct_type_specifier
	: struct_or_union IDENTIFIER /* 'struct MyStruct' */
	  {$$=(long)setTypeStructOrEnumIdentifier((T_KIND)$1,(char*)$2,ID_STRUCT);}
	  LR { $$=(long)current_id;current_level++;} struct_declaration_list RR /* '{ ... }' definition */
	  {checkForwardReference();
$$=(long)setTypeField((A_TYPE*)$3,(A_ID*)$6); current_level--;
	  current_id=(A_ID*)$5; }
	| struct_or_union {$$=(long)makeType((T_KIND)$1);} LR {$$=(long)current_id;current_level++;} /* 'struct { ... }' anonymous */
	  struct_declaration_list RR {checkForwardReference();$$=(long)setTypeField((A_TYPE*)$2,(A_ID*)$5);
	  current_level--;current_id=(A_ID*)$4;}
	|
struct_or_union IDENTIFIER /* 'struct MyStruct' usage */
	  {$$=(long)getTypeOfStructOrEnumRefIdentifier((T_KIND)$1,(char*)$2,ID_STRUCT);}
	;

/* Distinguishes between STRUCT and UNION tokens */
struct_or_union
	: STRUCT_SYM		{$$=T_STRUCT;}
	| UNION_SYM			{$$=T_UNION;}
	;
/* List of declarations inside a struct/union */
struct_declaration_list
	: struct_declaration							{$$=$1;}
	| struct_declaration_list struct_declaration	{$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$2);}
	;
/* A single declaration within a struct/union */
struct_declaration
	: type_specifier struct_declarator_list SEMICOLON
	  {$$=(long)setStructDeclaratorListSpecifier((A_ID*)$2,(A_TYPE*)$1);}
	;
/* List of declarators within a struct (e.g., "int a, b;") */
struct_declarator_list
	: struct_declarator									{$$=$1;}
	| struct_declarator_list COMMA struct_declarator
	  {$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$3);}
	;
/* A single declarator within a struct */
struct_declarator
	: declarator										{$$=$1;}
	;

/* Rules for 'enum' type specifiers */
enum_type_specifier
	: ENUM_SYM IDENTIFIER	{$$=(long)setTypeStructOrEnumIdentifier(T_ENUM,(char*)$2,ID_ENUM);}
	  LR enumerator_list RR	{$$=(long)setTypeField((A_TYPE*)$3,(A_ID*)$5);}
	|
ENUM_SYM				{$$=(long)makeType(T_ENUM);}
	  LR enumerator_list RR	{$$=(long)setTypeField((A_TYPE*)$2,(A_ID*)$4);}
	| ENUM_SYM IDENTIFIER
	  {$$=(long)getTypeOfStructOrEnumRefIdentifier(T_ENUM,(char*)$2,ID_ENUM);}
	;

/* List of enumerators (e.g., "ONE, TWO") */
enumerator_list
	: enumerator							{$$=$1;}
	|
enumerator_list COMMA enumerator		{$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$3);}
	;

/* A single enumerator, optionally with an assigned value */
enumerator
	: IDENTIFIER
      {$$=(long)setDeclaratorKind(makeIdentifier((char*)$1),ID_ENUM_LITERAL);}
	|
IDENTIFIER
	  {$$=(long)setDeclaratorKind(makeIdentifier((char*)$1),ID_ENUM_LITERAL);}
	  ASSIGN expression {$$=(long)setDeclaratorInit((A_ID*)$2,(A_NODE*)$4);}
	;

/* A declarator (e.g., "*a", "a[]", "f(int)") */
declarator
	: pointer direct_declarator	{$$=(long)setDeclaratorElementType((A_ID*)$2,(A_TYPE*)$1);}
	| direct_declarator			{$$=$1;}
	;
/* Pointer part of a declarator (e.g., "*" or "**") */
pointer
	: STAR			{$$=(long)makeType(T_POINTER);}
	| STAR pointer	{$$=(long)setTypeElementType((A_TYPE*)$2,makeType(T_POINTER));}
	;
/* The main part of a declarator (ID, array, function) */
direct_declarator
	: IDENTIFIER		{$$=(long)makeIdentifier((char*)$1);}
	| LP declarator RP	{$$=$2;} /* Parenthesized declarator */
	|
direct_declarator LB constant_expression_opt RB /* Array declarator */
	  {$$=(long)setDeclaratorElementType((A_ID*)$1,setTypeExpr(makeType(T_ARRAY),(A_NODE*)$3));}
	|
direct_declarator LP {$$=(long)current_id;current_level++;} /* Function declarator */
	  parameter_type_list_opt RP
	  {checkForwardReference();current_id=(A_ID*)$3;current_level--;
	  $$=(long)setDeclaratorElementType((A_ID*)$1,setTypeField(makeType(T_FUNC),(A_ID*)$4));}
	;

/* Optional list of function parameters */
parameter_type_list_opt
	:						{$$=NIL;}
	|
parameter_type_list	{$$=$1;}
	;

/* A list of parameters, possibly ending with '...' */
parameter_type_list
	: parameter_list {$$=$1;}
	| parameter_list COMMA DOTDOTDOT {$$=(long)linkDeclaratorList(
	  (A_ID*)$1,setDeclaratorKind(makeDummyIdentifier(),ID_PARM));}
	;
/* List of one or more parameter declarations */
parameter_list
	: parameter_declaration			{$$=$1;}
	| parameter_list COMMA parameter_declaration
	  {$$=(long)linkDeclaratorList((A_ID*)$1,(A_ID*)$3);}
	;
/* A single function parameter declaration */
parameter_declaration
	: declaration_specifiers declarator
	  {$$=(long)setParameterDeclaratorSpecifier((A_ID*)$2,(A_SPECIFIER*)$1);}
	|
declaration_specifiers abstract_declarator_opt /* Parameter with optional name */
  	  {$$=(long)setParameterDeclaratorSpecifier(setDeclaratorType(
	  makeDummyIdentifier(),(A_TYPE*)$2),(A_SPECIFIER*)$1);}
	;
/* Optional abstract declarator (e.g., in a cast) */
abstract_declarator_opt
	:						{$$=NIL;}
	| abstract_declarator	{$$=$1;}
	;
/* A declarator for a type, without an identifier (e.g., "int *", "char []") */
abstract_declarator
	: direct_abstract_declarator			{$$=$1;}
	| pointer								{$$=(long)makeType(T_POINTER);}
	| pointer direct_abstract_declarator	{$$=(long)setTypeElementType((A_TYPE*)$2,makeType(T_POINTER));}
	;
/* The direct (non-pointer) part of an abstract declarator */
direct_abstract_declarator
	: LP abstract_declarator RP				{$$=$2;}
	| LB constant_expression_opt RB
	  {$$=(long)setTypeExpr(makeType(T_ARRAY),(A_NODE*)$2);}
	|
direct_abstract_declarator LB constant_expression_opt RB
	  {$$=(long)setTypeElementType((A_TYPE*)$1,setTypeExpr(makeType(T_ARRAY),(A_NODE*)$3));}
	| LP parameter_type_list_opt RP
	  {$$=(long)setTypeField(makeType(T_FUNC),(A_ID*)$2);}
	| direct_abstract_declarator LP parameter_type_list_opt RP
	  {$$=(long)setTypeElementType((A_TYPE*)$1,setTypeField(makeType(T_FUNC),(A_ID*)$3));}
	;
/* --- STATEMENTS --- */

/* Optional list of statements (e.g., in a compound statement) */
statement_list_opt
	:						{$$=(long)makeNode(N_STMT_LIST_NIL,NIL,NIL,NIL);}
	| statement_list		{$$=$1;}
	;
/* List of one or more statements */
statement_list
	: statement {$$=(long)makeNode(N_STMT_LIST,(A_NODE*)$1,NIL,
	  makeNode(N_STMT_LIST_NIL,NIL,NIL,NIL));}
	| statement_list statement {$$=(long)makeNodeList(N_STMT_LIST,(A_NODE*)$1,(A_NODE*)$2);}
	;
/* A generic C statement */
statement
	: labeled_statement		{$$=$1;}
	| compound_statement	{$$=$1;}
	| expression_statement	{$$=$1;}
	| selection_statement	{$$=$1;}
	| iteration_statement	{$$=$1;}
	| jump_statement		{$$=$1;}
	;
/* A statement with a 'case' or 'default' label */
labeled_statement
	: CASE_SYM constant_expression COLON statement
	  {$$=(long)makeNode(N_STMT_LABEL_CASE, (A_NODE*)$2,NIL,(A_NODE*)$4);}
	|
DEFAULT_SYM COLON statement
	  {$$=(long)makeNode(N_STMT_LABEL_DEFAULT,NIL,(A_NODE*)$3,NIL);}
	;

/* A compound statement: a block with optional declarations and statements */
compound_statement
	: LR {$$=(long)current_id;current_level++;} /* Enter new scope */
	  declaration_list_opt
	  statement_list_opt 
	  RR {checkForwardReference();
/* Exit scope */
	  $$=(long)makeNode(N_STMT_COMPOUND,(A_NODE*)$3,NIL,(A_NODE*)$4); current_id=(A_ID*)$2;
	  current_level--;}
	;

/* A statement that is just an expression or empty */
expression_statement
	: SEMICOLON				{$$=(long)makeNode(N_STMT_EMPTY,NIL,NIL,NIL);}
	|
expression SEMICOLON	{$$=(long)makeNode(N_STMT_EXPRESSION,NIL,(A_NODE*)$1,NIL);}
	;

/* Selection statements: 'if', 'if-else', 'switch' */
selection_statement
	: IF_SYM LP expression RP statement
	  {$$=(long)makeNode(N_STMT_IF,(A_NODE*)$3,NIL,(A_NODE*)$5);}
	|
IF_SYM LP expression RP statement ELSE_SYM statement
	  {$$=(long)makeNode(N_STMT_IF_ELSE,(A_NODE*)$3,(A_NODE*)$5,(A_NODE*)$7);}
	| SWITCH_SYM LP expression RP statement
	  {$$=(long)makeNode(N_STMT_SWITCH,(A_NODE*)$3,NIL,(A_NODE*)$5);}
	;
/* Iteration (loop) statements: 'while', 'do-while', 'for' */
iteration_statement
	: WHILE_SYM LP expression RP statement
	  {$$=(long)makeNode(N_STMT_WHILE,(A_NODE*)$3,NIL,(A_NODE*)$5);}
	|
DO_SYM statement WHILE_SYM LP expression RP SEMICOLON
	  {$$=(long)makeNode(N_STMT_DO,(A_NODE*)$2,NIL,(A_NODE*)$5);}
	| FOR_SYM LP for_expression RP statement
	  {$$=(long)makeNode(N_STMT_FOR,(A_NODE*)$3,NIL,(A_NODE*)$5);}
	;
/* The three (optional) expressions inside a 'for' loop */
for_expression
	: expression_opt SEMICOLON expression_opt SEMICOLON expression_opt
	  {$$=(long)makeNode(N_FOR_EXP,(A_NODE*)$1,(A_NODE*)$3,(A_NODE*)$5);}
	;
/* An optional expression */
expression_opt
	: /* empty */	{$$=NIL;}
	| expression	{$$=$1;}
	;

/* Jump statements: 'return', 'continue', 'break' */
jump_statement
	: RETURN_SYM expression_opt SEMICOLON
	  {$$=(long)makeNode(N_STMT_RETURN,NIL,(A_NODE*)$2,NIL);}
	|
CONTINUE_SYM SEMICOLON
	  {$$=(long)makeNode(N_STMT_CONTINUE,NIL,NIL,NIL);}
	| BREAK_SYM SEMICOLON
	  {$$=(long)makeNode(N_STMT_BREAK,NIL,NIL,NIL);}
	;

/* --- EXPRESSIONS --- */

/* Optional list of arguments for a function call */
arg_expression_list_opt
	:							{$$=(long)makeNode(N_ARG_LIST_NIL,NIL,NIL,NIL);}
	|
arg_expression_list		{$$=$1;}
	;

/* List of one or more function arguments */
arg_expression_list
	: assignment_expression
	  {$$=(long)makeNode(N_ARG_LIST,(A_NODE*)$1,NIL,makeNode(N_ARG_LIST_NIL,NIL,NIL,NIL));}
	| arg_expression_list COMMA assignment_expression
	  {$$=(long)makeNodeList(N_ARG_LIST,(A_NODE*)$1,(A_NODE*)$3);}
	;
/* Optional constant expression (e.g., in array size) */
constant_expression_opt
	:							{$$=NIL;}
	| constant_expression		{$$=$1;}
	;
/* A constant expression (simplified to just 'expression' here) */
constant_expression
	: expression				{$$=$1;}
	;

/* Top-level expression rule (starts precedence cascade) */
expression
	: comma_expression			{$$=$1;}
	;
/* Comma operator (lowest precedence) */
comma_expression
	: assignment_expression		{$$=$1;}
	;

/* Assignment expression */
assignment_expression
	: conditional_expression	{$$=$1;}
	| unary_expression ASSIGN assignment_expression
	  {$$=(long)makeNode(N_EXP_ASSIGN,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;
/* Conditional (ternary) operator (?:) - not fully implemented in this rule */
conditional_expression
	: logical_or_expression		{$$=$1;}
	;

/* Logical OR (||) */
logical_or_expression
	: logical_and_expression	{$$=$1;}
	|
logical_or_expression BARBAR logical_and_expression
	  {$$=(long)makeNode(N_EXP_OR,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;

/* Logical AND (&&) */
logical_and_expression
	: bitwise_or_expression		{$$=$1;}
	| logical_and_expression AMPAMP bitwise_or_expression
	  {$$=(long)makeNode(N_EXP_AND,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;
/* Bitwise OR (|) - not fully implemented */
bitwise_or_expression
	: bitwise_xor_expression	{$$=$1;}
	;

/* Bitwise XOR (^) - not fully implemented */
bitwise_xor_expression
	: bitwise_and_expression	{$$=$1;}
	;
/* Bitwise AND (&) - not fully implemented */
bitwise_and_expression
	: equality_expression		{$$=$1;}
	;

/* Equality operators (==, !=) */
equality_expression
	: relational_expression		{$$=$1;}
	|
equality_expression EQL relational_expression
	  {$$=(long)makeNode(N_EXP_EQL,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| equality_expression NEQ relational_expression
	  {$$=(long)makeNode(N_EXP_NEQ,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;

/* Relational operators (<, >, <=, >=) */
relational_expression
	: shift_expression			{$$=$1;}
	|
relational_expression LSS shift_expression
	  {$$=(long)makeNode(N_EXP_LSS,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| relational_expression GTR shift_expression
	  {$$=(long)makeNode(N_EXP_GTR,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| relational_expression LEQ shift_expression
	  {$$=(long)makeNode(N_EXP_LEQ,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| relational_expression GEQ shift_expression
	  {$$=(long)makeNode(N_EXP_GEQ,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;
/* Shift operators (<<, >>) - not fully implemented */
shift_expression
	: additive_expression		{$$=$1;}
	;

/* Additive operators (+, -) */
additive_expression
	: multiplicative_expression	{$$=$1;}
	|
additive_expression PLUS multiplicative_expression
	  {$$=(long)makeNode(N_EXP_ADD,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| additive_expression MINUS multiplicative_expression
	  {$$=(long)makeNode(N_EXP_SUB,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;

/* Multiplicative operators (*, /, %) */
multiplicative_expression
	: cast_expression			{$$=$1;}
	|
multiplicative_expression STAR cast_expression
	  {$$=(long)makeNode(N_EXP_MUL,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| multiplicative_expression SLASH cast_expression
	  {$$= (long)makeNode(N_EXP_DIV,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| multiplicative_expression PERCENT cast_expression
	  {$$= (long)makeNode(N_EXP_MOD,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	;
/* Cast expression (e.g., "(int)a") */
cast_expression
	: unary_expression			{$$=$1;}
	| LP type_name RP cast_expression
	  {$$=(long)makeNode(N_EXP_CAST,(A_NODE*)$2,NIL,(A_NODE*)$4);}
	;
/* Unary operators (++, --, &, *, !, -, +, sizeof) */
unary_expression
	: postfix_expression			  {$$=$1;}
	| PLUSPLUS unary_expression		  {$$=(long)makeNode(N_EXP_PRE_INC,NIL,(A_NODE*)$2,NIL);}
	|
MINUSMINUS unary_expression	  {$$=(long)makeNode(N_EXP_PRE_DEC,NIL,(A_NODE*)$2,NIL);}
	| AMP cast_expression			  {$$=(long)makeNode(N_EXP_AMP,NIL,(A_NODE*)$2,NIL);} /* Address-of */
	| STAR cast_expression			  {$$=(long)makeNode(N_EXP_STAR,NIL,(A_NODE*)$2,NIL);} /* Dereference */
	|
EXCL cast_expression			  {$$=(long)makeNode(N_EXP_NOT,NIL,(A_NODE*)$2,NIL);}
	| MINUS cast_expression			  {$$=(long)makeNode(N_EXP_MINUS,NIL,(A_NODE*)$2,NIL);}
	| PLUS cast_expression			  {$$=(long)makeNode(N_EXP_PLUS,NIL,(A_NODE*)$2,NIL);}
	| SIZEOF_SYM unary_expression	  {$$=(long)makeNode(N_EXP_SIZE_EXP,NIL,(A_NODE*)$2,NIL);}
	|
SIZEOF_SYM LP type_name RP      {$$=(long)makeNode(N_EXP_SIZE_TYPE,NIL,(A_NODE*)$3,NIL);}
	;
/* Postfix expressions (array index, function call, struct access, ++, --) */
postfix_expression
	: primary_expression								 {$$=$1;}
	| postfix_expression LB expression RB				 {$$=(long)makeNode(N_EXP_ARRAY,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	|
postfix_expression LP arg_expression_list_opt RP	 {$$=(long)makeNode(N_EXP_FUNCTION_CALL,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| postfix_expression PERIOD IDENTIFIER				 {$$=(long)makeNode(N_EXP_STRUCT,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| postfix_expression ARROW IDENTIFIER				 {$$=(long)makeNode(N_EXP_ARROW,(A_NODE*)$1,NIL,(A_NODE*)$3);}
	| postfix_expression PLUSPLUS						 {$$=(long)makeNode(N_EXP_POST_INC,NIL,(A_NODE*)$1,NIL);}
	| postfix_expression MINUSMINUS						 {$$=(long)makeNode(N_EXP_POST_DEC,NIL,(A_NODE*)$1,NIL);}
	;
/* Primary expressions (literals, identifiers, parenthesized expressions) */
primary_expression
	: IDENTIFIER		{$$=(long)makeNode(N_EXP_IDENT,NIL,(A_NODE*)getIdentifierDeclared((char*)$1),NIL);}
	| INTEGER_CONSTANT	{$$=(long)makeNode(N_EXP_INT_CONST,NIL,(A_NODE*)$1,NIL);}
	| FLOAT_CONSTANT	{$$=(long)makeNode(N_EXP_FLOAT_CONST,NIL,(A_NODE*)$1,NIL);}
	| CHARACTER_CONSTANT{$$=(long)makeNode(N_EXP_CHAR_CONST,NIL,(A_NODE*)$1,NIL);}
	| STRING_LITERAL	{$$=(long)makeNode(N_EXP_STRING_LITERAL,NIL,(A_NODE*)$1,NIL);}
	| LP expression RP	{$$=$2;}
	;
/* A type name (used in casts and sizeof) */
type_name
	: declaration_specifiers abstract_declarator_opt
	  {$$=(long)setTypeNameSpecifier((A_TYPE*)$2,(A_SPECIFIER*)$1);}
	;
/* ================================================================== */
/* 4. Epilogue Section (User Code) */
/* ================================================================== */
%%
extern char *yytext;
/* Text of the current token from lexer */

/* Error reporting function required by yacc */
void yyerror(char *s)
{
	syntax_err++;
printf("line %d: %s near %s \n", line_no, s,yytext);
}
