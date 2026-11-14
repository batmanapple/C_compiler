#include <stdio.h>
#include <stdlib.h>
#include "type.h"

char *node_name[] = {
    "N_NULL", "N_PROGRAM", "N_EXP_IDENT", "N_EXP_INT_CONST", "N_EXP_FLOAT_CONST",
    "N_EXP_CHAR_CONST", "N_EXP_STRING_LITERAL", "N_EXP_ARRAY", "N_EXP_FUNCTION_CALL",
    "N_EXP_STRUCT", "N_EXP_ARROW", "N_EXP_POST_INC", "N_EXP_POST_DEC", "N_EXP_PRE_INC",
    "N_EXP_PRE_DEC", "N_EXP_AMP", "N_EXP_STAR", "N_EXP_NOT", "N_EXP_PLUS", "N_EXP_MINUS",
    "N_EXP_SIZE_EXP", "N_EXP_SIZE_TYPE", "N_EXP_CAST", "N_EXP_MUL", "N_EXP_DIV",
    "N_EXP_MOD", "N_EXP_ADD", "N_EXP_SUB", "N_EXP_LSS", "N_EXP_GTR", "N_EXP_LEQ",
    "N_EXP_GEQ", "N_EXP_NEQ", "N_EXP_EQL", "N_EXP_AND", "N_EXP_OR", "N_EXP_ASSIGN",
    "N_ARG_LIST", "N_ARG_LIST_NIL", "N_STMT_LABEL_CASE", "N_STMT_LABEL_DEFAULT",
    "N_STMT_COMPOUND", "N_STMT_EMPTY", "N_STMT_EXPRESSION", "N_STMT_IF", "N_STMT_IF_ELSE",
    "N_STMT_SWITCH", "N_STMT_WHILE", "N_STMT_DO", "N_STMT_FOR", "N_STMT_RETURN",
    "N_STMT_CONTINUE", "N_STMT_BREAK", "N_INIT_LIST", "N_INIT_LIST_ONE", "N_INIT_LIST_NIL"
};

char *type_kind_name[] = {
    "NULL", "ENUM", "ARRAY", "STRUCT", "UNION", "FUNC", "POINTER", "VOID"
};

char *id_kind_name[] = {
    "NULL", "VAR", "FUNC", "PARM", "FIELD", "TYPE", "ENUM", "STRUCT", "ENUM_LITERAL"
};

char *spec_name[] = {
    "NULL", "AUTO", "STATIC", "TYPEDEF"
};

// External arrays for mapping enum values to string names for printing
extern char * node_name[];
extern char *type_kind_name[];
extern char *id_kind_name[];
extern char *spec_name[];

// Function Prototypes
void print_sem_ast(A_NODE *);
void prt_sem_program(A_NODE *, int);
void prt_sem_initializer(A_NODE *, int);
void prt_sem_arg_expr_list(A_NODE *, int);
void prt_sem_statement(A_NODE *, int);
void prt_sem_statement_list(A_NODE *, int);
void prt_sem_for_expression(A_NODE *, int);
void prt_sem_expression(A_NODE *, int);
void prt_sem_A_TYPE(A_TYPE *, int);
void prt_sem_A_ID_LIST(A_ID *, int);
void prt_sem_A_ID(A_ID *, int);
void prt_sem_A_ID_NAME(A_ID *, int);
void prt_sem_LITERAL(int, int);
void prt_sem_integer(int, int);

extern A_LITERAL literal_table[];
extern A_TYPE *int_type, *float_type, *char_type, *void_type, *string_type;

// Helper functions for formatting (assumed to be defined elsewhere)
void print_space(int s) {
    int i;
    for (i = 0; i < s; i++) printf("|  ");
}

void print_node(A_NODE *node, int s) {
    print_space(s);
    printf("%s (%x,%d)\n", node_name[node->name], (unsigned int)node->type, node->value);
}

// Main entry point for printing the Semantic Tree
void print_sem_ast(A_NODE *node)
{
	printf("======= semantic tree ==========\n");
	prt_sem_program(node,0);
}

// Prints the root program node and traverses the global identifier list
void prt_sem_program(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_PROGRAM:
			prt_sem_A_ID_LIST(node->clink, s+1); // Print global variables/functions
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Recursively prints initialization lists (e.g., int a[] = {1, 2, 3})
void prt_sem_initializer(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_INIT_LIST:
			prt_sem_initializer(node->llink, s+1);
			prt_sem_initializer(node->rlink, s+1);
			break;
		case N_INIT_LIST_ONE:
			prt_sem_expression(node->clink, s+1);
			break;
		case N_INIT_LIST_NIL:
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Prints expressions based on their node type
// Handles variables, constants, arithmetic operations, and function calls
void prt_sem_expression(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_EXP_IDENT :
			prt_sem_A_ID_NAME(node->clink, s+1);
			break;
		case N_EXP_INT_CONST :
			prt_sem_integer(node->clink, s+1);
			break;
		case N_EXP_FLOAT_CONST :
			prt_sem_LITERAL(node->clink, s+1);
			break;
		case N_EXP_CHAR_CONST :
			prt_sem_integer(node->clink, s+1);
			break;
		case N_EXP_STRING_LITERAL :
			prt_sem_LITERAL(node->clink, s+1);
			break;
		case N_EXP_ARRAY :
			prt_sem_expression(node->llink, s+1); // Array name
			prt_sem_expression(node->rlink, s+1); // Index
			break;
		case N_EXP_FUNCTION_CALL : 
			prt_sem_expression(node->llink, s+1); // Function name
			prt_sem_arg_expr_list(node->rlink, s+1); // Arguments
			break;
		case N_EXP_STRUCT :
			prt_sem_expression(node->llink, s+1);
			prt_sem_A_ID_NAME(node->rlink, s+1);
			break;
		case N_EXP_ARROW :
			prt_sem_expression(node->llink, s+1);
			prt_sem_A_ID_NAME(node->rlink, s+1);
			break;
		// Unary Operators
		case N_EXP_POST_INC :
		case N_EXP_POST_DEC :
		case N_EXP_PRE_INC :
		case N_EXP_PRE_DEC :
		case N_EXP_AMP :
		case N_EXP_STAR :
		case N_EXP_NOT :
		case N_EXP_PLUS :
		case N_EXP_MINUS :
			prt_sem_expression(node->clink, s+1);
			break;
		case N_EXP_SIZE_EXP :
		case N_EXP_SIZE_TYPE :
			prt_sem_integer(node->clink, s+1);
			break;
		case N_EXP_CAST :
			prt_sem_A_TYPE(node->llink, s+1);
			prt_sem_expression(node->rlink, s+1);
			break;
		// Binary Operators
		case N_EXP_MUL :
		case N_EXP_DIV :
		case N_EXP_MOD :
		case N_EXP_ADD :
		case N_EXP_SUB :
		case N_EXP_LSS :
		case N_EXP_GTR :
		case N_EXP_LEQ :
		case N_EXP_GEQ :
		case N_EXP_NEQ :
		case N_EXP_EQL :
		case N_EXP_AND :
		case N_EXP_OR :
		case N_EXP_ASSIGN :
			prt_sem_expression(node->llink, s+1);
			prt_sem_expression(node->rlink, s+1);
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Prints the list of arguments in a function call
void prt_sem_arg_expr_list(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_ARG_LIST :
			prt_sem_expression(node->llink, s+1);
			prt_sem_arg_expr_list(node->rlink, s+1);
			break;
		case N_ARG_LIST_NIL :
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Prints different types of statements (control flow, assignments, etc.)
void prt_sem_statement(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_STMT_LABEL_CASE :
			prt_sem_integer(node->llink, s+1);
			prt_sem_statement(node->rlink, s+1);
			break;
		case N_STMT_LABEL_DEFAULT :
			prt_sem_statement(node->clink, s+1);
			break;
		case N_STMT_COMPOUND:
			if(node->llink) prt_sem_A_ID_LIST(node->llink, s+1); // Local variables
			prt_sem_statement_list(node->rlink, s+1); // Statements inside block
			break;
		case N_STMT_EMPTY:
			break;
		case N_STMT_EXPRESSION:
			prt_sem_expression(node->clink, s+1);
			break;
		case N_STMT_IF:
			prt_sem_expression(node->llink, s+1); // Condition
			prt_sem_statement(node->rlink, s+1); // True branch
			break;
		case N_STMT_IF_ELSE:
			prt_sem_expression(node->llink, s+1); // Condition
			prt_sem_statement(node->clink, s+1); // True branch
			prt_sem_statement(node->rlink, s+1); // False branch
			break;
		case N_STMT_SWITCH:
			prt_sem_expression(node->llink, s+1);
			prt_sem_statement(node->rlink, s+1);
			break;
		case N_STMT_WHILE:
			prt_sem_expression(node->llink, s+1);
			prt_sem_statement(node->rlink, s+1);
			break;
		case N_STMT_DO:
			prt_sem_statement(node->llink, s+1);
			prt_sem_expression(node->rlink, s+1);
			break;
		case N_STMT_FOR:
			prt_sem_for_expression(node->llink, s+1);
			prt_sem_statement(node->rlink, s+1);
			break;
		case N_STMT_CONTINUE:
			break;
		case N_STMT_BREAK:
			break;
		case N_STMT_RETURN:
			if(node->clink) prt_sem_expression(node->clink, s+1);
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Recursively prints a list of statements
void prt_sem_statement_list(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_STMT_LIST:
			prt_sem_statement(node->llink, s+1);
			prt_sem_statement_list(node->rlink, s+1);
			break;
		case N_STMT_LIST_NIL:
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Prints the components of a 'for' loop (init, condition, update)
void prt_sem_for_expression(A_NODE *node, int s)
{
	print_node(node,s);
	switch(node->name) {
		case N_FOR_EXP :
			if(node->llink) prt_sem_expression(node->llink, s+1); // Init
			if(node->clink) prt_sem_expression(node->clink, s+1); // Condition
			if(node->rlink) prt_sem_expression(node->rlink, s+1); // Increment
			break;
		default :
			printf("****syntax tree error******");
	}
}

// Helper to print an integer value with indentation
void prt_sem_integer(int a, int s)
{
	print_space(s);
	printf("INT=%d\n", a);
}

// Prints a literal value from the literal table based on its type
void prt_sem_LITERAL(int lit, int s)
{
	print_space(s);
	printf("LITERAL: ");
	if (literal_table[lit].type==int_type)
		printf("%d\n", literal_table[lit].value.i);
	if (literal_table[lit].type==float_type)
		printf("%f\n", literal_table[lit].value.f);
	else if (literal_table[lit].type==string_type)
		printf("%s\n", literal_table[lit].value.s);
}

// Prints type information recursively
// Uses checks to prevent infinite recursion for self-referencing types
void prt_sem_A_TYPE(A_TYPE *t, int s)
{
	print_space(s);
	if (t==int_type)
		printf("(int)\n");
	else if (t==float_type)
		printf("(float)\n");
	else if (t==char_type)
		printf("(char %d)\n",t->size);
	else if (t==void_type)
		printf("(void)\n");
	else if (t->kind==T_NULL)
		printf("(null)\n");
	else if (t->prt==FALSE) // Check if already printed to avoid infinite loops
		printf("(DONE:%x)\n",t);
    else if (t->kind == T_STRUCT || t->kind == T_UNION)
        t->prt = TRUE;
	else
		switch (t->kind) {
			case T_ENUM:
				t->prt=FALSE;
				printf("ENUM\n");
				print_space(s); printf("| ENUMERATORS\n");
				prt_sem_A_ID_LIST(t->field,s+2);
				break;
			case T_POINTER:
				t->prt=FALSE;
				printf("POINTER\n");
				print_space(s); printf("| ELEMENT_TYPE\n");
				prt_sem_A_TYPE(t->element_type,s+2);
				break;
			case T_ARRAY:
				t->prt=FALSE;
				printf("ARRAY\n");
				print_space(s); printf("| INDEX\n");
				prt_sem_integer(t->expr,s+2);
				print_space(s); printf("| ELEMENT_TYPE\n");
				prt_sem_A_TYPE(t->element_type,s+2);
				break;
			case T_STRUCT:
				t->prt=FALSE;
				printf("STRUCT\n");
				print_space(s); printf("| FIELD\n");
				prt_sem_A_ID_LIST(t->field,s+2);
				break;
			case T_UNION:
				t->prt=FALSE;
				printf("UNION\n");
				print_space(s); printf("| FIELD\n");
				prt_sem_A_ID_LIST(t->field,s+2);
				break;
			case T_FUNC:
				t->prt=FALSE;
				printf("FUNCTION\n");
				print_space(s); printf("| PARAMETER\n");
				prt_sem_A_ID_LIST(t->field,s+2);
				print_space(s); printf("| TYPE\n");
				prt_sem_A_TYPE(t->element_type,s+2);
				if (t->expr) {
					print_space(s); printf("| BODY\n");
					prt_sem_statement(t->expr,s+2);
				}
		}
}

// Prints a linked list of identifiers (used for parameters, fields, variables)
void prt_sem_A_ID_LIST(A_ID *id, int s)
{
	while (id) {
		prt_sem_A_ID(id,s);
		id=id->link;
	}
}

// Prints just the summary of an identifier (name, type address, value, etc.)
void prt_sem_A_ID_NAME(A_ID *id, int s)
{
	print_space(s);
	printf("(ID=\"%s\") TYPE:%x KIND:%s SPEC=%s LEV=%d VAL=%d ADDR=%d\n", id->name, id->type,
			id_kind_name[id->kind], spec_name[id->specifier],id->level,
			id->value, id->address);
}

// Prints full details of an identifier, including its type definition and initializer
void prt_sem_A_ID(A_ID *id, int s)
{
	print_space(s);
	printf("(ID=\"%s\") TYPE:%x KIND:%s SPEC=%s LEV=%d VAL=%d ADDR=%d\n", id->name, id->type,
			id_kind_name[id->kind], spec_name[id->specifier],id->level,
			id->value, id->address);
	if (id->type) {
		print_space(s);
		printf("| TYPE\n");
		prt_sem_A_TYPE(id->type,s+2);
	}
	if (id->init) {
		print_space(s);
		printf("| INIT\n");
		if (id->kind==ID_ENUM_LITERAL)
			if (id->init)
				prt_sem_integer(id->init,s+2);
			else ;
		else
			prt_sem_initializer(id->init,s+2); 
	}
}
