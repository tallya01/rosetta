# Compilador para a Linguagem Goianinha

Este repositório contém o código-fonte das etapas de análise de um compilador para a linguagem didática "Goianinha", criada pelo professor Thierson C. Rosa (UFG). O projeto é parte da disciplina de Compiladores do Instituto de Informática da Universidade Federal de Goiás.

Nesta fase, foram implementados os componentes para a análise léxica, sintática e semântica de um programa-fonte, além das estruturas de dados de suporte. A última fase será a implementação de um gerador de código assembly.

## Componentes Implementados

### 1. Tabela de Símbolos

Foi implementada uma estrutura de dados de **pilha de tabelas de símbolos** para gerenciar os escopos do programa. Esta estrutura é crucial para a análise semântica e permite a declaração de variáveis com o mesmo nome em escopos diferentes.

  * **Localização**: `tabela_simbolos/`
  * **Implementação**: `tabela_simbolos.c` e `tabela_simbolos.h`
  * **Testes**: Um programa de teste (`main.c`) foi criado para validar as operações da pilha, como criação e remoção de escopos e inserção e busca de símbolos.

### 2. Analisador Léxico

O analisador léxico, gerado com o **Flex**, é responsável por ler o código-fonte e convertê-lo em uma sequência de tokens.

  * **Localização**: `analisadores/`
  * **Arquivo de especificação**: `goianinha.l`
  * **Responsabilidades**:
      * Reconhecer palavras-reservadas (`programa`, `se`, `int`, etc.).
      * Identificar literais, como identificadores (`id`), constantes inteiras (`intconst`) e caracteres (`carconst`).
      * Processar operadores, pontuação e outros símbolos da linguagem.
      * Remover comentários (`/* ... */`) e espaços em branco.
      * Contabilizar o número da linha (`yylineno`) para reportar erros.

### 3. Analisador Sintático

O analisador sintático, gerado com o **Bison**, verifica se a sequência de tokens produzida pelo analisador léxico obedece à gramática da linguagem Goianinha.

  * **Localização**: `analisadores/`
  * **Arquivo de especificação**: `goianinha.y`
  * **Responsabilidades**:
      * Implementar a gramática formal da linguagem.
      * Reportar erros sintáticos (`ERRO SINTATICO`) com o número da linha correspondente.
      * Integrar-se com o analisador léxico (função `yylex()`).
      * **Construir a Árvore Sintática Abstrata (AST)** durante a análise.

### 4. Árvore Sintática Abstrata (AST)

A AST é uma representação hierárquica do código-fonte, que captura sua estrutura de forma mais limpa que a árvore de análise concreta. Ela é construída pelo analisador sintático e serve como estrutura de dados de entrada para a análise semântica.

  * **Localização**: `analisadores/`
  * **Implementação**: `ast.c` e `ast.h`
  * **Estrutura**:
      * Cada nó (`ASTNode`) representa uma construção da linguagem (declaração, comando, expressão, etc.).
      * Os nós contêm informações como tipo do nó, filhos, valor léxico e tipo de dado (preenchido na análise semântica).

### 5. Analisador Semântico

O analisador semântico percorre a AST para verificar a consistência e o significado do programa, garantindo que ele faça sentido de acordo com as regras da linguagem.

  * **Localização**: `analisadores/`
  * **Implementação**: `semantico.c` e `semantico.h`
  * **Responsabilidades**:
      * **Gerenciamento de Escopo**: Utiliza a pilha de tabelas de símbolos para controlar a visibilidade de identificadores.
      * **Verificação de Declarações**: Garante que variáveis e funções não sejam redeclaradas no mesmo escopo.
      * **Verificação de Tipos**: Assegura que os tipos de dados em expressões, atribuições e chamadas de função sejam compatíveis.
      * **Reporte de Erros**: Emite mensagens de erro semântico detalhadas, como "variável não declarada" ou "tipos incompatíveis".

## Ferramentas Utilizadas

  * **Linguagem**: C
  * **Gerador de Analisador Léxico**: Flex
  * **Gerador de Analisador Sintático**: Bison
  * **Compilador**: GCC
  * **Sistema de Build**: Make

## Como Compilar e Executar

O projeto está dividido em dois módulos, cada um com seu próprio `Makefile`.

### Testando a Tabela de Símbolos

Para compilar e executar o programa de teste da tabela de símbolos:

```bash
# 1. Entre no diretório da tabela de símbolos
cd tabela_simbolos/

# 2. Compile o projeto
make

# 3. Execute o programa de teste
./teste_tabela
```

### Compilando e Executando os Analisadores

Para compilar o analisador `goianinha` e usá-lo para analisar um arquivo de código:

```bash
# 1. Entre no diretório dos analisadores
cd analisadores/

# 2. Compile o projeto. O Makefile se encarregará de gerar os
#    arquivos a partir do Flex e do Bison e compilar tudo.
make

# 3. Execute o analisador, passando um arquivo-fonte como argumento
#    (substitua 'programa_exemplo.g' pelo seu arquivo).
./goianinha programa_exemplo.g
```
