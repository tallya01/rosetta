# Nome do compilador C
CC = gcc

# Nome do executável final
TARGET = goianinha

# Diretório onde os fontes da tabela de símbolos estão
TS_DIR = ../tabela_simbolos

# Ativa warnings, seta diretório da tabela e ignora função main do léxico
CFLAGS = -Wall -Wno-unused-function -I $(TS_DIR) -DGOIANINHA_PARSER

# Inclui a lib do Flex na linkagem
LDFLAGS = -lfl

# Arquivos de objeto (.o) que serão gerados
OBJS = y.tab.o lex.yy.o tabela_simbolos.o ast.o semantico.o gerador_codigo.o
# --------------------

# Regra padrão: compila tudo
all: $(TARGET)

# Regra para criar o executável final a partir dos arquivos de objeto
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

# Regra para gerar o parser e o header a partir do arquivo .y
y.tab.c y.tab.h: goianinha.y
	bison -d -o y.tab.c goianinha.y

# Regra para gerar o scanner a partir do arquivo .l
lex.yy.c: goianinha.l y.tab.h
	flex goianinha.l

# Regras para compilar os arquivos .c em .o
y.tab.o: y.tab.c $(TS_DIR)/tabela_simbolos.h ast.h semantico.h gerador_codigo.h
	$(CC) $(CFLAGS) -c $< -o $@

lex.yy.o: lex.yy.c
	$(CC) $(CFLAGS) -c $< -o $@

ast.o: ast.c ast.h
	$(CC) $(CFLAGS) -c $< -o $@

semantico.o: semantico.c semantico.h ast.h $(TS_DIR)/tabela_simbolos.h
	$(CC) $(CFLAGS) -c $< -o $@

gerador_codigo.o: gerador_codigo.c gerador_codigo.h ast.h
	$(CC) $(CFLAGS) -c $< -o $@

# Regra específica para compilar tabela_simbolos.o, buscando os fontes no diretório correto
tabela_simbolos.o: $(TS_DIR)/tabela_simbolos.c $(TS_DIR)/tabela_simbolos.h
	$(CC) $(CFLAGS) -c $< -o $@
# --------------------

# Regra para limpar os arquivos gerados
clean:
	rm -f $(TARGET) $(OBJS) y.tab.c y.tab.h lex.yy.c