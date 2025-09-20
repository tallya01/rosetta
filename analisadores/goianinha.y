/* goianinha.y - Analisador Sintático para a Linguagem Goianinha */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela_simbolos.h"

extern int yylex();
extern int yylineno;
extern char* yytext;
extern FILE *yyin;

void yyerror(const char *s);

ScopeStack* g_pilha_escopos;
Tipo g_tipo_atual;
int g_ordem_var = 0;
Symbol* g_funcao_atual = NULL;
%}

%union {
    int num_val;
    char *str_val;
    Tipo tipo_val;
    Symbol* sym_ptr;
}

%token <str_val> T_ID T_CADEIA T_CARCONST
%token <num_val> T_INTCONST
%token T_PROGRAMA T_CAR T_INT T_RETORNE T_LEIA T_ESCREVA T_NOVALINHA
%token T_SE T_ENTAO T_SENAO T_ENQUANTO T_EXECUTE T_OU T_E
%token T_EQ T_NE T_GE T_LE T_SOMA T_SUB T_MULT T_DIV T_ATRIB
%token T_MENOR T_MAIOR T_NEG
%token T_LPAREN T_RPAREN T_LCHAVE T_RCHAVE T_PVIRGULA T_VIRGULA

%type <tipo_val> Tipo
%type <sym_ptr> PreDeclFunc

%left T_OU
%left T_E
%nonassoc T_EQ T_NE
%nonassoc T_MENOR T_MAIOR T_LE T_GE
%left T_SOMA T_SUB
%left T_MULT T_DIV
%right T_NEG
%nonassoc T_ENTAO
%nonassoc T_SENAO

%start Programa

%%

Programa:
    DeclFuncVar DeclProg
    ;

DeclFuncVar:
    /* Vazio */
    | DeclFuncVar DeclGlobal
    ;

DeclGlobal:
    DeclVarGlobal
    | DeclFunc
    ;

DeclVarGlobal:
    Tipo T_ID
    {
        if (inserir_variavel(g_pilha_escopos, $2, $1, 0) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Simbolo global '%s' redeclarado.", $2);
            yyerror(erro_msg);
            YYABORT; /*como não há recuperação de erros, para o parsing*/
        }
        free($2);
    }
    ListaDeclVarCont T_PVIRGULA
    ;

ListaDeclVarCont:
    /* Vazio */
    | ListaDeclVarCont T_VIRGULA T_ID
    {
        if (inserir_variavel(g_pilha_escopos, $3, g_tipo_atual, 0) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Variavel '%s' redeclarada no mesmo escopo.", $3);
            yyerror(erro_msg);
            YYABORT;
        }
        free($3);
    }
    ;

DeclFunc:
    PreDeclFunc
    {
        g_funcao_atual = $1;
        criar_novo_escopo(g_pilha_escopos);
        g_ordem_var = 1;
    }
    T_LPAREN ListaParametros T_RPAREN
    Bloco
    {
        remover_escopo_atual(g_pilha_escopos);
        g_funcao_atual = NULL;
    }
    ;

PreDeclFunc:
    Tipo T_ID
    {
        $$ = inserir_funcao(g_pilha_escopos, $2, $1, 0);
        if ($$ == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Simbolo global '%s' redeclarado.", $2);
            yyerror(erro_msg);
            YYABORT;
        }
        free($2);
    }
    ;

ListaParametros:
    /* Vazio */
    | ListaParametrosCont
    ;

ListaParametrosCont:
    Tipo T_ID
    {
        inserir_parametro(g_pilha_escopos, $2, $1, g_ordem_var);
        adicionar_info_parametro(g_funcao_atual, $2, $1);
        g_funcao_atual->num_args++;
        g_ordem_var++;
        free($2);
    }
    | ListaParametrosCont T_VIRGULA Tipo T_ID
    {
        inserir_parametro(g_pilha_escopos, $4, $3, g_ordem_var);
        adicionar_info_parametro(g_funcao_atual, $4, $3);
        g_funcao_atual->num_args++;
        g_ordem_var++;
        free($4);
    }
    ;

DeclProg:
    T_PROGRAMA
    {
        criar_novo_escopo(g_pilha_escopos);
    }
    Bloco
    {
        remover_escopo_atual(g_pilha_escopos);
    }
    ;

Bloco:
    T_LCHAVE ListaDeclVar ListaComando T_RCHAVE
    ;

ListaDeclVar:
    /* Vazio */
    | ListaDeclVar DeclVarLocal
    ;

DeclVarLocal:
    Tipo T_ID
    {
        g_tipo_atual = $1;
        if (inserir_variavel(g_pilha_escopos, $2, $1, 0) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Variavel '%s' redeclarada no mesmo escopo.", $2);
            yyerror(erro_msg);
            YYABORT;
        }
        free($2);
    }
    ListaDeclVarCont T_PVIRGULA
    ;

Tipo:
    T_INT { $$ = TIPO_INT; }
    | T_CAR { $$ = TIPO_CAR; }
    ;

ListaComando:
    /* Vazio */
    | ListaComando Comando
    ;

Comando:
    Expr T_PVIRGULA
    | T_PVIRGULA
    | BlocoComoComando
    | ComandoSe
    | ComandoEnquanto
    | ComandoLeia
    | ComandoEscreva
    | ComandoRetorne
    | T_NOVALINHA T_PVIRGULA
    ;

BlocoComoComando:
    {
        criar_novo_escopo(g_pilha_escopos);
    } 
    Bloco
    { 
        remover_escopo_atual(g_pilha_escopos);
    }
    ;

Expr:
    Atribuicao
    | OrExpr
    ;

Atribuicao: 
    T_ID T_ATRIB Expr
    {
        if (pesquisar_simbolo(g_pilha_escopos, $1) == NULL) {
            char erro_msg[100]; sprintf(erro_msg, "Variavel '%s' nao declarada.", $1); 
            yyerror(erro_msg); 
            YYABORT; 
        } 
        free($1); 
    }
    ;

OrExpr:
    AndExpr
    | OrExpr T_OU AndExpr
    ;

AndExpr:
    EqExpr 
    | AndExpr T_E EqExpr
    ;

EqExpr:
    DesigExpr
    | EqExpr T_EQ DesigExpr
    | EqExpr T_NE DesigExpr
    ;

DesigExpr:
    AddExpr
    | DesigExpr T_MENOR AddExpr
    | DesigExpr T_MAIOR AddExpr
    | DesigExpr T_LE AddExpr
    | DesigExpr T_GE AddExpr
    ;

AddExpr:
    MulExpr
    | AddExpr T_SOMA MulExpr
    | AddExpr T_SUB MulExpr
    ;

MulExpr:
    UnExpr
    | MulExpr T_MULT UnExpr
    | MulExpr T_DIV UnExpr
    ;

UnExpr:
    PrimExpr
    | T_SUB UnExpr
    | T_NEG UnExpr
    ;

PrimExpr:
    T_ID 
    {
        if (pesquisar_simbolo(g_pilha_escopos, $1) == NULL) {
            char erro_msg[100]; sprintf(erro_msg, "Simbolo '%s' nao declarado.", $1);
            yyerror(erro_msg); YYABORT;
        }
        free($1); 
    }
    | T_ID T_LPAREN T_RPAREN
    | T_ID T_LPAREN ListExpr T_RPAREN
    | T_INTCONST
    | T_CARCONST
    | T_LPAREN Expr T_RPAREN
    ;

ListExpr:
    Expr
    | ListExpr T_VIRGULA Expr
    ;

ComandoSe:
    T_SE T_LPAREN Expr T_RPAREN T_ENTAO Comando %prec T_ENTAO
    | T_SE T_LPAREN Expr T_RPAREN T_ENTAO Comando T_SENAO Comando
    ;

ComandoEnquanto:
    T_ENQUANTO T_LPAREN Expr T_RPAREN T_EXECUTE Comando
    ;

ComandoLeia:
    T_LEIA T_ID T_PVIRGULA 
    {
        if (pesquisar_simbolo(g_pilha_escopos, $2) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Variavel '%s' nao declarada para leitura.", $2);
            yyerror(erro_msg);
            YYABORT; 
        }
        free($2);
    }
    ;

ComandoEscreva:
    T_ESCREVA Expr T_PVIRGULA
    | T_ESCREVA T_CADEIA T_PVIRGULA
    ;

ComandoRetorne: 
    T_RETORNE Expr T_PVIRGULA
    ;

%%
/* --- Seção 3: Código C Adicional --- */

int main(int argc, char **argv) {
    if (argc > 1) {
        yyin = fopen(argv[1], "r");
        if (!yyin) {
            fprintf(stderr, "Erro: Nao foi possivel abrir o arquivo '%s'\n", argv[1]);
            return 1;
        }
    } else {
        yyin = stdin;
    }

    g_pilha_escopos = iniciar_pilha_tabela_simbolos();

    int parse_result = yyparse();

    if (parse_result == 0) {
        printf("\n--- Conteudo Final da Tabela de Simbolos ---\n");
        imprimir_pilha(g_pilha_escopos);
        printf("Analise sintatica bem-sucedida!\n");
    }

    eliminar_pilha_tabelas(g_pilha_escopos);

    if (yyin != stdin) {
        fclose(yyin);
    }
    
    return parse_result;
}

void yyerror(const char *s) {
    fprintf(stderr, "ERRO: %s na linha %d\n", s, yylineno);
}