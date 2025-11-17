/* goianinha.y - Analisador Sintático e Gerador de AST para a Linguagem Goianinha */

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tabela_simbolos.h"
#include "ast.h"
#include "semantico.h"

extern int yylex();
extern int yylineno;
extern char* yytext;
extern FILE *yyin;

void yyerror(const char *s);

ScopeStack* g_pilha_escopos;
Tipo g_tipo_atual;
int g_ordem_var = 0;
Symbol* g_funcao_atual = NULL;
ASTNode* g_raiz_ast = NULL;

%}

%union {
    int num_val;
    char *str_val;
    Tipo tipo_val;
    Symbol* sym_ptr;
    struct ASTNode* ast_node;
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
%type <ast_node> Programa DeclFuncVar DeclGlobal DeclVarGlobal ListaDeclVarCont
%type <ast_node> DeclFunc Bloco ListaParametros ListaParametrosCont
%type <ast_node> DeclProg ListaDeclVar DeclVarLocal
%type <ast_node> ListaComando Comando BlocoComoComando
%type <ast_node> Expr Atribuicao OrExpr AndExpr EqExpr DesigExpr AddExpr MulExpr UnExpr PrimExpr ListExpr
%type <ast_node> ComandoSe ComandoEnquanto ComandoLeia ComandoEscreva ComandoRetorne

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
    {
        $$ = criar_no(NO_PROGRAMA, $1, $2, NULL, yylineno);
        g_raiz_ast = $$; /* Salva na variável global */
    }
    ;

DeclFuncVar:
    /* Vazio */ { $$ = NULL; }
    | DeclFuncVar DeclGlobal
    {
        if ($1 == NULL) {
            $$ = $2;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = $2;
            $$ = $1;
        }
    }
    ;

DeclGlobal:
    DeclVarGlobal { $$ = $1; }
    | DeclFunc    { $$ = $1; }
    ;

DeclVarGlobal:
    Tipo T_ID
    {
        if (inserir_variavel(g_pilha_escopos, $2, $1, 0) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Simbolo global '%s' redeclarado.", $2);
            yyerror(erro_msg);
            YYABORT;
        }
    }
    ListaDeclVarCont T_PVIRGULA
    {
        ASTNode *id_node = criar_folha_id($2, yylineno);
        id_node->tipo_dado = $1;
        
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL, yylineno);
        decl_node->tipo_dado = $1;
        
        decl_node->prox = $4; 
        
        $$ = decl_node;
        free($2);
    }
    ;

ListaDeclVarCont:
    /* Vazio */ { $$ = NULL; }
    |
    ListaDeclVarCont T_VIRGULA T_ID
    {
        if (inserir_variavel(g_pilha_escopos, $3, g_tipo_atual, 0) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Variavel '%s' redeclarada no mesmo escopo.", $3);
            yyerror(erro_msg);
            YYABORT;
        }
        
        ASTNode *id_node = criar_folha_id($3, yylineno);
           
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL, yylineno);
        
         if ($1 == NULL) {
            $$ = decl_node;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = decl_node;
            $$ = $1;
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
        
        ASTNode *id_func = criar_folha_id($1->nome, yylineno);
        
        $$ = criar_no(NO_DECL_FUNC, id_func, $4, $6, yylineno);
        $$->tipo_dado = $1->tipo;
        
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
    /* Vazio */ { $$ = NULL; }
    |
    ListaParametrosCont { $$ = $1; }
    ;

ListaParametrosCont:
    Tipo T_ID
    {
        inserir_parametro(g_pilha_escopos, $2, $1, g_ordem_var);
        adicionar_info_parametro(g_funcao_atual, $2, $1);
        g_funcao_atual->num_args++;
        g_ordem_var++;
        
        ASTNode *id_node = criar_folha_id($2, yylineno);
        $$ = criar_no(NO_DECL_VAR, id_node, NULL, NULL, yylineno);
        $$->tipo_dado = $1;
        
        free($2);
    }
    |
    ListaParametrosCont T_VIRGULA Tipo T_ID
    {
        inserir_parametro(g_pilha_escopos, $4, $3, g_ordem_var);
        adicionar_info_parametro(g_funcao_atual, $4, $3);
        g_funcao_atual->num_args++;
        g_ordem_var++;
        
        ASTNode *id_node = criar_folha_id($4, yylineno);
        ASTNode *param_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL, yylineno);
        param_node->tipo_dado = $3;
        
        if ($1 == NULL) {
            $$ = param_node;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = param_node;
            $$ = $1;
        }
        
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
        /* O DeclProg retorna apenas o bloco principal */
        $$ = $3;
    }
    ;

Bloco:
    T_LCHAVE ListaDeclVar ListaComando T_RCHAVE
    {
        /* Bloco contem lista de declarações locais e lista de comandos */
        $$ = criar_no(NO_BLOCO, $2, $3, NULL, yylineno);
    }
    ;

ListaDeclVar:
    /* Vazio */ { $$ = NULL; }
    | ListaDeclVar DeclVarLocal
    {
        if ($1 == NULL) {
            $$ = $2;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = $2;
            $$ = $1;
        }
    }
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
    }
    ListaDeclVarCont T_PVIRGULA
    {
        ASTNode *id_node = criar_folha_id($2, yylineno);
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL, yylineno);
        decl_node->tipo_dado = $1;
        decl_node->prox = $4; /* Encadeia outras vars da mesma linha: int a, b; */
        
        $$ = decl_node;
        free($2);
    }
    ;

Tipo:
    T_INT { $$ = TIPO_INT; }
    | T_CAR { $$ = TIPO_CAR; }
    ;

ListaComando:
    /* Vazio */ { $$ = NULL; }
    | ListaComando Comando
    {
        if ($1 == NULL) {
            $$ = $2;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = $2;
            $$ = $1;
        }
    }
    ;

Comando:
    Expr T_PVIRGULA          { $$ = $1; }
    | T_PVIRGULA             { $$ = criar_no(NO_NULO, NULL, NULL, NULL, yylineno); }
    | BlocoComoComando       { $$ = $1; }
    | ComandoSe              { $$ = $1; }
    | ComandoEnquanto        { $$ = $1; }
    | ComandoLeia            { $$ = $1; }
    | ComandoEscreva         { $$ = $1; }
    | ComandoRetorne         { $$ = $1; }
    | T_NOVALINHA T_PVIRGULA { $$ = criar_no(NO_NOVALINHA, NULL, NULL, NULL, yylineno); }
    ;

BlocoComoComando:
    { criar_novo_escopo(g_pilha_escopos); } 
    Bloco
    { remover_escopo_atual(g_pilha_escopos); $$ = $2; }
    ;

Expr:
    Atribuicao { $$ = $1; }
    | OrExpr   { $$ = $1; }
    ;

Atribuicao: 
    T_ID T_ATRIB Expr
    {
        if (pesquisar_simbolo(g_pilha_escopos, $1) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Variavel '%s' nao declarada.", $1); 
            yyerror(erro_msg); 
            YYABORT; 
        } 
        ASTNode *id_node = criar_folha_id($1, yylineno);
        $$ = criar_no(NO_ATRIBUICAO, id_node, $3, NULL, yylineno);
        free($1);
    }
    ;

OrExpr:
    AndExpr { $$ = $1; }
    | OrExpr T_OU AndExpr { $$ = criar_no(NO_OU, $1, $3, NULL, yylineno); }
    ;

AndExpr:
    EqExpr { $$ = $1; }
    | AndExpr T_E EqExpr { $$ = criar_no(NO_E, $1, $3, NULL, yylineno); }
    ;

EqExpr:
    DesigExpr { $$ = $1; }
    | EqExpr T_EQ DesigExpr { $$ = criar_no(NO_IGUAL, $1, $3, NULL, yylineno); }
    | EqExpr T_NE DesigExpr { $$ = criar_no(NO_DIF, $1, $3, NULL, yylineno); }
    ;

DesigExpr:
    AddExpr { $$ = $1; }
    | DesigExpr T_MENOR AddExpr { $$ = criar_no(NO_MENOR, $1, $3, NULL, yylineno); }
    | DesigExpr T_MAIOR AddExpr { $$ = criar_no(NO_MAIOR, $1, $3, NULL, yylineno); }
    | DesigExpr T_LE AddExpr    { $$ = criar_no(NO_MENOR_IGUAL, $1, $3, NULL, yylineno); }
    | DesigExpr T_GE AddExpr    { $$ = criar_no(NO_MAIOR_IGUAL, $1, $3, NULL, yylineno); }
    ;

AddExpr:
    MulExpr { $$ = $1; }
    | AddExpr T_SOMA MulExpr { $$ = criar_no(NO_SOMA, $1, $3, NULL, yylineno); }
    | AddExpr T_SUB MulExpr  { $$ = criar_no(NO_SUB, $1, $3, NULL, yylineno); }
    ;

MulExpr:
    UnExpr { $$ = $1; }
    | MulExpr T_MULT UnExpr { $$ = criar_no(NO_MULT, $1, $3, NULL, yylineno); }
    | MulExpr T_DIV UnExpr  { $$ = criar_no(NO_DIV, $1, $3, NULL, yylineno); }
    ;

UnExpr:
    PrimExpr { $$ = $1; }
    | T_SUB UnExpr { 
        /* Subtração unária (negativo aritmético) */
        ASTNode *zero = criar_folha_int(0, yylineno);
        $$ = criar_no(NO_SUB, zero, $2, NULL, yylineno); 
      }
    | T_NEG UnExpr { $$ = criar_no(NO_NEG, $2, NULL, NULL, yylineno); }
    ;

PrimExpr:
    T_ID 
    {
        if (pesquisar_simbolo(g_pilha_escopos, $1) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Simbolo '%s' nao declarado.", $1);
            yyerror(erro_msg); YYABORT;
        }
        $$ = criar_folha_id($1, yylineno);
        free($1);
    }
    | T_ID T_LPAREN T_RPAREN
    {
         ASTNode *id_node = criar_folha_id($1, yylineno);
         $$ = criar_no(NO_CHAMADA_FUNC, id_node, NULL, NULL, yylineno);
         free($1);
    }
    | T_ID T_LPAREN ListExpr T_RPAREN
    {
         ASTNode *id_node = criar_folha_id($1, yylineno);
         $$ = criar_no(NO_CHAMADA_FUNC, id_node, $3, NULL, yylineno);
         free($1);
    }
    | T_INTCONST { $$ = criar_folha_int($1, yylineno); }
    | T_CARCONST { $$ = criar_folha_car($1, yylineno); free($1); }
    | T_LPAREN Expr T_RPAREN { $$ = $2; }
    ;

ListExpr:
    Expr { $$ = $1; }
    | ListExpr T_VIRGULA Expr
    {
        if ($1 == NULL) {
            $$ = $3;
        } else {
            ASTNode *temp = $1;
            while (temp->prox != NULL) temp = temp->prox;
            temp->prox = $3;
            $$ = $1;
        }
    }
    ;

ComandoSe:
    T_SE T_LPAREN Expr T_RPAREN T_ENTAO Comando %prec T_ENTAO
    {
        /* IF sem ELSE: Filho1=Expr, Filho2=Comando, Filho3=NULL */
        $$ = criar_no(NO_SE, $3, $6, NULL, yylineno);
    }
    | T_SE T_LPAREN Expr T_RPAREN T_ENTAO Comando T_SENAO Comando
    {
        /* IF com ELSE: Filho1=Expr, Filho2=CmdThen, Filho3=CmdElse */
        $$ = criar_no(NO_SE, $3, $6, $8, yylineno);
    }
    ;

ComandoEnquanto:
    T_ENQUANTO T_LPAREN Expr T_RPAREN T_EXECUTE Comando
    {
        $$ = criar_no(NO_ENQUANTO, $3, $6, NULL, yylineno);
    }
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
        ASTNode *id_node = criar_folha_id($2, yylineno);
        $$ = criar_no(NO_LEIA, id_node, NULL, NULL, yylineno);
        free($2);
    }
    ;

ComandoEscreva:
    T_ESCREVA Expr T_PVIRGULA { $$ = criar_no(NO_ESCREVA, $2, NULL, NULL, yylineno); }
    | T_ESCREVA T_CADEIA T_PVIRGULA
    {
        /* Tratamento de string literal no escreva */
        ASTNode *str_node = criar_folha_id($2, yylineno);
        $$ = criar_no(NO_ESCREVA, str_node, NULL, NULL, yylineno);
        free($2);
    }
    ;

ComandoRetorne: 
    T_RETORNE Expr T_PVIRGULA
    {
        $$ = criar_no(NO_RETORNE, $2, NULL, NULL, yylineno);
    }
    ;

%%

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
        /* printf("\n--- Conteudo Final da Tabela de Simbolos ---\n"); */
        /* imprimir_pilha(g_pilha_escopos); */
        
        /* printf("\n--- Arvore Sintatica Abstrata (AST) ---\n"); */
        /* imprimir_ast(g_raiz_ast, 0); */

        printf("\nAnalise sintatica bem-sucedida!\n");

        int semantico_result = verificar_semantica(g_raiz_ast);
        if(semantico_result == 0) {
            /* realizar geração de código */
        }
    }

    eliminar_pilha_tabelas(g_pilha_escopos);
    liberar_ast(g_raiz_ast);

    if (yyin != stdin) {
        fclose(yyin);
    }
    
    return parse_result;
}

void yyerror(const char *s) {
    fprintf(stderr, "ERRO: %s na linha %d\n", s, yylineno);
}