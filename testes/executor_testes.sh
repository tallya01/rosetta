#!/bin/bash

# --- CONFIGURAÇÕES ---
DIRETORIO_ENTRADA="./programas_teste"
DIRETORIO_SAIDA="./resultados_teste"
EXECUTAVEL="../analisadores/goianinha"
EXTENSAO_SAIDA=".txt"
NOME_ASM_GERADO="saida.asm" # Nome fixo que o executável gera

# Cria o diretório de saída se ele não existir
mkdir -p "$DIRETORIO_SAIDA"

# Verifica se o executável existe
if [ ! -x "$EXECUTAVEL" ]; then
    echo "Erro: O executável '$EXECUTAVEL' não foi encontrado ou não tem permissão de execução."
    exit 1
fi

echo "Iniciando processamento..."

# --- LOOP PRINCIPAL ---
for arquivo_completo in "$DIRETORIO_ENTRADA"/*; do

    if [ -f "$arquivo_completo" ]; then
        
        # 1. Extrai o nome base e remove a extensão para uso nos nomes de saída
        nome_arquivo=$(basename -- "$arquivo_completo")
        nome_sem_ext="${nome_arquivo%.*}"
        
        # 2. Define o caminho do arquivo de log/saída padrão
        arquivo_saida="$DIRETORIO_SAIDA/${nome_sem_ext}_resultado${EXTENSAO_SAIDA}"
        
        # 3. Define o nome do novo arquivo .asm (baseado no pedido: nome original + _code.asm)
        # Nota: Isso criará o arquivo na mesma pasta do arquivo de entrada (ex: entrada/arquivo.in_code.asm)
        destino_asm="./$DIRETORIO_SAIDA/${nome_sem_ext}_code.asm"

        echo "Processando: $nome_arquivo"
        
        # --- EXECUÇÃO ---
        # Executa o programa
        "$EXECUTAVEL" "$arquivo_completo" > "$arquivo_saida" 2>&1
        
        # Verifica se a execução foi bem sucedida
        if [ $? -eq 0 ]; then
            
            # --- NOVA ETAPA: Copiar o saida.asm ---
            # Verifica se o executável realmente gerou o saida.asm no diretório atual
            if [ -f "$NOME_ASM_GERADO" ]; then
                cp "$NOME_ASM_GERADO" "$destino_asm"
                echo "  [OK] Sucesso. ASM copiado para: $destino_asm"
            else
                echo "  [AVISO] Compilação executada, mas o arquivo '$NOME_ASM_GERADO' não foi encontrado para cópia."
            fi
            
        else
            echo "  [ERRO] Erro de compilação para $nome_arquivo"
        fi
    fi
done

echo "Processamento concluído!"