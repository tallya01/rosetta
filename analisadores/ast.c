#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode* criar_no(TipoNo tipo, ASTNode* f1, ASTNode* f2, ASTNode* f3, int linha) {
    ASTNode* no = (ASTNode*) malloc(sizeof(ASTNode));
    if (no != NULL) {
        no->tipo = tipo;
        no->linha = linha; 
        no->valor_lexico = NULL;
        no->filho[0] = f1;
        no->filho[1] = f2;
        no->filho[2] = f3;
        no->prox = NULL;
    }
    return no;
}

ASTNode* criar_folha_id(char* lexema, int linha) {
    ASTNode* no = criar_no(NO_ID, NULL, NULL, NULL, linha);
    if (no != NULL && lexema != NULL) {
        no->valor_lexico = strdup(lexema); /* Copia a string */
    }
    return no;
}

ASTNode* criar_folha_str(char* lexema, int linha) {
    ASTNode* no = criar_no(NO_CADEIA_CAR, NULL, NULL, NULL, linha);
    if (no != NULL && lexema != NULL) {
        no->valor_lexico = strdup(lexema); /* Copia a string */
    }
    return no;
}

ASTNode* criar_folha_int(int valor, int linha) {
    ASTNode* no = criar_no(NO_INT_CONST, NULL, NULL, NULL, linha);
    if (no != NULL) {
        no->valor_int = valor;
    }
    return no;
}

ASTNode* criar_folha_car(char* lexema, int linha) {
    ASTNode* no = criar_no(NO_CAR_CONST, NULL, NULL, NULL, linha);
    if (no != NULL && lexema != NULL) {
        no->valor_lexico = strdup(lexema);
    }
    return no;
}

/* Função auxiliar para imprimir a AST (visualização) */
void imprimir_ast(ASTNode* no, int nivel) {
    if (no == NULL) return;

    for (int i = 0; i < nivel; i++) printf("  ");
    
    switch(no->tipo) {
        case NO_PROGRAMA: printf("PROGRAMA\n"); break;
        case NO_DECL_FUNC: printf("FUNC_DECL\n"); break;
        case NO_DECL_VAR: printf("VAR_DECL\n"); break;
        case NO_BLOCO: printf("BLOCO\n"); break;
        case NO_SE: printf("SE\n"); break;
        case NO_ENQUANTO: printf("ENQUANTO\n"); break;
        case NO_ATRIBUICAO: printf("ATRIBUICAO\n"); break;
        case NO_LEIA: printf("LEIA\n"); break;
        case NO_ESCREVA: printf("ESCREVA\n"); break;
        case NO_SOMA: printf("SOMA (+)\n"); break;
        case NO_SUB: printf("SUB (-)\n"); break;
        case NO_MULT: printf("MULT (*)\n"); break;
        case NO_DIV: printf("DIV (/)\n"); break;
        case NO_ID: printf("ID: %s\n", no->valor_lexico); break;
        case NO_INT_CONST: printf("INT: %d\n", no->valor_int); break;
        case NO_CAR_CONST: printf("CAR: %s\n", no->valor_lexico); break;
        case NO_CHAMADA_FUNC: printf("CHAMADA_FUNC\n"); break;
        case NO_NOVALINHA: printf("NOVA_LINHA\n"); break;
        case NO_RETORNE: printf("RETORNE\n"); break;
        case NO_LISTA: printf("LISTA\n"); break;
        case NO_NULO: printf("NULO\n"); break;
        case NO_CADEIA_CAR: printf("CADEIA_CAR: %s\n", no->valor_lexico); break;
        default: printf("NO_TIPO_%d\n", no->tipo);
    }

    imprimir_ast(no->filho[0], nivel + 1);
    imprimir_ast(no->filho[1], nivel + 1);
    imprimir_ast(no->filho[2], nivel + 1);
    
    if (no->prox != NULL) {
        imprimir_ast(no->prox, nivel);
    }
}

void liberar_ast(ASTNode* no) {
    if (no == NULL) return;
    liberar_ast(no->filho[0]);
    liberar_ast(no->filho[1]);
    liberar_ast(no->filho[2]);
    liberar_ast(no->prox);
    if (no->valor_lexico) free(no->valor_lexico);
    free(no);
}
