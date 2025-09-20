/* goianinha.l - Analisador Léxico para a Linguagem Goianinha */

%{
/* --- Seção 1: Definições e Inclusões em C --- */

#include <stdio.h>
#include <string.h>

#include "tabela_simbolos.h"
#include "y.tab.h"

void yyerror(const char *s);
void reportar_erro_lexico(const char* mensagem) {
    fprintf(stderr, "ERRO: %s na linha %d\n", mensagem, yylineno);
}

%}

/* --- Seção de Opções e Definições do Flex --- */

%option yylineno
%option noyywrap

/* Definição de estado para lidar com comentários de bloco. */
%x COMMENT

/* Definições de nomes para expressões regulares */
LETRA           [a-zA-Z]
DIGITO          [0-9]
ID              ({LETRA}|_)({LETRA}|{DIGITO}|_)*
INTCONST        {DIGITO}+
CADEIA          \"([^\"\n\\]|\\.)*\"
CARCONST        \'([^\'\n\\]|\\.)\'

/* --- Seção 2: Regras --- */
%%

"/*"                    { BEGIN(COMMENT); }
<COMMENT>"*/"           { BEGIN(INITIAL); }
<COMMENT>.|\n           { /* Ignora tudo dentro do comentário */ }
<COMMENT><<EOF>>        { reportar_erro_lexico("COMENTARIO NAO TERMINA"); BEGIN(INITIAL); }

"programa"              { return T_PROGRAMA; }
"car"                   { return T_CAR; }
"int"                   { return T_INT; }
"retorne"               { return T_RETORNE; }
"leia"                  { return T_LEIA; }
"escreva"               { return T_ESCREVA; }
"novalinha"             { return T_NOVALINHA; }
"se"                    { return T_SE; }
"entao"                 { return T_ENTAO; }
"senao"                 { return T_SENAO; }
"enquanto"              { return T_ENQUANTO; }
"execute"               { return T_EXECUTE; }
"ou"                    { return T_OU; }
"e"                     { return T_E; }

{INTCONST}              { yylval.num_val = atoi(yytext); return T_INTCONST; }
{CADEIA}                { yylval.str_val = strdup(yytext); return T_CADEIA; }
{CARCONST}              { yylval.str_val = strdup(yytext); return T_CARCONST; }

{ID}                    { yylval.str_val = strdup(yytext); return T_ID; }

"=="                    { return T_EQ; }
"!="                    { return T_NE; }
">="                    { return T_GE; }
"<="                    { return T_LE; }
"+"                     { return T_SOMA; }
"-"                     { return T_SUB; }
"*"                     { return T_MULT; }
"/"                     { return T_DIV; }
"="                     { return T_ATRIB; }
"<"                     { return T_MENOR; }
">"                     { return T_MAIOR; }
"!"                     { return T_NEG; }
","                     { return T_VIRGULA; }
";"                     { return T_PVIRGULA; }
"("                     { return T_LPAREN; }
")"                     { return T_RPAREN; }
"{"                     { return T_LCHAVE; }
"}"                     { return T_RCHAVE; }

[ \t\r\n]+              { /* Ignora espaços em branco, tabs e novas linhas */ }

\"[^"\n]*\n             { reportar_erro_lexico("CADEIA DE CARACTERES OCUPA MAIS DE UMA LINHA"); }
.                       { reportar_erro_lexico("CARACTERE INVALIDO"); }

%%
/* --- Seção 3: Código C Adicional --- */

/* Função main() para permitir testes isolados.
 * Definir o macro GOIANINHA_PARSER para ignorar e evitar erro de duplicação.
 */
#ifndef GOIANINHA_PARSER
int main(int argc, char** argv) {
    if (argc > 1) {
        FILE *file = fopen(argv[1], "r");
        if (!file) {
            perror("Nao foi possivel abrir o arquivo");
            return 1;
        }
        yyin = file;
    }

    int token;
    printf("--- Iniciando Analise Lexica ---\n");
    while ((token = yylex()) != 0) {
        printf("Linha: %d, Lexema: '%s', Token: ", yylineno, yytext);
        /* Este switch precisará ser atualizado com os novos tokens (T_SOMA, etc) */
        if (token < 256) {
             printf("'%c'\n", token);
        } else {
             /* Apenas um exemplo, pode ser expandido */
             printf("%d (código)\n", token);
        }
    }
    printf("--- Fim da Analise Lexica ---\n");
    return 0;
}
#endif