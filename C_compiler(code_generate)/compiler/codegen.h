#ifndef CODEGEN_H
#define CODEGEN_H

typedef enum op {
    OP_NULL, LOD, LDX, LDXB, LDA, LITI, 
    STO, STOB, STX, STXB,
    SUBI, SUBF, DIVI, DIVF, ADDI, ADDF, OFFSET, MULI, MULF, MOD, 
    LSSI, LSSF, GTRI, GTRF, LEQI, LEQF, GEQI, GEQF, NEQI, NEQF, EQLI, EQLF, 
    NOT, OR, AND, CVTI, CVTF, 
    JPC, JPCR, JMP, JPT, JPTR, INT, INCI, INCF, DECI, DECF, SUP, CAL, ADDR,
    RET, MINUSI, MINUSF, LDI, LDIB, POP
} OPCODE;

static char *opcode_name[] = {
    "OP_NULL", "LOD", "LDX", "LDXB", "LDA", "LITI", 
    "STO", "STOB", "STX", "STXB",
    "SUBI", "SUBF", "DIVI", "DIVF", "ADDI", "ADDF", "OFFSET", "MULI", "MULF", "MOD", 
    "LSSI", "LSSF", "GTRI", "GTRF", "LEQI", "LEQF", "GEQI", "GEQF", "NEQI", "NEQF", "EQLI", "EQLF", 
    "NOT", "OR", "AND", "CVTI", "CVTF", 
    "JPC", "JPCR", "JMP", "JPT", "JPTR", "INT", "INCI", "INCF", "DECI", "DECF", "SUP", "CAL", "ADDR",
    "RET", "MINUSI", "MINUSF", "LDI", "LDIB", "POP"
};

#endif
