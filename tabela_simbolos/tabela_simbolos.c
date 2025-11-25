#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela_simbolos.h"

// a - Iniciar a pilha de tabela de símbolos
ScopeStack* iniciar_pilha_tabela_simbolos() {
    ScopeStack* pilha = (ScopeStack*) malloc(sizeof(ScopeStack));
    if (pilha) {
        pilha->topo = NULL;
    }
    // Cria o escopo global inicial
    criar_novo_escopo(pilha);
    return pilha;
}

// b - Criar uma nova tabela de símbolos (novo escopo) e empilhá-la
void criar_novo_escopo(ScopeStack* pilha) {
    SymbolTable* novo_escopo = (SymbolTable*) malloc(sizeof(SymbolTable));
    if (!novo_escopo) {
        perror("Falha ao alocar memória para novo escopo");
        exit(EXIT_FAILURE);
    }
    novo_escopo->head = NULL;
    novo_escopo->proximo = pilha->topo;
    pilha->topo = novo_escopo;
}

// d - Remover escopo atual
void remover_escopo_atual(ScopeStack* pilha) {
    if (!pilha || !pilha->topo) return;

    SymbolTable* escopo_a_remover = pilha->topo;
    pilha->topo = escopo_a_remover->proximo;

    // Liberar todos os símbolos do escopo removido
    Symbol* atual = escopo_a_remover->head;
    while (atual) {
        Symbol* proximo = atual->proximo;
        free(atual->nome);
        // Se for função, liberar a lista de parâmetros
        if (atual->categoria == CAT_FUNCAO) {
            ParametroInfo* param_atual = atual->params_info;
            while (param_atual) {
                ParametroInfo* proximo_param = param_atual->proximo;
                free(param_atual->nome);
                free(param_atual);
                param_atual = proximo_param;
            }
        }
        free(atual);
        atual = proximo;
    }
    free(escopo_a_remover);
}

// Função auxiliar para criar um símbolo genérico
static Symbol* criar_simbolo(const char* nome, Categoria cat, Tipo tipo, int ordem) {
    Symbol* novo_simbolo = (Symbol*) malloc(sizeof(Symbol));
    if (!novo_simbolo) return NULL;
    
    novo_simbolo->nome = strdup(nome);
    if (!novo_simbolo->nome) {
        free(novo_simbolo);
        return NULL;
    }
    
    novo_simbolo->categoria = cat;
    novo_simbolo->tipo = tipo;
    novo_simbolo->ordem = ordem;
    novo_simbolo->num_args = 0;
    novo_simbolo->params_info = NULL;
    novo_simbolo->proximo = NULL;
    
    return novo_simbolo;
}

// Função auxiliar para inserir um símbolo no escopo atual
static Symbol* inserir_no_escopo_atual(ScopeStack* pilha, Symbol* novo_simbolo) {
    if (!pilha || !pilha->topo) {
        free(novo_simbolo->nome);
        free(novo_simbolo);
        return NULL;
    }
    
    // Verifica se o símbolo já existe no escopo atual
    Symbol* atual = pilha->topo->head;
    while(atual) {
        if (strcmp(atual->nome, novo_simbolo->nome) == 0) {
            // Símbolo já existe neste escopo
            free(novo_simbolo->nome);
            free(novo_simbolo);
            return NULL; 
        }
        atual = atual->proximo;
    }
    
    // Insere no início da lista
    novo_simbolo->proximo = pilha->topo->head;
    pilha->topo->head = novo_simbolo;
    
    return novo_simbolo;
}

// f - Inserir um nome de variável na tabela de símbolos atual
Symbol* inserir_variavel(ScopeStack* pilha, const char* nome, Tipo tipo, int ordem) {
    Symbol* novo_simbolo = criar_simbolo(nome, CAT_VARIAVEL, tipo, ordem);
    if (!novo_simbolo) return NULL;
    return inserir_no_escopo_atual(pilha, novo_simbolo);
}

// g - Inserir o nome de um parâmetro na tabela de símbolos atual
Symbol* inserir_parametro(ScopeStack* pilha, const char* nome, Tipo tipo, int ordem) {
    Symbol* novo_simbolo = criar_simbolo(nome, CAT_PARAMETRO, tipo, ordem);
    if (!novo_simbolo) return NULL;
    return inserir_no_escopo_atual(pilha, novo_simbolo);
}

// e - Inserir um nome de função na tabela de símbolos atual
Symbol* inserir_funcao(ScopeStack* pilha, const char* nome, Tipo tipo_retorno, int num_args) {
    Symbol* novo_simbolo = criar_simbolo(nome, CAT_FUNCAO, tipo_retorno, 0);
    if (!novo_simbolo) return NULL;
    
    novo_simbolo->num_args = num_args;
    // As informações dos parâmetros serão adicionadas depois
    return inserir_no_escopo_atual(pilha, novo_simbolo);
}

// Função auxiliar para e - conforme especificado no documento
void adicionar_info_parametro(Symbol* simbolo_funcao, const char* nome_param, Tipo tipo_param) {
    if (!simbolo_funcao || simbolo_funcao->categoria != CAT_FUNCAO) return;

    ParametroInfo* novo_param = (ParametroInfo*) malloc(sizeof(ParametroInfo));
    if (!novo_param) return;
    
    novo_param->nome = strdup(nome_param);
    novo_param->tipo = tipo_param;
    novo_param->proximo = NULL;

    // Insere no final da lista para manter a ordem
    if (simbolo_funcao->params_info == NULL) {
        simbolo_funcao->params_info = novo_param;
    } else {
        ParametroInfo* atual = simbolo_funcao->params_info;
        while (atual->proximo != NULL) {
            atual = atual->proximo;
        }
        atual->proximo = novo_param;
    }
}


// c - Pesquisar por um nome na pilha de tabelas de símbolos
Symbol* pesquisar_simbolo(ScopeStack* pilha, const char* nome) {
    if (!pilha) return NULL;

    SymbolTable* escopo_atual = pilha->topo;
    // Itera do escopo do topo em direção à base
    while (escopo_atual) {
        Symbol* simbolo_atual = escopo_atual->head;
        while (simbolo_atual) {
            if (strcmp(simbolo_atual->nome, nome) == 0) {
                return simbolo_atual; // Encontrado
            }
            simbolo_atual = simbolo_atual->proximo;
        }
        escopo_atual = escopo_atual->proximo;
    }
    
    return NULL; // Não encontrado
}

// h - Eliminar a pilha de tabelas de símbolos
void eliminar_pilha_tabelas(ScopeStack* pilha) {
    while (pilha && pilha->topo) {
        remover_escopo_atual(pilha);
    }
    free(pilha);
}

// Função auxiliar para imprimir a pilha (para teste)
void imprimir_pilha(ScopeStack* pilha) {
    if (!pilha) {
        printf("Pilha não inicializada.\n");
        return;
    }
    printf("================ PILHA DE TABELAS DE SÍMBOLOS ================\n");
    SymbolTable* escopo_atual = pilha->topo;
    int nivel_escopo = 0;
    while (escopo_atual) {
        printf("--- Escopo Nível %d ---\n", nivel_escopo);
        Symbol* simbolo_atual = escopo_atual->head;
        if (!simbolo_atual) {
            printf("  (vazio)\n");
        }
        while (simbolo_atual) {
            printf("  > Nome: %-10s | Cat: %-9s | Tipo: %-3s | Ordem: %d",
                   simbolo_atual->nome,
                   simbolo_atual->categoria == CAT_VARIAVEL ? "Variavel" : (simbolo_atual->categoria == CAT_PARAMETRO ? "Parametro" : "Funcao"),
                   simbolo_atual->tipo == TIPO_INT ? "int" : "car",
                   simbolo_atual->ordem);
            if (simbolo_atual->categoria == CAT_FUNCAO) {
                printf(" | Args: %d\n", simbolo_atual->num_args);
                ParametroInfo* p_info = simbolo_atual->params_info;
                while(p_info) {
                    printf("    -> Param: %-10s | Tipo: %s\n", p_info->nome, p_info->tipo == TIPO_INT ? "int" : "car");
                    p_info = p_info->proximo;
                }
            } else {
                printf("\n");
            }
            simbolo_atual = simbolo_atual->proximo;
        }
        escopo_atual = escopo_atual->proximo;
        nivel_escopo++;
    }
    printf("==============================================================\n\n");
}

int eh_global(ScopeStack* pilha, char* nome) {
    if (!pilha) return 0;

    SymbolTable* escopo_global = pilha->topo;
    Symbol* simbolo_atual = escopo_global->head;

    while (simbolo_atual) {
        if (strcmp(simbolo_atual->nome, nome) == 0) {
            return 1; // Encontrado
        }
        simbolo_atual = simbolo_atual->proximo;
    }

    return 0;
}