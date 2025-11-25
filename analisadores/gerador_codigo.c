#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gerador_codigo.h"
#include "ast.h"
#include "tabela_simbolos.h"

// Variáveis globais para controle interno
static FILE* out;
static int label_counter = 0;
static int string_literal_counter = 0; // Para labels de strings
static ScopeStack* g_pilha_escopos_gerador = NULL;
static int g_offset_local = 0; // Controla o offset das variáveis locais no stack frame
static int decl_global_atual = 1; // Flag para saber se estamos em declaração global

// Protótipos de funções internas
void gerar_no(ASTNode* no);
void gerar_cabecalho();
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

// Função auxiliar para criar labels únicos
char* novo_label() {
    char* buffer = (char*)malloc(20);
    sprintf(buffer, "L%d", label_counter++);
    return buffer;
}

// --- INTEGRAÇÃO COM TABELA DE SÍMBOLOS ---
Symbol* obter_simbolo(char* nome) {
    return pesquisar_simbolo(g_pilha_escopos_gerador, nome);
}

// Função auxiliar para calcular o espaço necessário para variáveis locais em um escopo
int calcular_espaco_local(ASTNode* no) {
    if (no == NULL) return 0;

    int espaco = 0;
    ASTNode* atual = no;

    // Se o nó é um bloco, começamos pelas suas declarações
    if (atual->tipo == NO_BLOCO) {
        atual = atual->filho[0];
    }

    while (atual != NULL) {
        if (atual->tipo == NO_DECL_VAR) {
            espaco += 4; // Assumindo que cada variável ocupa 4 bytes
        } else if (atual->tipo == NO_SE) {
            // Para sub-blocos, calculamos o máximo espaço necessário, não a soma.
            int espaco_if = calcular_espaco_local(atual->filho[1]);
            int espaco_else = 0;
            if (atual->filho[2] != NULL) {
                espaco_else = calcular_espaco_local(atual->filho[2]);
            }
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
        return s->ordem; // A 'ordem' agora armazenará o offset
    }
    return 0; // Fallback
}
// ------------------------------------------------------------------------------

void gerar_codigo(ASTNode* raiz, FILE* saida, ScopeStack* pilha) {
    out = saida;
    if (!out) return;

    g_pilha_escopos_gerador = pilha;

    gerar_cabecalho(raiz);
    
    // A tabela de símbolos já está populada. Apenas gera o código.
    gerar_no(raiz);

    gerar_rodape();
}
void gerar_declaracoes_globais(ASTNode* no) {
    if (no == NULL) {
        decl_global_atual = 0;
        return;
    };
    
    if (no->tipo == NO_DECL_VAR) {
        ASTNode* id_node = no->filho[0];
        if (eh_global(g_pilha_escopos_gerador, id_node->valor_lexico)) {
            fprintf(out, "_%s: .word 0\n", id_node->valor_lexico);
        }
    } else if (no->tipo == NO_DECL_FUNC) {
        // Se encontrarmos uma função, geramos seu código completo aqui.
        // Isso garante que todas as funções sejam declaradas antes do main.
        gerar_funcao(no);
    } else {
        decl_global_atual = 0;
        return;
    }
    gerar_declaracoes_globais(no->prox);
}

void gerar_cabecalho(ASTNode* raiz) {
    fprintf(out, ".data\n");
    fprintf(out, "newline: .asciiz \"\\n\"\n");
    // Adiciona um literal para espaço, se necessário
    fprintf(out, "space: .asciiz \" \"\n");

    // Pré-scan para strings literais (se necessário, mas por agora vamos gerar dinamicamente)
    // ...

    gerar_declaracoes_globais(raiz->filho[0]); // Gera todas as variáveis e funções globais
    fprintf(out, ".text\n");
    fprintf(out, ".globl main\n\n");
}

void gerar_rodape() {
    // Funções auxiliares de runtime se necessário
}

// Despachante principal recursivo
void gerar_no(ASTNode* no) {
    if (no == NULL) return;

    switch(no->tipo) {
        case NO_PROGRAMA:
            gerar_no(no->filho[0]); // Declarações globais e funções
            
            // O bloco principal (Programa)
            if (no->filho[1] != NULL) {
                // Calcula o espaço local necessário para o escopo da main
                int espaco_vars = calcular_espaco_local(no->filho[1]);
                // Espaço para $ra, $fp (8 bytes) + variáveis, alinhado em 4 bytes
                int tamanho_frame = (espaco_vars + 8 + 3) & ~3;

                fprintf(out, "\nmain:\n");
                // Prólogo da main
                fprintf(out, "  addiu $sp, $sp, -%d\n", tamanho_frame);
                fprintf(out, "  sw $ra, %d($sp)\n", tamanho_frame - 4);
                fprintf(out, "  sw $fp, %d($sp)\n", tamanho_frame - 8);
                fprintf(out, "  move $fp, $sp\n");
                
                g_offset_local = 0; // Reseta offset para o main
                
                // Itera sobre a lista de comandos do bloco principal
                ASTNode* comando = no->filho[1];
                while (comando) {
                    gerar_no(comando);
                    comando = comando->prox;
                }

                // Epílogo da main
                fprintf(out, "  lw $ra, %d($sp)\n", tamanho_frame - 4);
                fprintf(out, "  lw $fp, %d($sp)\n", tamanho_frame - 8);
                fprintf(out, "  addiu $sp, $sp, %d\n", tamanho_frame);
                fprintf(out, "  li $v0, 10\n"); // Syscall exit
                fprintf(out, "  syscall\n");
            }
            break;

        case NO_DECL_VAR:
            // A geração de variáveis globais foi movida para o cabeçalho.
            // A geração de locais é tratada dentro dos blocos de função/main.
            gerar_declaracao_var(no); // Ainda necessário para locais
            break;
        
        case NO_DECL_FUNC:
            // A geração de funções foi movida para o cabeçalho.
            break;

        case NO_BLOCO:
            {
                int offset_anterior = g_offset_local;
                gerar_no(no->filho[0]); // Declarações locais
                gerar_no(no->filho[1]); // Comandos
                g_offset_local = offset_anterior; // Restaura offset do escopo pai
            }
            break;

        case NO_ATRIBUICAO:
            gerar_atribuicao(no);
            break;
        case NO_SE:
            gerar_if(no);
            break;
        case NO_ENQUANTO:
            gerar_while(no);
            break;
        case NO_ESCREVA:
        case NO_LEIA:
            gerar_io(no);
            break;
        case NO_CHAMADA_FUNC:
            gerar_chamada(no);
            break;
        case NO_RETORNE:
            gerar_expressao(no->filho[0]); // Resultado em $a0
            // O epílogo da função cuidará do 'jr $ra'
            // Aqui, garantimos que o valor de retorno está em $v0
            fprintf(out, "  move $v0, $a0\n");
            // Pula para o final da função (epílogo)
            // (Assumindo que cada função terá um label de fim)
            // fprintf(out, "  j %s_end\n", nome_da_funcao_atual);
            break;
            
        case NO_NOVALINHA:
            fprintf(out, "  li $v0, 4\n");      // Syscall para imprimir string
            fprintf(out, "  la $a0, newline\n"); // Carrega o endereço da string de nova linha
            fprintf(out, "  syscall\n");
            break;

        default:
            // Para outros tipos de nós que podem ser expressões (ex: operadores)
            // mas que são chamados no contexto de um comando (o que é raro, mas possível),
            // podemos simplesmente tentar avaliá-los como expressão.
            // Na maioria dos casos, isso não fará nada ou será inofensivo.
            gerar_expressao(no);
            break;
    }
    
    // Gera o próximo comando na lista
    if (no->prox != NULL) {
        gerar_no(no->prox);
    }
}

void gerar_declaracao_var(ASTNode* no) {
    ASTNode* id_node = no->filho[0];
    
    // Se o escopo atual é global (verificado pela tabela de símbolos)
    if (!decl_global_atual) { // Apenas para locais
        // O offset agora é positivo a partir de $fp.
        Symbol* s = obter_simbolo(id_node->valor_lexico);
        if (s) {
            // A análise semântica deveria pré-calcular isso.
            // Por enquanto, calculamos aqui.
            s->ordem = g_offset_local; 
            g_offset_local += 4; // Próxima variável terá um offset maior.
        }
    }
}


// Avalia expressões e coloca resultado em $a0
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

        case NO_ATRIBUICAO:
            // Uma atribuição dentro de uma expressão deve ser avaliada
            gerar_atribuicao(no);
            break;

        case NO_CHAMADA_FUNC:
            gerar_chamada(no);
            break;

        case NO_SOMA:
        case NO_SUB:
        case NO_MULT:
        case NO_DIV:
        case NO_IGUAL:
        case NO_DIF:
        case NO_MAIOR:
        case NO_MENOR:
        case NO_MAIOR_IGUAL:
        case NO_MENOR_IGUAL:
        case NO_E:
        case NO_OU:
            // Lado esquerdo (LHS)
            gerar_expressao(no->filho[0]);
            // Empilha o resultado do LHS
            fprintf(out, "  addiu $sp, $sp, -4\n");
            fprintf(out, "  sw $a0, 0($sp)\n");
            
            // Lado direito (RHS)
            gerar_expressao(no->filho[1]);
            
            // Desempilha o LHS para $t1
            fprintf(out, "  lw $t1, 0($sp)\n");
            fprintf(out, "  addiu $sp, $sp, 4\n");
            
            // Agora, $t1 = LHS, $a0 = RHS
            switch (no->tipo) {
                case NO_SOMA: fprintf(out, "  add $a0, $t1, $a0\n"); break;
                case NO_SUB:  fprintf(out, "  sub $a0, $t1, $a0\n"); break;
                case NO_MULT: fprintf(out, "  mul $a0, $t1, $a0\n"); break;
                case NO_DIV:  fprintf(out, "  div $t1, $a0\n  mflo $a0\n"); break;
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

        default:
            // Tipo de nó de expressão não tratado
            break;
    }
}

void gerar_atribuicao(ASTNode* no) {
    gerar_expressao(no->filho[1]);
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
    
    free(labelElse);
    free(labelEnd);
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
    
    free(labelIni);
    free(labelFim);
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
        // Se for uma string literal
        if (no->filho[0]->tipo == NO_CADEIA_CAR) {
            char* str_label = (char*)malloc(20);
            sprintf(str_label, "str%d", string_literal_counter++);
            
            // Adiciona a string ao segmento .data
            fprintf(out, ".data\n");
            fprintf(out, "%s: .asciiz %s\n", str_label, no->filho[0]->valor_lexico);
            fprintf(out, ".text\n");
            
            // Gera código para imprimir a string
            fprintf(out, "  li $v0, 4\n");
            fprintf(out, "  la $a0, %s\n", str_label);
            free(str_label);
        } else { // Se for uma expressão (int/car)
            gerar_expressao(no->filho[0]);
            fprintf(out, "  li $v0, 1\n"); // Syscall para imprimir inteiro
        }
        fprintf(out, "  syscall\n");
    }
}

void gerar_funcao(ASTNode* no) {
    char* nomeFunc = no->filho[0]->valor_lexico;
    
    // Calcula o espaço local necessário para o corpo da função
    int espaco_vars = calcular_espaco_local(no->filho[2]);
    // Espaço para $ra, $fp (8 bytes) + variáveis, alinhado em 4 bytes
    int tamanho_frame = (espaco_vars + 8 + 3) & ~3;

    fprintf(out, "\n%s:\n", nomeFunc);
    
    // Prólogo
    fprintf(out, "  addiu $sp, $sp, -%d\n", tamanho_frame);
    fprintf(out, "  sw $ra, %d($sp)\n", tamanho_frame - 4);
    fprintf(out, "  sw $fp, %d($sp)\n", tamanho_frame - 8);
    fprintf(out, "  move $fp, $sp\n");
    
    g_offset_local = 0; // Reset offset para vars locais da função
    
    // Processar corpo da função
    gerar_no(no->filho[2]);
    
    // Epílogo
    fprintf(out, "%s_end:\n", nomeFunc);
    fprintf(out, "  lw $ra, %d($sp)\n", tamanho_frame - 4);
    fprintf(out, "  lw $fp, %d($sp)\n", tamanho_frame - 8);
    fprintf(out, "  addiu $sp, $sp, %d\n", tamanho_frame);
    fprintf(out, "  jr $ra\n");
}

void gerar_chamada(ASTNode* no) {
    char* funcName = no->filho[0]->valor_lexico;
    ASTNode* arg = no->filho[1];
    int count = 0;
    
    // Empilha os argumentos na ordem inversa (padrão C)
    // Para simplificar, vamos empilhar na ordem direta.
    while (arg != NULL) {
        gerar_expressao(arg);
        fprintf(out, "  addiu $sp, $sp, -4\n");
        fprintf(out, "  sw $a0, 0($sp)\n");
        arg = arg->prox;
        count++;
    }
    
    fprintf(out, "  jal %s\n", funcName);
    
    if (count > 0) {
        fprintf(out, "  addiu $sp, $sp, %d\n", count * 4);
    }
    
    // O resultado da função (se houver) está em $v0.
    // Movemos para $a0 para consistência com o resto do gerador de expressão.
    fprintf(out, "  move $a0, $v0\n");
}
