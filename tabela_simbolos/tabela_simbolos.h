#ifndef TABELA_SIMBOLOS_H
#define TABELA_SIMBOLOS_H

// Enum para os tipos de dados da linguagem Goianinha
typedef enum {
    TIPO_INT,
    TIPO_CAR
} Tipo;

// Enum para a categoria do símbolo
typedef enum {
    CAT_VARIAVEL,
    CAT_PARAMETRO,
    CAT_FUNCAO
} Categoria;

// Estrutura para armazenar informações sobre os parâmetros de uma função
typedef struct ParametroInfo {
    char* nome;
    Tipo tipo;
    struct ParametroInfo* proximo;
} ParametroInfo;

// Estrutura para uma entrada na tabela de símbolos (um símbolo)
typedef struct Symbol {
    char* nome;                 // Lexema do identificador
    Categoria categoria;
    Tipo tipo;                  // Tipo da variável/parâmetro ou tipo de retorno da função
    int ordem;                  // Ordem de declaração para variáveis/parâmetros
    int num_args;               // Número de argumentos (apenas para funções)
    ParametroInfo* params_info; // Lista de informações dos parâmetros (apenas para funções)
    struct Symbol* proximo;     // Ponteiro para o próximo símbolo no mesmo escopo
} Symbol;

// Estrutura para a tabela de símbolos de um escopo
typedef struct SymbolTable {
    Symbol* head;
    struct SymbolTable* proximo; // Ponteiro para o próximo escopo na pilha
} SymbolTable;

// Estrutura para a pilha de escopos
typedef struct {
    SymbolTable* topo;
} ScopeStack;

/**
 * @brief Inicia a pilha de tabelas de símbolos.
 *
 * @return Ponteiro para a nova pilha de escopos criada.
 */
ScopeStack* iniciar_pilha_tabela_simbolos();

/**
 * @brief Cria um novo escopo (tabela de símbolos) e o empilha.
 *
 * @param pilha A pilha de escopos.
 */
void criar_novo_escopo(ScopeStack* pilha);

/**
 * @brief Remove o escopo atual (do topo da pilha).
 *
 * @param pilha A pilha de escopos.
 */
void remover_escopo_atual(ScopeStack* pilha);

/**
 * @brief Insere um nome de variável na tabela de símbolos do escopo atual.
 *
 * @param pilha A pilha de escopos.
 * @param nome O nome da variável.
 * @param tipo O tipo da variável.
 * @param ordem A posição da variável na lista de declaração.
 * @return Ponteiro para o símbolo inserido ou NULL se já existir no escopo atual.
 */
Symbol* inserir_variavel(ScopeStack* pilha, const char* nome, Tipo tipo, int ordem);

/**
 * @brief Insere o nome de um parâmetro na tabela de símbolos do escopo atual.
 *
 * @param pilha A pilha de escopos.
 * @param nome O nome do parâmetro.
 * @param tipo O tipo do parâmetro.
 * @param ordem A posição do parâmetro na lista de declaração.
 * @return Ponteiro para o símbolo inserido ou NULL se já existir no escopo atual.
 */
Symbol* inserir_parametro(ScopeStack* pilha, const char* nome, Tipo tipo, int ordem);

/**
 * @brief Insere um nome de função na tabela de símbolos do escopo atual.
 *
 * @param pilha A pilha de escopos.
 * @param nome O nome da função.
 * @param tipo_retorno O tipo de retorno da função.
 * @param num_args O número de argumentos da função.
 * @return Ponteiro para o símbolo da função inserida ou NULL se já existir.
 */
Symbol* inserir_funcao(ScopeStack* pilha, const char* nome, Tipo tipo_retorno, int num_args);

/**
 * @brief Adiciona informações de um parâmetro a uma função já declarada.
 *
 * @param simbolo_funcao Ponteiro para o símbolo da função.
 * @param nome_param O nome do parâmetro.
 * @param tipo_param O tipo do parâmetro.
 */
void adicionar_info_parametro(Symbol* simbolo_funcao, const char* nome_param, Tipo tipo_param);


/**
 * @brief Pesquisa por um nome em toda a pilha de escopos, do topo à base.
 *
 * @param pilha A pilha de escopos.
 * @param nome O nome a ser pesquisado.
 * @return Ponteiro para o símbolo encontrado ou NULL se não for encontrado.
 */
Symbol* pesquisar_simbolo(ScopeStack* pilha, const char* nome);

/**
 * @brief Elimina a pilha de tabelas de símbolos, liberando toda a memória.
 *
 * @param pilha A pilha de escopos.
 */
void eliminar_pilha_tabelas(ScopeStack* pilha);

/**
 * @brief Imprime a pilha de tabelas de símbolos para depuração.
 *
 * @param pilha A pilha de escopos.
 */
void imprimir_pilha(ScopeStack* pilha);

#endif // TABELA_SIMBOLOS_H