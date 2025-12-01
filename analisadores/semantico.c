/* analise_semantica.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantico.h"

// Variáveis globais para controle interno da análise
static int g_erros_semanticos = 0;
static Tipo g_tipo_retorno_esperado = TIPO_INT; // Para validar 'retorne'
static int g_dentro_de_funcao = 0; // Flag para saber se estamos dentro de uma função

// Para imprimir erros com linha
void erro_semantico(int linha, const char* mensagem) {
    fprintf(stderr, "ERRO SEMANTICO (Linha %d): %s\n", linha, mensagem);
    g_erros_semanticos++;
}

// Auxiliar para mensagens de erro
const char* nome_tipo(Tipo t) {
    if (t == TIPO_INT) return "int";
    if (t == TIPO_CAR) return "car";
    return "indefinido";
}

// Funções internas para percorrer a árvore recursivamente
void analisar_no(ASTNode* no, ScopeStack* pilha);
Tipo inferir_tipo_expressao(ASTNode* no, ScopeStack* pilha);

/* --- Função Principal --- */
int verificar_semantica(ASTNode* raiz, ScopeStack* pilha_semantica) {
    g_erros_semanticos = 0;
    
    printf("\n--- Iniciando Analise Semantica ---\n");
    
    // Começa a percorrer a árvore
    if (raiz != NULL) {
        analisar_no(raiz, pilha_semantica);
    }
    
    if (g_erros_semanticos == 0) {
        printf("Analise semantica concluida com SUCESSO.\n");
        return 0;
    } else {
        printf("Analise semantica concluida com %d ERROS.\n", g_erros_semanticos);
        return 1;
    }
}

void analisar_no(ASTNode* no, ScopeStack* pilha) {
    if (no == NULL) return;

    switch (no->tipo) {
        case NO_PROGRAMA:
            analisar_no(no->filho[0], pilha); // DeclFuncVar
            analisar_no(no->filho[1], pilha); // DeclProg
            break;

        case NO_DECL_VAR:
        {
            ASTNode* atual = no;
            while (atual != NULL && atual->tipo == NO_DECL_VAR) {
                ASTNode* id_node = atual->filho[0]; // NO_ID
                
                // Tenta inserir. Se falhar, é redeclaração no mesmo escopo.
                if (inserir_variavel(pilha, id_node->valor_lexico, atual->tipo_dado, 0) == NULL) {
                    char msg[100];
                    sprintf(msg, "Variavel '%s' ja declarada neste escopo.", id_node->valor_lexico);
                    erro_semantico(atual->linha, msg);
                }
                atual = atual->prox;
            }
            // Continua declarações globais (caso a próxima seja função, o loop para sem concluir tudo)
            if (atual != NULL)
                analisar_no(atual, pilha);
        }
        break;

        case NO_DECL_FUNC:
        {
            ASTNode* id_func = no->filho[0];
            Symbol* sym_func = inserir_funcao(pilha, id_func->valor_lexico, no->tipo_dado, 0);
            
            if (sym_func == NULL) {
                char msg[100];
                sprintf(msg, "Funcao '%s' ja declarada.", id_func->valor_lexico);
                erro_semantico(no->linha, msg);
            }

            // Contexto para validação de retorno
            Tipo tipo_anterior = g_tipo_retorno_esperado;
            int flag_anterior = g_dentro_de_funcao;
            g_tipo_retorno_esperado = no->tipo_dado;
            g_dentro_de_funcao = 1;

            criar_novo_escopo(pilha);

            // Processamento dos parâmetros
            ASTNode* params = no->filho[1];
            if (params != NULL) {
                analisar_no(params, pilha);
                
                if (sym_func != NULL) {
                    ASTNode* p = params;
                    while(p != NULL) {
                        ASTNode* p_id = p->filho[0];
                        adicionar_info_parametro(sym_func, p_id->valor_lexico, p->tipo_dado);
                        sym_func->num_args++;
                        p = p->prox;
                    }
                }
            }
            
            // Processamento do corpo da função
            ASTNode* bloco_corpo = no->filho[2];
            if (bloco_corpo != NULL && bloco_corpo->tipo == NO_BLOCO) {
                // Analisa variáveis locais
                analisar_no(bloco_corpo->filho[0], pilha);
                // Analisa comandos
                analisar_no(bloco_corpo->filho[1], pilha);
            }

            remover_escopo_atual(pilha);
            
            // Restaura contexto anterior
            g_tipo_retorno_esperado = tipo_anterior;
            g_dentro_de_funcao = flag_anterior;
        }
        break;

        case NO_BLOCO:
            criar_novo_escopo(pilha);
            analisar_no(no->filho[0], pilha); // Analisa variáveis locais
            analisar_no(no->filho[1], pilha); // Analisa comandos
            remover_escopo_atual(pilha);
            break;

        case NO_ATRIBUICAO:
        {
            ASTNode* id_node = no->filho[0];
            ASTNode* expr = no->filho[1];

            Symbol* sym = pesquisar_simbolo(pilha, id_node->valor_lexico);
            if (sym == NULL) {
                char msg[100];
                sprintf(msg, "Variavel '%s' nao declarada.", id_node->valor_lexico);
                erro_semantico(no->linha, msg);
            } else {
                Tipo t_expr = inferir_tipo_expressao(expr, pilha);
                if (t_expr != sym->tipo) {
                    char msg[100];
                    sprintf(msg, "Atribuicao incompativel: Variavel '%s' eh %s, mas expressao eh %s.",
                            id_node->valor_lexico, nome_tipo(sym->tipo), nome_tipo(t_expr));
                    erro_semantico(no->linha, msg);
                }
            }
        }
        break;

        case NO_SE:
        case NO_ENQUANTO:
        {
            ASTNode* expr = no->filho[0];
            // Avalia expressão condicional
            inferir_tipo_expressao(expr, pilha);
            
            analisar_no(no->filho[1], pilha); /* Bloco Then */
            if (no->filho[2] != NULL) {
                analisar_no(no->filho[2], pilha); /* Bloco Else */
            }
        }
        break;
        
        case NO_ESCREVA:
            if (no->filho[0]->tipo == NO_CADEIA_CAR) {
                // String literal, não há tipo para inferir
            } else if (no->filho[0]->tipo != NO_ID || pesquisar_simbolo(pilha, no->filho[0]->valor_lexico) != NULL) {
                 inferir_tipo_expressao(no->filho[0], pilha);
            }
            break;

        case NO_RETORNE:
        {
            if (!g_dentro_de_funcao) {
                erro_semantico(no->linha, "'retorne' utilizado fora de funcao.");
            } else {
                Tipo t_expr = inferir_tipo_expressao(no->filho[0], pilha);
                if (t_expr != g_tipo_retorno_esperado) {
                    char msg[100];
                    sprintf(msg, "Tipo de retorno invalido. Esperado %s, encontrado %s.",
                            nome_tipo(g_tipo_retorno_esperado), nome_tipo(t_expr));
                    erro_semantico(no->linha, msg);
                }
            }
        }
        break;
        
        // Chamada recursiva padrão para nós que são apenas containers ou expressões isoladas como comando
        case NO_LISTA: 
        case NO_CHAMADA_FUNC:
             inferir_tipo_expressao(no, pilha);
             break;

        default:
            // Visita genérica aos filhos se não houver tratamento específico para o nó
            if (no->filho[0]) analisar_no(no->filho[0], pilha);
            if (no->filho[1]) analisar_no(no->filho[1], pilha);
            if (no->filho[2]) analisar_no(no->filho[2], pilha);
            if (no->prox) analisar_no(no->prox, pilha);
            break;
    }
    
    // Processa comandos adicionais da lista, caso existam
    if (no->prox != NULL && 
        /* A lista de declarações globais (DeclFuncVar) é uma lista encadeada via 'prox'.
         * A exceção para NO_DECL_VAR é para evitar re-processar listas de variáveis
         * que já são tratadas em um loop interno no seu próprio case. */
        no->tipo != NO_DECL_VAR) {
        
        analisar_no(no->prox, pilha);
    }
}

/* --- Inferência e Validação de Tipos em Expressões --- */
Tipo inferir_tipo_expressao(ASTNode* no, ScopeStack* pilha) {
    if (no == NULL) return TIPO_INT; /* Fallback seguro */

    switch (no->tipo) {
        case NO_INT_CONST:
            no->tipo_dado = TIPO_INT;
            return TIPO_INT;

        case NO_CAR_CONST:
            no->tipo_dado = TIPO_CAR;
            return TIPO_CAR;

        case NO_ID:
        {
            Symbol* sym = pesquisar_simbolo(pilha, no->valor_lexico);
            if (sym == NULL) {
                char msg[100];
                sprintf(msg, "Identificador '%s' nao declarado.", no->valor_lexico);
                erro_semantico(no->linha, msg);
                return TIPO_INT; /* Assume INT para evitar erros em cascata */
            }
            no->tipo_dado = sym->tipo;
            return sym->tipo;
        }

        case NO_CHAMADA_FUNC:
        {
            Symbol* func = pesquisar_simbolo(pilha, no->filho[0]->valor_lexico);
            if (func == NULL) {
                char msg[100];
                sprintf(msg, "Funcao '%s' nao declarada.", no->filho[0]->valor_lexico);
                erro_semantico(no->linha, msg);
                return TIPO_INT;
            }
            
            ASTNode* arg = no->filho[1];
            int count = 0;
            
            ParametroInfo* param_def = func->params_info;
            
            while (arg != NULL) {
                Tipo t_arg = inferir_tipo_expressao(arg, pilha);
                count++;
                
                if (param_def != NULL) {
                    if (t_arg != param_def->tipo) {
                        char msg[150];
                        sprintf(msg, "Argumento %d da funcao '%s' incompativel. Esperado %s, dado %s.",
                                count, func->nome, nome_tipo(param_def->tipo), nome_tipo(t_arg));
                        erro_semantico(no->linha, msg);
                    }
                    param_def = param_def->proximo;
                } else {
                    // Mais argumentos do que parâmetros declarados
                    // Será checado abaixo no count != num_args
                }
                arg = arg->prox;
            }

            if (count != func->num_args) {
                char msg[100];
                sprintf(msg, "Numero incorreto de argumentos para '%s'. Esperado %d, dado %d.",
                        func->nome, func->num_args, count);
                erro_semantico(no->linha, msg);
            }

            no->tipo_dado = func->tipo;
            return func->tipo;
        }

        // Operações Aritméticas: Exigem INT e retornam INT
        case NO_SOMA:
        case NO_SUB:
        case NO_MULT:
        case NO_DIV:
        {
            Tipo t1 = inferir_tipo_expressao(no->filho[0], pilha);
            Tipo t2 = TIPO_INT; // Assume INT para checagem unária
            ASTNode* op2 = no->filho[1];
            
            if (op2 != NULL) { // Se for operação binária, confere o segundo operando
                t2 = inferir_tipo_expressao(op2, pilha);
            }

            if (t1 != TIPO_INT || t2 != TIPO_INT) {
                erro_semantico(no->linha, "Operacoes aritmeticas requerem operandos do tipo INT.");
            }
            
            no->tipo_dado = TIPO_INT;
            return TIPO_INT;
        }

        // Operações Relacionais/Lógicas: Operandos devem ser iguais, Retorna INT (pseudo-bool)
        case NO_IGUAL:
        case NO_DIF:
        case NO_MAIOR:
        case NO_MENOR:
        case NO_MAIOR_IGUAL:
        case NO_MENOR_IGUAL:
        {
            Tipo t1 = inferir_tipo_expressao(no->filho[0], pilha);
            Tipo t2 = inferir_tipo_expressao(no->filho[1], pilha);
            
            if (t1 != t2) {
                erro_semantico(no->linha, "Comparacao entre tipos diferentes.");
            }
            
            no->tipo_dado = TIPO_INT;
            return TIPO_INT;
        }

        case NO_E:
        case NO_OU:
        case NO_NEG:
        {
             /* Lógicos operam sobre INT (verdadeiro/falso) */
             inferir_tipo_expressao(no->filho[0], pilha);
             if (no->filho[1]) inferir_tipo_expressao(no->filho[1], pilha);
             
             no->tipo_dado = TIPO_INT;
             return TIPO_INT;
        }

        default:
            return TIPO_INT;
    }
}