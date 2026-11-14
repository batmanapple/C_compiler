// compiler.c (main file)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "y.tab.h"

extern int syntax_err;
extern int semantic_err;
extern A_NODE *root;
extern FILE *yyin;

// file pointer (Used in codegen.c)
FILE *fout;

// prototype
void initialize();
void print_sem_ast(A_NODE *);
void semantic_analysis(A_NODE *);
void code_generation(A_NODE *);
int yyparse();

void main(int argc, char *argv[])
{
    // 1. check for source file
    if (argc < 2) {
        printf("source file not given\n");
        exit(1);
    }

    // 2. a.asm(output file) setting logic
    if (strcmp(argv[1], "-o") == 0) {
        if (argc > 3) {
            if ((fout = fopen(argv[2], "w")) == NULL) {
                printf("can not open output file: %s\n", argv[2]);
                exit(1);
            }
        } else {
            printf("out file not given\n");
            exit(1);
        }
    } else if (argc == 2) {
        if ((fout = fopen("a.asm", "w")) == NULL) {
            printf("can not open output file: a.asm\n");
            exit(1);
        }
    } else if ((fout = fopen("a.asm", "w")) == NULL) {
        printf("can not open output file: a.asm\n");
        exit(1);
    }

    // 3. opening source file
    if ((yyin = fopen(argv[argc - 1], "r")) == NULL) {
        printf("can not open input file: %s\n", argv[argc - 1]);
        exit(1);
    }

    // 4. Syntax Analysis
    printf("\nstart syntax analysis\n");
    initialize();
    yyparse();
    if (syntax_err) exit(1);

    // 5. Semantic Analysis
    printf("\nstart semantic analysis\n");
    semantic_analysis(root);
    if (semantic_err) exit(1);

    // 6. Code Generation
    printf("start code generation\n");
    code_generation(root);

    exit(0);
}
