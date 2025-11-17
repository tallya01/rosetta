/* goianinha.y - Analisador Sintático e Gerador de AST para a Linguagem Goianinha */

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

typedef enum {
    NO_PROGRAMA,
    NO_DECL_FUNC,
    NO_DECL_VAR,
    NO_BLOCO,
    NO_SE,
    NO_ENQUANTO,
    NO_ATRIBUICAO,
    NO_RETORNE,
    NO_LEIA,
    NO_ESCREVA,
    NO_SOMA, NO_SUB, NO_MULT, NO_DIV,
    NO_IGUAL, NO_DIF, NO_MAIOR, NO_MENOR, NO_MAIOR_IGUAL, NO_MENOR_IGUAL,
    NO_E, NO_OU, NO_NEG,
    NO_ID,
    NO_INT_CONST,
    NO_CAR_CONST,
    NO_CHAMADA_FUNC,
    NO_LISTA,      /* Para sequências de comandos ou declarações */
    NO_NOVALINHA,
    NO_NULO        /* Para nós vazios */
} TipoNo;

typedef struct ASTNode {
    TipoNo tipo;
    int linha;              
    char *valor_lexico;     /* Para IDs e Strings */
    int valor_int;          /* Para constantes inteiras */
    Tipo tipo_dado;         /* TIPO_INT, TIPO_CAR (para análise semântica) */
    
    struct ASTNode *filho[3]; /* Até 3 filhos (ex: IF expr ENTAO cmd SENAO cmd) */
    struct ASTNode *prox;     /* Para listas encadeadas de comandos/decls */
} ASTNode;

ScopeStack* g_pilha_escopos;
Tipo g_tipo_atual;
int g_ordem_var = 0;
Symbol* g_funcao_atual = NULL;
ASTNode* g_raiz_ast = NULL;

ASTNode* criar_no(TipoNo tipo, ASTNode* f1, ASTNode* f2, ASTNode* f3);
ASTNode* criar_folha_id(char* lexema);
ASTNode* criar_folha_int(int valor);
ASTNode* criar_folha_car(char* lexema);
void imprimir_ast(ASTNode* no, int nivel);
void liberar_ast(ASTNode* no);

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
        $$ = criar_no(NO_PROGRAMA, $1, $2, NULL);
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
        ASTNode *id_node = criar_folha_id($2);
        id_node->tipo_dado = $1;
        
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL);
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
        
        ASTNode *id_node = criar_folha_id($3);
           
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL);
        
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
        
        ASTNode *id_func = criar_folha_id($1->nome);
        
        $$ = criar_no(NO_DECL_FUNC, id_func, $4, $6);
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
        
        ASTNode *id_node = criar_folha_id($2);
        $$ = criar_no(NO_DECL_VAR, id_node, NULL, NULL);
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
        
        ASTNode *id_node = criar_folha_id($4);
        ASTNode *param_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL);
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
        $$ = criar_no(NO_BLOCO, $2, $3, NULL);
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
        ASTNode *id_node = criar_folha_id($2);
        ASTNode *decl_node = criar_no(NO_DECL_VAR, id_node, NULL, NULL);
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
    | T_PVIRGULA             { $$ = criar_no(NO_NULO, NULL, NULL, NULL); }
    | BlocoComoComando       { $$ = $1; }
    | ComandoSe              { $$ = $1; }
    | ComandoEnquanto        { $$ = $1; }
    | ComandoLeia            { $$ = $1; }
    | ComandoEscreva         { $$ = $1; }
    | ComandoRetorne         { $$ = $1; }
    | T_NOVALINHA T_PVIRGULA { $$ = criar_no(NO_NOVALINHA, NULL, NULL, NULL); }
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
        ASTNode *id_node = criar_folha_id($1);
        $$ = criar_no(NO_ATRIBUICAO, id_node, $3, NULL);
        free($1);
    }
    ;

OrExpr:
    AndExpr { $$ = $1; }
    | OrExpr T_OU AndExpr { $$ = criar_no(NO_OU, $1, $3, NULL); }
    ;

AndExpr:
    EqExpr { $$ = $1; }
    | AndExpr T_E EqExpr { $$ = criar_no(NO_E, $1, $3, NULL); }
    ;

EqExpr:
    DesigExpr { $$ = $1; }
    | EqExpr T_EQ DesigExpr { $$ = criar_no(NO_IGUAL, $1, $3, NULL); }
    | EqExpr T_NE DesigExpr { $$ = criar_no(NO_DIF, $1, $3, NULL); }
    ;

DesigExpr:
    AddExpr { $$ = $1; }
    | DesigExpr T_MENOR AddExpr { $$ = criar_no(NO_MENOR, $1, $3, NULL); }
    | DesigExpr T_MAIOR AddExpr { $$ = criar_no(NO_MAIOR, $1, $3, NULL); }
    | DesigExpr T_LE AddExpr    { $$ = criar_no(NO_MENOR_IGUAL, $1, $3, NULL); }
    | DesigExpr T_GE AddExpr    { $$ = criar_no(NO_MAIOR_IGUAL, $1, $3, NULL); }
    ;

AddExpr:
    MulExpr { $$ = $1; }
    | AddExpr T_SOMA MulExpr { $$ = criar_no(NO_SOMA, $1, $3, NULL); }
    | AddExpr T_SUB MulExpr  { $$ = criar_no(NO_SUB, $1, $3, NULL); }
    ;

MulExpr:
    UnExpr { $$ = $1; }
    | MulExpr T_MULT UnExpr { $$ = criar_no(NO_MULT, $1, $3, NULL); }
    | MulExpr T_DIV UnExpr  { $$ = criar_no(NO_DIV, $1, $3, NULL); }
    ;

UnExpr:
    PrimExpr { $$ = $1; }
    | T_SUB UnExpr { 
        /* Subtração unária (negativo aritmético) */
        ASTNode *zero = criar_folha_int(0);
        $$ = criar_no(NO_SUB, zero, $2, NULL); 
      }
    | T_NEG UnExpr { $$ = criar_no(NO_NEG, $2, NULL, NULL); }
    ;

PrimExpr:
    T_ID 
    {
        if (pesquisar_simbolo(g_pilha_escopos, $1) == NULL) {
            char erro_msg[100];
            sprintf(erro_msg, "Simbolo '%s' nao declarado.", $1);
            yyerror(erro_msg); YYABORT;
        }
        $$ = criar_folha_id($1);
        free($1);
    }
    | T_ID T_LPAREN T_RPAREN
    {
         ASTNode *id_node = criar_folha_id($1);
         $$ = criar_no(NO_CHAMADA_FUNC, id_node, NULL, NULL);
         free($1);
    }
    | T_ID T_LPAREN ListExpr T_RPAREN
    {
         ASTNode *id_node = criar_folha_id($1);
         $$ = criar_no(NO_CHAMADA_FUNC, id_node, $3, NULL);
         free($1);
    }
    | T_INTCONST { $$ = criar_folha_int($1); }
    | T_CARCONST { $$ = criar_folha_car($1); free($1); }
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
        $$ = criar_no(NO_SE, $3, $6, NULL);
    }
    | T_SE T_LPAREN Expr T_RPAREN T_ENTAO Comando T_SENAO Comando
    {
        /* IF com ELSE: Filho1=Expr, Filho2=CmdThen, Filho3=CmdElse */
        $$ = criar_no(NO_SE, $3, $6, $8);
    }
    ;

ComandoEnquanto:
    T_ENQUANTO T_LPAREN Expr T_RPAREN T_EXECUTE Comando
    {
        $$ = criar_no(NO_ENQUANTO, $3, $6, NULL);
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
        ASTNode *id_node = criar_folha_id($2);
        $$ = criar_no(NO_LEIA, id_node, NULL, NULL);
        free($2);
    }
    ;

ComandoEscreva:
    T_ESCREVA Expr T_PVIRGULA { $$ = criar_no(NO_ESCREVA, $2, NULL, NULL); }
    | T_ESCREVA T_CADEIA T_PVIRGULA
    {
        /* Tratamento de string literal no escreva */
        ASTNode *str_node = criar_folha_id($2);
        $$ = criar_no(NO_ESCREVA, str_node, NULL, NULL);
        free($2);
    }
    ;

ComandoRetorne: 
    T_RETORNE Expr T_PVIRGULA
    {
        $$ = criar_no(NO_RETORNE, $2, NULL, NULL);
    }
    ;

%%
/* --- Seção 3: Código C Adicional e Implementação da AST --- */

ASTNode* criar_no(TipoNo tipo, ASTNode* f1, ASTNode* f2, ASTNode* f3) {
    ASTNode* no = (ASTNode*) malloc(sizeof(ASTNode));
    if (no != NULL) {
        no->tipo = tipo;
        no->linha = yylineno; 
        no->valor_lexico = NULL;
        no->filho[0] = f1;
        no->filho[1] = f2;
        no->filho[2] = f3;
        no->prox = NULL;
    }
    return no;
}

ASTNode* criar_folha_id(char* lexema) {
    ASTNode* no = criar_no(NO_ID, NULL, NULL, NULL);
    if (no != NULL && lexema != NULL) {
        no->valor_lexico = strdup(lexema); /* Copia a string */
    }
    return no;
}

ASTNode* criar_folha_int(int valor) {
    ASTNode* no = criar_no(NO_INT_CONST, NULL, NULL, NULL);
    if (no != NULL) {
        no->valor_int = valor;
    }
    return no;
}

ASTNode* criar_folha_car(char* lexema) {
    ASTNode* no = criar_no(NO_CAR_CONST, NULL, NULL, NULL);
    if (no != NULL && lexema != NULL) {
        no->valor_lexico = strdup(lexema);
    }
    return no;
}

/* Função auxiliar para imprimir a AST (visualização) */
void imprimir_ast(ASTNode* no, int nivel) {
    if (no == NULL) return;

    for (int i = 0; i < nivel; i++) printf("  ");
    
    switch(no->tipo) {
        case NO_PROGRAMA: printf("PROGRAMA\n"); break;
        case NO_DECL_FUNC: printf("FUNC_DECL\n"); break;
        case NO_DECL_VAR: printf("VAR_DECL\n"); break;
        case NO_BLOCO: printf("BLOCO\n"); break;
        case NO_SE: printf("SE\n"); break;
        case NO_ENQUANTO: printf("ENQUANTO\n"); break;
        case NO_ATRIBUICAO: printf("ATRIBUICAO\n"); break;
        case NO_LEIA: printf("LEIA\n"); break;
        case NO_ESCREVA: printf("ESCREVA\n"); break;
        case NO_SOMA: printf("SOMA (+)\n"); break;
        case NO_SUB: printf("SUB (-)\n"); break;
        case NO_MULT: printf("MULT (*)\n"); break;
        case NO_DIV: printf("DIV (/)\n"); break;
        case NO_ID: printf("ID: %s\n", no->valor_lexico); break;
        case NO_INT_CONST: printf("INT: %d\n", no->valor_int); break;
        case NO_CAR_CONST: printf("CAR: %s\n", no->valor_lexico); break;
        case NO_CHAMADA_FUNC: printf("CHAMADA_FUNC\n"); break;
        default: printf("NO_TIPO_%d\n", no->tipo);
    }

    imprimir_ast(no->filho[0], nivel + 1);
    imprimir_ast(no->filho[1], nivel + 1);
    imprimir_ast(no->filho[2], nivel + 1);
    
    if (no->prox != NULL) {
        imprimir_ast(no->prox, nivel);
    }
}

void liberar_ast(ASTNode* no) {
    if (no == NULL) return;
    liberar_ast(no->filho[0]);
    liberar_ast(no->filho[1]);
    liberar_ast(no->filho[2]);
    liberar_ast(no->prox);
    if (no->valor_lexico) free(no->valor_lexico);
    free(no);
}

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
        
        printf("\n--- Arvore Sintatica Abstrata (AST) ---\n");
        imprimir_ast(g_raiz_ast, 0);
        
        printf("\nAnalise sintatica bem-sucedida!\n");
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