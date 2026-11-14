#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "type.h"
#include "codegen.h"

extern int syntax_err;
extern int semantic_err;
extern FILE *fout;
extern A_TYPE *int_type, *float_type, *char_type, *void_type, *string_type;
extern A_LITERAL literal_table[];
extern int literal_no;

BOOLEAN isFloatType(A_TYPE *);
BOOLEAN isArrayType(A_TYPE *);

void gen_program(A_NODE *);
void gen_declaration_list(A_ID *);
void gen_declaration(A_ID *);
void gen_expression(A_NODE *);
void gen_statement(A_NODE *, int, int);
void gen_statement_list(A_NODE *, int, int);
void gen_expression_left(A_NODE *);
void gen_arg_expression(A_NODE *);
void gen_literal_table(); 

void gen_code_i(OPCODE, int, int);
void gen_code_f(OPCODE, int, float);
void gen_code_s(OPCODE, int, char *);
void gen_code_l(OPCODE, int, int);
void gen_label_number(int);
void gen_label_name(char *);
int get_label();

int label_no = 0;

int is_valid_node(void *ptr) {
    if (!ptr) return 0;
    if ((unsigned long)ptr < 0x1000) return 0;
    return 1;
}

void code_generation(A_NODE *node) {
    if (fout == NULL) {
        printf("[FATAL] Output file not opened!\n");
        exit(1);
    }
    if (node) gen_program(node);
}

// [FIXED] Remove extra quotes in output format
void gen_literal_table() {
    int i;
    for (i = 1; i <= literal_no; i++) {
        if (literal_table[i].addr == 0) continue; 
        if (literal_table[i].type->kind == T_POINTER) { 
             // value.s likely already has quotes, so simply print %s without extra quotes
             fprintf(fout, "\tSTR %d, %s\n", literal_table[i].addr, literal_table[i].value.s);
        }
    }
}

void gen_program(A_NODE *node) {
    switch (node->name) {
    case N_PROGRAM:
        gen_code_i(INT, 0, node->value); 
        gen_code_s(SUP, 0, "main");
        gen_code_i(RET, 0, 0);
        if (node->clink) gen_declaration_list((A_ID*)node->clink);
        gen_literal_table();
        break;
    default:
        break;
    }
}

void gen_declaration_list(A_ID *id) {
    while (is_valid_node(id)) {
        gen_declaration(id);
        id = id->link;
    }
}

void gen_declaration(A_ID *id) {
    if (!is_valid_node(id) || !is_valid_node(id->type)) return;

    if (id->kind == ID_VAR && id->init) {
        if (id->type->kind == T_INT) 
            gen_code_i(LITI, 0, (int)(long)id->init);
        else if (id->type->kind == T_FLOAT) 
            gen_code_i(LITI, 0, (int)(long)id->init); 
    }
    else if (id->kind == ID_FUNC) {
        if (id->type->expr) {
            printf("[INFO] Generating function: %s\n", id->name);
            gen_label_name(id->name);
            gen_code_i(INT, 0, id->type->local_var_size);
            if (is_valid_node(id->type->expr)) 
                gen_statement(id->type->expr, 0, 0);
            gen_code_i(RET, 0, 0);
        }
    }
}

void gen_statement(A_NODE *node, int cont_label, int break_label) {
    if (!is_valid_node(node)) return;

    int l1, l2, l3;
    switch (node->name) {
    case N_STMT_IF:
        l1 = get_label();
        gen_expression(node->llink);
        gen_code_l(JPC, 0, l1);
        gen_statement(node->rlink, cont_label, break_label);
        gen_label_number(l1);
        break;
    case N_STMT_IF_ELSE:
        l1 = get_label(); l2 = get_label();
        gen_expression(node->llink);
        gen_code_l(JPC, 0, l1);
        gen_statement(node->clink, cont_label, break_label);
        gen_code_l(JMP, 0, l2);
        gen_label_number(l1);
        gen_statement(node->rlink, cont_label, break_label);
        gen_label_number(l2);
        break;
    case N_STMT_WHILE:
        l1 = get_label(); l2 = get_label();
        gen_label_number(l1);
        gen_expression(node->llink);
        gen_code_l(JPC, 0, l2);
        gen_statement(node->rlink, l1, l2);
        gen_code_l(JMP, 0, l1);
        gen_label_number(l2);
        break;
    case N_STMT_FOR: {
        A_NODE *n = node->llink;
        l1 = get_label(); l2 = get_label(); l3 = get_label();
        if (n->llink) gen_expression(n->llink); 
        gen_label_number(l1); 
        if (n->clink) { 
            gen_expression(n->clink); 
            gen_code_l(JPC, 0, l2);
        } 
        gen_statement(node->rlink, l3, l2); 
        gen_label_number(l3); 
        if (n->rlink) gen_expression(n->rlink); 
        gen_code_l(JMP, 0, l1); 
        gen_label_number(l2); 
        break;
    }
    case N_STMT_COMPOUND:
        if (node->rlink) gen_statement_list(node->rlink, cont_label, break_label);
        break;
    case N_STMT_RETURN:
        if (node->clink) gen_expression(node->clink);
        gen_code_i(RET, 0, 0);
        break;
    case N_STMT_EXPRESSION:
        if (node->clink) gen_expression(node->clink);
        break;
    default:
        break;
    }
}

void gen_statement_list(A_NODE *node, int cont_label, int break_label) {
    if (!is_valid_node(node)) return;
    if (node->name == N_STMT_LIST) {
        gen_statement(node->llink, cont_label, break_label);
        gen_statement_list(node->rlink, cont_label, break_label);
    }
}

void gen_expression(A_NODE *node) {
    if (!is_valid_node(node)) return;
    A_ID *id;
    A_TYPE *t;
    int i;

    switch (node->name) {
    case N_EXP_IDENT:
        id = (A_ID*)node->clink;
        if (!is_valid_node(id) || !is_valid_node(id->type)) return;
        t = id->type;
        switch (id->kind) {
        case ID_VAR: case ID_PARM:
            if (t->kind == T_ARRAY || t->kind == T_STRUCT) 
                gen_code_i(LDA, id->level, id->address);
            else 
                gen_code_i(LOD, id->level, id->address);
            break;
        case ID_ENUM_LITERAL: 
            gen_code_i(LITI, 0, (int)(long)id->init); 
            break;
        }
        break;

    case N_EXP_INT_CONST: 
        gen_code_i(LITI, 0, (int)(long)node->clink); 
        break;
    case N_EXP_FLOAT_CONST: 
        i = (int)(long)node->clink; 
        gen_code_i(LOD, 0, i);
        break;
    case N_EXP_CHAR_CONST: 
        gen_code_i(LITI, 0, (int)(long)node->clink); 
        break;
    
    // [FIXED] Use actual address from literal table, not the index
    case N_EXP_STRING_LITERAL: 
        i = (int)(long)node->clink; 
        if (i > 0 && i <= literal_no) {
            gen_code_i(LDA, 0, literal_table[i].addr);
        } else {
            // Fallback for safety
            gen_code_i(LDA, 0, 0);
        }
        break;

    case N_EXP_ARRAY:
        gen_expression(node->llink);
        gen_expression(node->rlink);
        if (node->type && node->type->size > 1) { 
            gen_code_i(LITI, 0, node->type->size); 
            gen_code_i(MULI, 0, 0); 
        }
        gen_code_i(ADDI, 0, 0); 
        if (node->type && !isArrayType(node->type)) 
            gen_code_i(LDI, 0, 0); 
        break;

    case N_EXP_FUNCTION_CALL:
        if (!is_valid_node(node->llink) || !is_valid_node(node->llink->type)) return;
        t = node->llink->type;
        
        i = t->element_type->size;
        if (i % 4) i = i / 4 * 4 + 4;
        gen_code_i(INT, 0, i);
        
        gen_arg_expression(node->rlink);

        A_NODE *func_node = node->llink;
        while (func_node) {
            if (func_node->name == N_EXP_IDENT) break;
            else if (func_node->name == N_EXP_AMP) func_node = func_node->clink;
            else if (func_node->name == N_EXP_CAST) func_node = func_node->rlink;
            else break;
        }

        if (func_node && func_node->name == N_EXP_IDENT && func_node->clink) {
             A_ID *fid = (A_ID*)func_node->clink;
             if (is_valid_node(fid) && fid->name) {
                 if (fid->kind == ID_FUNC && fid->type->expr == NULL) {
                     gen_code_i(CAL, 0, fid->address);
                 } else {
                     gen_code_s(CAL, 0, fid->name);
                 }
             }
        }
        break;

    case N_EXP_ASSIGN:
        gen_expression_left(node->llink);
        gen_expression(node->rlink);
        gen_code_i(STO, 0, 0);
        break;
        
    case N_EXP_POST_INC:
    case N_EXP_PRE_INC:
        gen_expression_left(node->clink);
        gen_expression(node->clink);
        gen_code_i(LITI, 0, 1);
        gen_code_i(ADDI, 0, 0);
        gen_code_i(STO, 0, 0);
        break;
    case N_EXP_POST_DEC:
    case N_EXP_PRE_DEC:
        gen_expression_left(node->clink);
        gen_expression(node->clink);
        gen_code_i(LITI, 0, 1);
        gen_code_i(SUBI, 0, 0);
        gen_code_i(STO, 0, 0);
        break;

    default:
        if (node->llink) gen_expression(node->llink);
        if (node->name != N_EXP_CAST && node->rlink) gen_expression(node->rlink);

        switch(node->name) {
            case N_EXP_ADD: 
                if (node->type && isFloatType(node->type)) gen_code_i(ADDF,0,0); 
                else gen_code_i(ADDI,0,0); break;
            case N_EXP_SUB: 
                if (node->type && isFloatType(node->type)) gen_code_i(SUBF,0,0); 
                else gen_code_i(SUBI,0,0); break;
            case N_EXP_MUL: 
                if (node->type && isFloatType(node->type)) gen_code_i(MULF,0,0); 
                else gen_code_i(MULI,0,0); break;
            case N_EXP_DIV: 
                if (node->type && isFloatType(node->type)) gen_code_i(DIVF,0,0); 
                else gen_code_i(DIVI,0,0); break;
            
            case N_EXP_LSS: 
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(LSSF,0,0);
                else gen_code_i(LSSI,0,0); break;
            case N_EXP_GTR: 
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(GTRF,0,0);
                else gen_code_i(GTRI,0,0); break;
            case N_EXP_EQL: 
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(EQLF,0,0);
                else gen_code_i(EQLI,0,0); break;
            case N_EXP_NEQ:
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(NEQF,0,0);
                else gen_code_i(NEQI,0,0); break;
            case N_EXP_GEQ:
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(GEQF,0,0);
                else gen_code_i(GEQI,0,0); break;
            case N_EXP_LEQ:
                if (node->llink && node->llink->type && isFloatType(node->llink->type)) gen_code_i(LEQF,0,0);
                else gen_code_i(LEQI,0,0); break;
            
            case N_EXP_CAST:
                if (node->rlink) gen_expression(node->rlink);
                if (node->rlink && node->rlink->type && node->type) {
                    if (node->type->kind == T_FLOAT && node->rlink->type->kind != T_FLOAT) 
                        gen_code_i(CVTF, 0, 0); 
                    else if (node->type->kind != T_FLOAT && node->rlink->type->kind == T_FLOAT) 
                        gen_code_i(CVTI, 0, 0); 
                } 
                break;
        }
    }
}

void gen_expression_left(A_NODE *node) {
    if (!is_valid_node(node)) return;
    A_ID *id;
    switch (node->name) {
    case N_EXP_IDENT:
        id = (A_ID*)node->clink;
        if (is_valid_node(id)) gen_code_i(LDA, id->level, id->address);
        break;
    case N_EXP_ARRAY:
        gen_expression(node->llink); 
        gen_expression(node->rlink); 
        if (node->type && node->type->size > 1) { 
            gen_code_i(LITI, 0, node->type->size); 
            gen_code_i(MULI, 0, 0); 
        }
        gen_code_i(ADDI, 0, 0);
        break;
    }
}

void gen_arg_expression(A_NODE *node) {
    if (!is_valid_node(node)) return;
    switch (node->name) {
    case N_ARG_LIST:
        gen_expression(node->llink);
        gen_arg_expression(node->rlink);
        break;
    case N_ARG_LIST_NIL: break;
    }
}

void gen_code_i(OPCODE op, int l, int a) { if(fout) fprintf(fout, "\t%s %d, %d\n", opcode_name[op], l, a); }
void gen_code_f(OPCODE op, int l, float a) { if(fout) fprintf(fout, "\t%s %d, %f\n", opcode_name[op], l, a); }
void gen_code_s(OPCODE op, int l, char *a) { if(fout) fprintf(fout, "\t%s %d, %s\n", opcode_name[op], l, a); }
void gen_code_l(OPCODE op, int l, int a) { if(fout) fprintf(fout, "\t%s %d, L%d\n", opcode_name[op], l, a); }
void gen_label_number(int i) { if(fout) fprintf(fout, "L%d:\n", i); }
void gen_label_name(char *s) { if(fout) fprintf(fout, "%s:\n", s); }
int get_label() { return label_no++; }
