# SPEC.md - Especificação Técnica: MSX dBASE Clone (dBASE II+ / III- for MSX)

## 1. Visão Geral do Projeto
O objetivo deste projeto é desenvolver um gerenciador de banco de dados e ambiente de programação relacional totalmente compatível com o **dBASE II+** (padrão CP/M), evoluindo progressivamente para suportar recursos do **dBASE III / III+** (padrão MS-DOS/xBase).

O software é projetado especificamente para rodar no **MSX 2+** com armazenamento massivo (HD/SD) via **MSX-DOS 2**, otimizado para CPUs Z80 operando em overclock (7 a 10 MHz) e utilizando até 512 KB de RAM Mapeada (Memory Mapper).

---

## 2. Arquitetura de Hardware e Software

### 2.1 Público-Alvo de Hardware (Target System)
* **Plataforma:** MSX 2+ (compatível com MSX2, MSX2+ e MSX turboR).
* **Processador (CPU):** Zilog Z80A (3.58 MHz nominal, com suporte otimizado a acelerações de 5, 7 e 10 MHz).
* **Memória RAM:** Mínimo de 256 KB / Recomendado 512 KB de RAM Mapeada (*Memory Mapper* padrão MSX).
* **VDP / Vídeo:** Yamaha V9958 (Modo Texto de 80 colunas / TUI avançada).
* **Sistema Operacional:** MSX-DOS 2.x (suporte a pastas/diretórios e gerenciamento de arquivos estendido).
* **Armazenamento:** Dispositivos de acesso direto rápidos (Nextor / IDE / MegaFlashROM SD / Sunrise IDE).

### 2.2 Stack de Desenvolvimento (Toolchain)
* **Linguagem Principal:** C (padrão C89/C99 suportado pelo SDCC).
* **Compilador:** **SDCC** (Small Device C Compiler), configurado para o target Z80.
* **Biblioteca de Hardware:** **MSXgl** (C Library for MSX family), utilizada para:
  * Controle direto e rápido da VRAM do V9958 em Modo Texto (80 colunas).
  * Gerenciamento do *Memory Mapper* (troca de páginas de RAM estendida).
  * Leitura e manipulação de entrada de teclado e rotinas de tempo.
  * Injeção de código Assembly Z80 inline (`__asm`) para rotinas críticas de performance.
* **Saída de Compilação:** Executável binário nativo para MSX-DOS 2 (`.COM`).

---

## 3. Diretrizes de Arquitetura de Software

### 3.1 Design da Janela de Memória do Z80 (64 KB Limit)
Como o Z80 possui barramento de endereçamento de 16 bits (64 KB visíveis por vez):
* **Página 0 (`0x0000 - 0x3FFF`):** Reservado para o MSX-DOS 2 e BIOS.
* **Página 1 (`0x4000 - 0x7FFF`):** Código principal do programa (Parser, Interpretador CLI, TUI Engine).
* **Página 2 (`0x8000 - 0xBFFF`):** Mapeamento dinâmico via *Memory Mapper* (Buffers de Registros `.dbf`, Caches de Índices e Buffers de Disco).
* **Página 3 (`0xC000 - 0xFFFF`):** RAM do sistema MSX-DOS, Pilha (*Stack*), Variáveis Globais da Aplicação.

### 3.2 Formato de Dados e Compatibilidade
* **Estrutura `.dbf` (dBASE II):**
  * Cabeçalho clássico dBASE II (registros de tamanho fixo, definição de tipos de campos N/C/L, limite de 32 campos por registro).
  * Compatibilidade byte a byte com arquivos criados no dBASE II para CP/M.
* **Indexação (Fases Iniciais vs. Avançadas):**
  * *Fase 1/2:* Varredura sequencial em memória otimizada com cache em RAM Mapper.
  * *Fase 3/4:* Implementação de Árvore B (*B-Tree*) compatível com o formato de índice `.ndx` (dBASE III).

---

## 4. Roteiro de Desenvolvimento (Fases de Evolução)
+-----------------------------------------------------------+
| FASE 1: dBASE II+ Core & Compatibility (CLI)              |
| - Parser de comandos + Leitura/Escrita de .dbf            |
+-----------------------------------------------------------+
|
v
+-----------------------------------------------------------+
| FASE 2: dBASE II++ Visual Revolution (TUI & Assistant)     |
| - Modo Texto 80 col, Janelas, Cores, Form @...SAY/GET      |
| - Menu estilo "Assistant" (inspirado no dBASE III+)       |
| - Sistema de HELP expansivo (F1)                          |
+-----------------------------------------------------------+
|
v
+-----------------------------------------------------------+
| FASE 3: dBASE III- (B-Tree Indexing .ndx)                 |
| - Comandos INDEX ON, SEEK, FIND, SET INDEX TO             |
| - Motor de busca relacional acelerado                     |
+-----------------------------------------------------------+
|
v
+-----------------------------------------------------------+
| FASE 4: dBASE III / III+ Full Features                    |
| - Áreas de trabalho simultâneas (SELECT 1, 2...), RELATE  |
| - Suporte opcional a campos de Memo (.dbt)                |
+-----------------------------------------------------------+

### Detalhamento das Fases:

#### **Fase 1: `dBASE II+` (Núcleo de Compatibilidade)**
* Implementar o parser CLI com prompt do ponto (`.`).
* Compatibilidade 100% com scripts `.prg` e tabelas `.dbf` do dBASE II de CP/M.
* Suporte aos comandos estruturais: `USE`, `DISPLAY`, `LIST`, `APPEND`, `REPLACE`, `DELETE`, `PACK`, `LOCATE`, `STORE`, `DO WHILE`, `IF...ENDIF`.

#### **Fase 2: `dBASE II++` (Interface & Usabilidade)**
* Implementação da TUI gráfica via caracteres de quadro em modo de 80 colunas.
* Criação de um menu interativo de topo (estilo *Assistant* do dBASE III Plus) navegável via setas do teclado MSX.
* Formulários de tela aprimorados para o comando `APPEND` e suporte avançado para `@ <lin>,<col> SAY ... GET`.
* Sistema de ajuda contextual estendido (`HELP <comando>` ou tecla `F1`).

#### **Fase 3: `dBASE III-` (Motor de Índices)**
* Suporte à estrutura de índices baseados em Árvore B (`.ndx`).
* Comandos de busca ultrarrápidos: `INDEX ON`, `SEEK`, `FIND`.
* Otimização de busca em memória aproveitando os 512 KB de RAM Mapper como cache de nós da árvore B.

#### **Fase 4: `dBASE III+` (Recursos Avançados)**
* Múltiplas áreas de trabalho abertas (`SELECT 1`, `SELECT 2`).
* Vinculação entre tabelas (`SET RELATION TO`).
* Funções estendidas de manipulação de data, hora e manipulação de texto.

---

## 5. Convenções de Código para Assistentes de IA (Copilot / Gemini)

Ao gerar código para este repositório, os assistentes de IA devem seguir estritamente as regras:

1. **Eficiência Z80:** Preferir tipos de dados de 8 bits (`uint8_t`, `int8_t`) em vez de 16/32 bits sempre que possível para economizar registradores e ciclos da CPU Z80.
2. **Ponteiros e Memória:** Minimizar o uso de alocação dinâmica pesada (`malloc`/`free`). Preferir buffers estáticos mapeados nas páginas estendidas via rotinas da MSXgl.
3. **Padrão C:** Escrever código C compatível com **SDCC**. Evitar sintaxes modernas do C11/C99 não suportadas pelo compilador.
4. **Tratamento de Hardware:** Toda a manipulação de VRAM e paginação deve utilizar as macros/funções da biblioteca **MSXgl** ou blocos `__asm` devidamente comentados.

