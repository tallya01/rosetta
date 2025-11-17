#ifndef AST_H
#define AST_H

#include "tabela_simbolos.h"

typedef enum {
    NO_PROGRAMA,
    NO_DECL_FUNC,
    NO_DECL_VAR,
    NO_BLOCO,
    NO_SE,
    NO_ENQUANTO,
    NO_ATRIBUICAO,
    NO_RETORNE,
    NO_LEIA,
    NO_ESCREVA,
    NO_SOMA, NO_SUB, NO_MULT, NO_DIV,
    NO_IGUAL, NO_DIF, NO_MAIOR, NO_MENOR, NO_MAIOR_IGUAL, NO_MENOR_IGUAL,
    NO_E, NO_OU, NO_NEG,
    NO_ID,
    NO_INT_CONST,
    NO_CAR_CONST,
    NO_CHAMADA_FUNC,
    NO_LISTA,      /* Para sequências de comandos ou declarações */
    NO_NOVALINHA,
    NO_NULO        /* Para nós vazios */
} TipoNo;

typedef struct ASTNode {
    TipoNo tipo;
    int linha;              
    char *valor_lexico;     /* Para IDs e Strings */
    int valor_int;          /* Para constantes inteiras */
    Tipo tipo_dado;         /* TIPO_INT, TIPO_CAR (para análise semântica) */
    
    struct ASTNode *filho[3]; /* Até 3 filhos (ex: IF expr ENTAO cmd SENAO cmd) */
    struct ASTNode *prox;     /* Para listas encadeadas de comandos/decls */
} ASTNode;

ASTNode* criar_no(TipoNo tipo, ASTNode* f1, ASTNode* f2, ASTNode* f3, int linha);
ASTNode* criar_folha_id(char* lexema, int linha);
ASTNode* criar_folha_int(int valor, int linha);
ASTNode* criar_folha_car(char* lexema, int linha);
void imprimir_ast(ASTNode* no, int nivel);
void liberar_ast(ASTNode* no);

#endif