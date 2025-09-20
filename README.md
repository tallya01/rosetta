# Compilador para a Linguagem Goianinha (Etapa 1)

Este repositório contém o código-fonte da primeira etapa do desenvolvimento de um compilador para a linguagem didática "Goianinha", criada pelo professor Thierson C. Rosa (UFG). O projeto é parte da disciplina de Compiladores do Instituto de Informática da Universidade Federal de Goiás.

Nesta fase, foram implementados os primeiros componentes para a análise de um programa-fonte: a Tabela de Símbolos, o Analisador Léxico e o Analisador Sintático.

## Componentes Implementados

### 1\. Tabela de Símbolos

Foi implementada uma estrutura de dados de **pilha de tabelas de símbolos** para gerenciar os escopos do programa. Esta estrutura é crucial para a análise semântica (a ser feita na próxima etapa) e permite a declaração de variáveis com o mesmo nome em escopos diferentes.

  * **Localização**: `tabela_simbolos/`
  * **Implementação**: `tabela_simbolos.c` e `tabela_simbolos.h`
  * **Testes**: Um programa de teste (`main.c`) foi criado para validar as operações da pilha, como criação e remoção de escopos e inserção e busca de símbolos.

### 2\. Analisador Léxico

O analisador léxico, gerado com o **Flex**, é responsável por ler o código-fonte e convertê-lo em uma sequência de tokens.

  * **Localização**: `analisadores/`
  * **Arquivo de especificação**: `goianinha.l`
  * **Responsabilidades**:
      * Reconhecer palavras-reservadas (`programa`, `se`, `int`, etc.).
      * Identificar literais, como identificadores (`id`), constantes inteiras (`intconst`) e caracteres (`carconst`).
      * Processar operadores, pontuação e outros símbolos da linguagem.
      * Remover comentários (`/* ... */`) e espaços em branco.
      * Contabilizar o número da linha (`yylineno`) para reportar erros.

### 3\. Analisador Sintático

O analisador sintático, gerado com o **Bison**, verifica se a sequência de tokens produzida pelo analisador léxico obedece à gramática da linguagem Goianinha.

  * **Localização**: `analisadores/`
  * **Arquivo de especificação**: `goianinha.y`
  * **Responsabilidades**:
      * Implementar a gramática formal da linguagem.
      * Reportar erros sintáticos (`ERRO SINTATICO`) com o número da linha correspondente.
      * Integrar-se com o analisador léxico (função `yylex()`) para obter os tokens.
      * Construir a árvore de análise sintática (embora as ações semânticas ainda não estejam implementadas nesta fase).
      * Orquestrar a análise, incluindo a inicialização da tabela de símbolos no início do processo.

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

### Compilando e Executando o Analisador Sintático

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