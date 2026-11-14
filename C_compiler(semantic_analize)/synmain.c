#include <stdio.h>
#include <stdlib.h>
#include "type.h"
#include "y.tab.h"

/* External variables */
extern int syntax_err;
extern int semantic_err; // Semantic error count (defined in synfun.c)
extern A_NODE *root;
extern FILE *yyin;

/* Function prototypes */
void initialize();
void print_sem_ast(A_NODE *);      // Now prints the semantic tree
void semantic_analysis(A_NODE *); // Performs semantic analysis
int yyparse();

/**
 * @brief Main driver for the compiler.
 * Follows the assignment specification:
 * initialize -> parse -> check syntax err -> semantic analysis -> check semantic err -> print tree
 */
void main(int argc, char *argv[]) 
{
    // 1. Handle Input Source (File vs Stdin)
    if (argc > 1) {
        if ((yyin = fopen(argv[1], "r")) == NULL) {
            fprintf(stderr, "Error: Can not open file %s\n", argv[1]);
            exit(1);
        }
        printf("Start Syntax & Semantic Analysis from file: %s\n\n", argv[1]);
    } else {
        printf("Start Syntax & Semantic Analysis from Standard Input\n");
        printf("... Type your code and press Ctrl+D (EOF) to parse ...\n\n");
    }

    // 2. Initialize Symbol Table
    initialize();

    // 3. Syntax Analysis (Parsing)
    yyparse();
    if (syntax_err) {
        printf("\n...Syntax analysis failed with %d errors.\n", syntax_err);
        exit(1);
    }

    // 4. Semantic Analysis
    semantic_analysis(root);
    if (semantic_err) {
        printf("\n...Semantic analysis failed with %d errors.\n", semantic_err);
        exit(1);
    }

    // 5. Print Semantic Tree
    printf("\n...Semantic analysis successful.\n");
    print_sem_ast(root); // Calls the updated printer
}
