/* semantico.h */
#ifndef SEMANTICO_H
#define SEMANTICO_H

#include "tabela_simbolos.h"
#include "ast.h"

/* * Função principal que inicia a análise semântica percorrendo a AST.
 * Retorna 1 se houver erros semânticos, 0 se sucesso.
 */
int verificar_semantica(ASTNode* raiz);

#endif