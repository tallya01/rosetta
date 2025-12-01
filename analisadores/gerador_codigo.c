#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gerador_codigo.h"
#include "ast.h"
#include "tabela_simbolos.h"

// --- Variáveis globais ---
static FILE* out;
static int label_counter = 0;
static int string_literal_counter = 0;
static ScopeStack* g_pilha_escopos_gerador = NULL;
static int g_offset_local = 0;
static int decl_global_atual = 1;

// --- Protótipos ---
void gerar_no(ASTNode* no);
void gerar_cabecalho(ASTNode* raiz);
void gerar_declaracoes_globais(ASTNode* no);
void gerar_rodape();
void gerar_expressao(ASTNode* no);
void gerar_atribuicao(ASTNode* no);
void gerar_if(ASTNode* no);
void gerar_while(ASTNode* no);
void gerar_io(ASTNode* no);
void gerar_funcao(ASTNode* no);
void gerar_chamada(ASTNode* no);
void gerar_declaracao_var(ASTNode* no);
void empilhar_argumentos(ASTNode* arg, int* count); // Protótipo adicionado

// --- Auxiliares ---
char* novo_label() {
    char* buffer = (char*)malloc(20);
    sprintf(buffer, "L%d", label_counter++);
    return buffer;
}

Symbol* obter_simbolo(char* nome) {
    return pesquisar_simbolo(g_pilha_escopos_gerador, nome);
}

// Calcula tamanho das variáveis locais (excluindo parâmetros)
int calcular_espaco_local(ASTNode* no) {
    if (no == NULL) return 0;
    int espaco = 0;
    ASTNode* atual = no;

    // Se for um bloco, primeiro processa as declarações imediatas
    if (atual->tipo == NO_BLOCO) {
        ASTNode* decls = atual->filho[0];
        while (decls != NULL) {
             if (decls->tipo == NO_DECL_VAR) {
                 espaco += 4;
             }
             decls = decls->prox; // Avança para a próxima declaração na lista
        }
        // Depois processa comandos internos
        atual = no->filho[1];
    }

    // Percorre a lista de comandos (usando 'prox')
    while (atual != NULL) {
        if (atual->tipo == NO_SE) {
            int espaco_if = calcular_espaco_local(atual->filho[1]);
            int espaco_else = (atual->filho[2] != NULL) ? calcular_espaco_local(atual->filho[2]) : 0;
            espaco += (espaco_if > espaco_else) ? espaco_if : espaco_else;
        } else if (atual->tipo == NO_ENQUANTO) {
            espaco += calcular_espaco_local(atual->filho[1]);
        } else if (atual->tipo == NO_BLOCO) {
            espaco += calcular_espaco_local(atual);
        }
        
        atual = atual->prox;
    }
    return espaco;
}

int obter_offset(char* nome) {
    Symbol* s = obter_simbolo(nome);
    if (s) {
        return s->ordem; 
    }
    return 0; 
}

// --- Função Principal ---
void gerar_codigo(ASTNode* raiz, FILE* saida, ScopeStack* pilha) {
    out = saida;
    if (!out) return;
    g_pilha_escopos_gerador = pilha;

    gerar_cabecalho(raiz);
    gerar_no(raiz); // Gera o restante (incluindo main se estiver na árvore como nó)
    gerar_rodape();
}

void gerar_declaracoes_globais(ASTNode* no) {
    if (no == NULL) {
        decl_global_atual = 0;
        return;
    };
    
    // Lista de Declarações Globais (DeclFuncVar)
    // No Yacc: DeclFuncVar -> ... | DeclFunc ...
    
    if (no->tipo == NO_DECL_VAR) {
        ASTNode* id_node = no->filho[1]; // No Yacc: filho[0]=Tipo, filho[1]=ID
        fprintf(out, "_%s: .word 0\n", id_node->valor_lexico);
        
        // Usa o ponteiro 'prox' para a próxima declaração global
        if (no->prox != NULL) gerar_declaracoes_globais(no->prox);
        
    } else if (no->tipo == NO_DECL_FUNC) {
        gerar_funcao(no);
        // Se houver mais declarações após a função (depende da estrutura DeclFuncVar)
        if (no->prox != NULL) gerar_declaracoes_globais(no->prox); 
    } else {
        // Pode ser DeclProg (Programa) ou fim
        decl_global_atual = 0;
        return;
    }
}

void gerar_cabecalho(ASTNode* raiz) {
    fprintf(out, ".data\n");
    fprintf(out, "newline: .asciiz \"\\n\"\n");
    fprintf(out, "space: .asciiz \" \"\n");

    // Gera variáveis globais e funções antes do main
    // O filho[0] de Programa é "DeclFuncVar"
    if (raiz && raiz->filho[0]) {
        gerar_declaracoes_globais(raiz->filho[0]);
    }

    fprintf(out, ".text\n");
    fprintf(out, ".globl main\n\n");
}

void gerar_rodape() {
    // Código auxiliar final
}

void gerar_no(ASTNode* no) {
    if (no == NULL) return;

    switch(no->tipo) {
        case NO_PROGRAMA:
            // Globais já foram processadas no cabeçalho.
            if (no->filho[1] != NULL) {
                ASTNode* blocoMain = no->filho[1]; 
                
                int espaco_vars = calcular_espaco_local(blocoMain);
                int tamanho_frame = (espaco_vars + 8 + 3) & ~3;

                fprintf(out, "\nmain:\n");
                fprintf(out, "  addiu $sp, $sp, -%d\n", tamanho_frame);
                fprintf(out, "  sw $ra, %d($sp)\n", tamanho_frame - 4);
                fprintf(out, "  sw $fp, %d($sp)\n", tamanho_frame - 8);
                fprintf(out, "  move $fp, $sp\n");
                
                g_offset_local = 0; 
                
                // Processa o bloco principal, que já lida com suas sub-partes
                if (blocoMain->tipo == NO_BLOCO) {
                    gerar_no(blocoMain); 
                }

                fprintf(out, "  lw $ra, %d($sp)\n", tamanho_frame - 4);
                fprintf(out, "  lw $fp, %d($sp)\n", tamanho_frame - 8);
                fprintf(out, "  addiu $sp, $sp, %d\n", tamanho_frame);
                fprintf(out, "  li $v0, 10\n"); 
                fprintf(out, "  syscall\n");
            }
            break;

        case NO_DECL_VAR:
            gerar_declaracao_var(no);
            if (no->prox != NULL) gerar_no(no->prox);
            break;
        
        case NO_DECL_FUNC:
            // Já gerado via declarações globais
            break;

        case NO_BLOCO:
            {
                int offset_anterior = g_offset_local;
                gerar_no(no->filho[0]); // Declarações
                
                ASTNode* stmt = no->filho[1]; // Comandos
                while(stmt) {
                    gerar_no(stmt);
                    stmt = stmt->prox;
                }
                g_offset_local = offset_anterior;
            }
            break;

        case NO_ATRIBUICAO: gerar_atribuicao(no); break;
        case NO_SE: gerar_if(no); break;
        case NO_ENQUANTO: gerar_while(no); break;
        case NO_ESCREVA: case NO_LEIA: gerar_io(no); break;
        case NO_CHAMADA_FUNC: gerar_chamada(no); break;
        
        case NO_RETORNE:
            gerar_expressao(no->filho[0]); 
            fprintf(out, "  move $v0, $a0\n"); 
            break;
            
        case NO_NOVALINHA:
            fprintf(out, "  li $v0, 4\n");
            fprintf(out, "  la $a0, newline\n");
            fprintf(out, "  syscall\n");
            break;

        default:
            gerar_expressao(no);
            break;
    }
}

void gerar_declaracao_var(ASTNode* no) {
    ASTNode* id_node = no->filho[1]; // filho[0]=tipo, filho[1]=id
    if (!decl_global_atual) { 
        Symbol* s = obter_simbolo(id_node->valor_lexico);
        if (s) {
            s->ordem = g_offset_local; 
            g_offset_local += 4; 
        }
    }
}

void gerar_expressao(ASTNode* no) {
    if (no == NULL) return;

    switch (no->tipo) {
        case NO_INT_CONST:
            fprintf(out, "  li $a0, %d\n", no->valor_int);
            break;

        case NO_ID:
            if (eh_global(g_pilha_escopos_gerador, no->valor_lexico)) {
                fprintf(out, "  lw $a0, _%s\n", no->valor_lexico);
            } else {
                int offset = obter_offset(no->valor_lexico);
                fprintf(out, "  lw $a0, %d($fp)\n", offset);
            }
            break;

        case NO_ATRIBUICAO: gerar_atribuicao(no); break;
        case NO_CHAMADA_FUNC: gerar_chamada(no); break;

        case NO_SOMA: case NO_SUB: case NO_MULT: case NO_DIV:
        case NO_IGUAL: case NO_DIF: case NO_MAIOR: case NO_MENOR:
        case NO_MAIOR_IGUAL: case NO_MENOR_IGUAL: case NO_E: case NO_OU:
            gerar_expressao(no->filho[0]);
            fprintf(out, "  addiu $sp, $sp, -4\n");
            fprintf(out, "  sw $a0, 0($sp)\n");
            
            gerar_expressao(no->filho[1]);
            
            fprintf(out, "  lw $t1, 0($sp)\n");
            fprintf(out, "  addiu $sp, $sp, 4\n");
            
            switch (no->tipo) {
                case NO_SOMA: fprintf(out, "  add $a0, $t1, $a0\n"); break;
                case NO_SUB:  fprintf(out, "  sub $a0, $t1, $a0\n"); break;
                case NO_MULT: fprintf(out, "  mul $a0, $t1, $a0\n"); break;
                case NO_DIV:  
                    fprintf(out, "  div $zero, $t1, $a0\n"); 
                    fprintf(out, "  mflo $a0\n"); 
                    break;
                case NO_IGUAL: fprintf(out, "  seq $a0, $t1, $a0\n"); break;
                case NO_DIF:   fprintf(out, "  sne $a0, $t1, $a0\n"); break;
                case NO_MAIOR: fprintf(out, "  sgt $a0, $t1, $a0\n"); break;
                case NO_MENOR: fprintf(out, "  slt $a0, $t1, $a0\n"); break;
                case NO_MAIOR_IGUAL: fprintf(out, "  sge $a0, $t1, $a0\n"); break;
                case NO_MENOR_IGUAL: fprintf(out, "  sle $a0, $t1, $a0\n"); break;
                case NO_E: fprintf(out, "  and $a0, $t1, $a0\n"); break;
                case NO_OU: fprintf(out, "  or $a0, $t1, $a0\n"); break;
                default: break;
            }
            break;
        default: break;
    }
}

void gerar_atribuicao(ASTNode* no) {
    gerar_expressao(no->filho[1]); // Valor em $a0
    ASTNode* idNode = no->filho[0];
    if (eh_global(g_pilha_escopos_gerador, idNode->valor_lexico)) {
        fprintf(out, "  sw $a0, _%s\n", idNode->valor_lexico);
    } else {
        int offset = obter_offset(idNode->valor_lexico);
        fprintf(out, "  sw $a0, %d($fp)\n", offset);
    }
}

void gerar_if(ASTNode* no) {
    char* labelElse = novo_label();
    char* labelEnd = novo_label();
    
    gerar_expressao(no->filho[0]);
    fprintf(out, "  beqz $a0, %s\n", labelElse);
    
    gerar_no(no->filho[1]);
    fprintf(out, "  j %s\n", labelEnd);
    
    fprintf(out, "%s:\n", labelElse);
    if (no->filho[2] != NULL) {
        gerar_no(no->filho[2]);
    }
    
    fprintf(out, "%s:\n", labelEnd);
    free(labelElse); free(labelEnd);
}

void gerar_while(ASTNode* no) {
    char* labelIni = novo_label();
    char* labelFim = novo_label();
    
    fprintf(out, "%s:\n", labelIni);
    gerar_expressao(no->filho[0]);
    fprintf(out, "  beqz $a0, %s\n", labelFim);
    gerar_no(no->filho[1]);
    fprintf(out, "  j %s\n", labelIni);
    fprintf(out, "%s:\n", labelFim);
    
    free(labelIni); free(labelFim);
}

void gerar_io(ASTNode* no) {
    if (no->tipo == NO_LEIA) {
        fprintf(out, "  li $v0, 5\n");
        fprintf(out, "  syscall\n");
        ASTNode* idNode = no->filho[0];
        if (eh_global(g_pilha_escopos_gerador, idNode->valor_lexico)) {
            fprintf(out, "  sw $v0, _%s\n", idNode->valor_lexico);
        } else {
            int offset = obter_offset(idNode->valor_lexico);
            fprintf(out, "  sw $v0, %d($fp)\n", offset);
        }
    } 
    else if (no->tipo == NO_ESCREVA) {
        if (no->filho[0]->tipo == NO_CADEIA_CAR) { 
            char* str_label = (char*)malloc(20);
            sprintf(str_label, "str%d", string_literal_counter++);
            fprintf(out, ".data\n");
            fprintf(out, "%s: .asciiz %s\n", str_label, no->filho[0]->valor_lexico);
            fprintf(out, ".text\n");
            fprintf(out, "  li $v0, 4\n");
            fprintf(out, "  la $a0, %s\n", str_label);
            free(str_label);
        } else { 
            gerar_expressao(no->filho[0]);
            fprintf(out, "  li $v0, 1\n");
        }
        fprintf(out, "  syscall\n");
    }
}

void gerar_funcao(ASTNode* no) {
    char* nomeFunc = no->filho[0]->valor_lexico;
    
    // filho[2] é o Bloco da função
    int espaco_vars = calcular_espaco_local(no->filho[2]);
    int tamanho_frame = (espaco_vars + 8 + 3) & ~3;

    fprintf(out, "\n%s:\n", nomeFunc);
    
    // Prólogo
    fprintf(out, "  addiu $sp, $sp, -%d\n", tamanho_frame);
    fprintf(out, "  sw $ra, %d($sp)\n", tamanho_frame - 4);
    fprintf(out, "  sw $fp, %d($sp)\n", tamanho_frame - 8);
    fprintf(out, "  move $fp, $sp\n");
    
    // Mapeamento de Parâmetros
    // filho[1] é a ListaParametros
    ASTNode* params = no->filho[1];
    int param_idx = 0;

    // Empilhamento ArgN..Arg1.
    
    int param_offset_base = tamanho_frame;

    while (params != NULL) {
        // params é um nó NO_DECL_VAR: filho[0]=Tipo, filho[1]=ID, filho[2]=Prox
        if (params->tipo == NO_DECL_VAR) {
             ASTNode* idParam = params->filho[1];
             if (idParam) {
                 Symbol* s = obter_simbolo(idParam->valor_lexico);
                 if (s) {
                     s->ordem = param_offset_base + (param_idx * 4);
                     param_idx++;
                 }
             }
             params = params->prox; // Avança na lista encadeada
        } else {
             // Caso a lista esteja vazia ou formato inesperado
             break;
        }
    }

    g_offset_local = 0; 
    
    // Gera corpo da função (Bloco)
    gerar_no(no->filho[2]);
    
    // Epílogo
    fprintf(out, "%s_end:\n", nomeFunc);
    fprintf(out, "  lw $ra, %d($sp)\n", tamanho_frame - 4);
    fprintf(out, "  lw $fp, %d($sp)\n", tamanho_frame - 8);
    fprintf(out, "  addiu $sp, $sp, %d\n", tamanho_frame);
    fprintf(out, "  jr $ra\n");
}

void empilhar_argumentos(ASTNode* arg, int* count) {
    if (arg == NULL) return;
    
    // filho[0] = Esquerda (Resto da lista), filho[1] = Direita (Expr atual)
    
    if (arg->tipo == NO_LISTA) {
        // Primeiro empilha o da Direita
        gerar_expressao(arg->filho[1]);
        fprintf(out, "  addiu $sp, $sp, -4\n");
        fprintf(out, "  sw $a0, 0($sp)\n");
        (*count)++;
        
        // Depois processa a Esquerda recursivamente
        empilhar_argumentos(arg->filho[0], count);
        
    } else {
        // Base case: apenas uma Expr (primeiro argumento da lista original)
        gerar_expressao(arg);
        fprintf(out, "  addiu $sp, $sp, -4\n");
        fprintf(out, "  sw $a0, 0($sp)\n");
        (*count)++;
    }
}

void gerar_chamada(ASTNode* no) {
    char* funcName = no->filho[0]->valor_lexico;
    ASTNode* arg = no->filho[1]; // ListExpr
    int count = 0;
    
    empilhar_argumentos(arg, &count);
    
    fprintf(out, "  jal %s\n", funcName);
    
    if (count > 0) {
        fprintf(out, "  addiu $sp, $sp, %d\n", count * 4);
    }
    
    fprintf(out, "  move $a0, $v0\n");
}