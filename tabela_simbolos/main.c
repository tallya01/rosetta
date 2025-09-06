#include <stdio.h>
#include "tabela_simbolos.h"

void testar_pesquisa(ScopeStack* pilha, const char* nome) {
    printf("Pesquisando por '%s': ", nome);
    Symbol* s = pesquisar_simbolo(pilha, nome);
    if (s) {
        printf("Encontrado! Categoria: %s\n", s->categoria == CAT_VARIAVEL ? "Variavel" : (s->categoria == CAT_PARAMETRO ? "Parametro" : "Funcao"));
    } else {
        printf("Não encontrado.\n");
    }
}

int main() {
    printf("Iniciando teste da Tabela de Símbolos...\n\n");

    // a) Iniciar a pilha
    ScopeStack* minha_pilha = iniciar_pilha_tabela_simbolos();
    printf("Pilha de escopos inicializada (com escopo global).\n");
    imprimir_pilha(minha_pilha);

    // Inserindo variável global
    printf("Inserindo variável global 'g_var' (tipo int, ordem 1).\n");
    inserir_variavel(minha_pilha, "g_var", TIPO_INT, 1);
    
    // e) Inserindo uma função no escopo global
    printf("Inserindo função 'soma' (retorno int, 2 args) no escopo global.\n");
    Symbol* func_soma = inserir_funcao(minha_pilha, "soma", TIPO_INT, 2);
    // Adicionando info dos parâmetros para a função
    adicionar_info_parametro(func_soma, "a", TIPO_INT);
    adicionar_info_parametro(func_soma, "b", TIPO_INT);

    imprimir_pilha(minha_pilha);

    // Simula a entrada no bloco da função 'soma'
    // b) Criar um novo escopo para a função
    printf("Entrando no escopo da função 'soma'. Criando novo escopo.\n");
    criar_novo_escopo(minha_pilha);

    // g) Inserindo os parâmetros no escopo da função
    printf("Inserindo parâmetros 'a' (int, 1) e 'b' (int, 2) no novo escopo.\n");
    inserir_parametro(minha_pilha, "a", TIPO_INT, 1);
    inserir_parametro(minha_pilha, "b", TIPO_INT, 2);

    // f) Inserindo uma variável local
    printf("Inserindo variável local 'resultado' (int, 1) no escopo da função.\n");
    inserir_variavel(minha_pilha, "resultado", TIPO_INT, 1);
    imprimir_pilha(minha_pilha);

    // c) Testando a pesquisa de símbolos
    testar_pesquisa(minha_pilha, "a");         // Deve encontrar o parâmetro 'a'
    testar_pesquisa(minha_pilha, "g_var");     // Deve encontrar a global 'g_var'
    testar_pesquisa(minha_pilha, "nao_existe"); // Não deve encontrar

    // Simula a entrada em um bloco aninhado
    printf("\nEntrando em um bloco aninhado (ex: dentro de um 'se'). Criando novo escopo.\n");
    criar_novo_escopo(minha_pilha);
    printf("Inserindo variável 'g_var' (tipo car, ordem 1) neste escopo (shadowing).\n");
    inserir_variavel(minha_pilha, "g_var", TIPO_CAR, 1);
    imprimir_pilha(minha_pilha);

    printf("Pesquisando por 'g_var' no escopo mais interno...\n");
    Symbol* s_gvar = pesquisar_simbolo(minha_pilha, "g_var");
    if (s_gvar) {
        printf("Encontrado! Tipo: %s. (Corretamente encontrou a variável local)\n", s_gvar->tipo == TIPO_CAR ? "car" : "int");
    }

    // d) Removendo escopos
    printf("\nSaindo do bloco aninhado. Removendo escopo.\n");
    remover_escopo_atual(minha_pilha);
    imprimir_pilha(minha_pilha);

    printf("Pesquisando por 'g_var' novamente...\n");
    s_gvar = pesquisar_simbolo(minha_pilha, "g_var");
     if (s_gvar) {
        printf("Encontrado! Tipo: %s. (Corretamente encontrou a variável global)\n", s_gvar->tipo == TIPO_CAR ? "car" : "int");
    }

    printf("\nSaindo da função 'soma'. Removendo escopo.\n");
    remover_escopo_atual(minha_pilha);
    imprimir_pilha(minha_pilha);

    // h) Eliminar a pilha
    printf("Finalizando teste. Eliminando a pilha de tabelas.\n");
    eliminar_pilha_tabelas(minha_pilha);
    printf("Pilha eliminada.\n");

    return 0;
}