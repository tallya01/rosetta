# Compilador para a Linguagem Goianinha

Este repositório contém o código-fonte de um compilador para a linguagem didática "Goianinha", criada pelo professor Thierson C. Rosa (UFG). O projeto é parte da disciplina de Compiladores do Instituto de Informática da Universidade Federal de Goiás.

Neste projeto, foram implementados todos os componentes de um compilador, incluindo as análises léxica, sintática, semântica e a geração de código assembly. A geração de código assembly apresenta alguns problemas de corretude, e por isso **sua execução está comentada no arquivo .y**, porém as demais etapas estão consistentes. Caso deseje executar a geração de código, basta descomentar a linha 433 em `goianinha.y`

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

A AST é uma representação hierárquica do código-fonte, que captura sua estrutura de forma mais limpa que a árvore de análise concreta. Ela é construída pelo analisador sintático e serve como estrutura de dados de entrada para as etapas seguintes.

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

### 6. Gerador de Código

A etapa final do processo de compilação é a geração de código. O gerador de código percorre a Árvore Sintática Abstrata (AST), já validada e anotada pelo analisador semântico, e traduz as construções da linguagem Goianinha para código assembly.

  * **Localização**: `analisadores/`
  * **Implementação**: `gerador_codigo.c` e `gerador_codigo.h`
  * **Funcionamento**:
      * Para cada nó da AST, o gerador emite uma ou mais instruções em assembly que implementam a semântica correspondente.
      * O código gerado é armazenado em um arquivo de saída padrão chamado `saida.asm`.

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

## Testes Automatizados

O projeto inclui um conjunto de testes automatizados para verificar o funcionamento de todas as etapas do compilador, desde a análise léxica até a geração de código.

Os testes consistem em um conjunto de programas-fonte em Goianinha, localizados em `testes/programas_teste/`, que incluem tanto códigos corretos quanto códigos com erros léxicos, sintáticos e semânticos.

### Como Executar os Testes

Para executar os testes, utilize o script `executor_testes.sh` através do `Makefile` no diretório `testes`.

```bash
# 1. Navegue até o diretório de testes
cd testes/

# 2. Execute o comando 'test' do Makefile
make test
```

O script fará o seguinte:
  * Compilará cada programa de teste em `programas_teste/`.
  * Os resultados da compilação (saída padrão e erros) serão salvos em arquivos `.txt` no diretório `resultados_teste/`.
  * Para os programas corretos, o código assembly gerado (`saida.asm`) será copiado para um arquivo correspondente no mesmo diretório (`<nome_do_teste>_code.asm`).

Para limpar os resultados dos testes, execute:
```bash
make clean
```
