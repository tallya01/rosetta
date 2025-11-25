#ifndef GERADOR_CODIGO_H
#define GERADOR_CODIGO_H

#include <stdio.h>
#include "ast.h"

/*
 * Função principal para gerar o código assembly MIPS.
 * Recebe a raiz da AST e o arquivo onde o código será escrito.
 */
void gerar_codigo(ASTNode* raiz, FILE* saida, ScopeStack* pilha);

#endif