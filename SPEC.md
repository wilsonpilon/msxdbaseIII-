# SPEC.md - Especificação Técnica: MSX dBASE Clone (dBASE II+ / III- for MSX)

## 1. Visão Geral do Projeto
O objetivo deste projeto é desenvolver um gerenciador de banco de dados e ambiente de programação relacional totalmente compatível com o **dBASE II+** (padrão CP/M), evoluindo progressivamente para suportar recursos do **dBASE III / III+** (padrão MS-DOS/xBase).

O software é projetado especificamente para rodar no **MSX 2+** com armazenamento massivo (HD/SD) via **MSX-DOS 2**, otimizado para CPUs Z80 operando em overclock (7 a 10 MHz) e utilizando até 512 KB de RAM Mapeada (Memory Mapper).

---

## 2. Arquitetura de Hardware e Software

### 2.1 Público-Alvo de Hardware (Target System)
* **Plataforma:** MSX 2+ (compatível com MSX2, MSX2+ e MSX turboR).
* **Processador (CPU):** Zilog Z80A (3.58 MHz nominal, com suporte otimizado a acelerações de 5, 7 e 10 MHz).
* **Memória RAM:** Mínimo absoluto de 512 KB de RAM Mapeada (*Memory Mapper* padrão MSX). **Alvo real: 1024 KB (1 MB)**, para rodar esta versão do dBASE com conforto (decisão de 2026-10-26; ver seção 6.3).
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
5. **Documentos vivos:** ao final de cada sessão atualizar `OUTLINE.md` (progresso), `resume.txt` (resumo do dia e próximos passos) e, se houver decisão nova, este `SPEC.md`.
6. **Um marco por vez:** manter o projeto compilável após cada etapa e informar impacto em RAM, disco e desempenho ao adicionar funcionalidade.
7. **Código didático:** o projeto também serve para estudo; preferir módulos pequenos, nomes claros e comentários que expliquem o "porquê".

---

## 6. Diretrizes de Produto e Arquitetura (consolidadas de DBASE_MSX_DIRETRIZES.md)

> Esta parte incorpora o antigo `DBASE_MSX_DIRETRIZES.md`. Em conflito com as seções 2 e 3, **prevalece esta seção**, que é mais recente. O arquivo original foi mantido apenas como histórico.

### 6.1 Visão
Um **xBase nativo para MSX** ("dBASE 2.5/3 para MSX"): compatível com dBASE II onde viável, com recursos selecionados do dBASE III, usando MSX2/2+/turbo R, MSX-DOS 2, Memory Mapper e armazenamento moderno, com interface própria do MSX. É um ambiente integrado de banco de dados e programação, não apenas um visualizador de DBF. Não é um porte do dBASE II+ para CP/M nem uma reprodução integral do dBASE III.

### 6.2 Decisões que permanecem explícitas
1. Não portar literalmente o dBASE III.
2. Não se limitar à experiência CP/M.
3. Preservar compatibilidade xBase onde trouxer valor.
4. Escolher recursos do dBASE III por utilidade e custo.
5. Usar Memory Mapper e modularização, nunca um executável monolítico.
6. Separar engine, linguagem, UI e plataforma.
7. Priorizar DBF íntegro antes de interface sofisticada.
8. Índices e memo vêm depois do núcleo DBF.
9. Manter sempre o modo console, mesmo com UI avançada.
10. Projetar para hardware real, não só emuladores.
11. SDCC + MSXgl é o caminho principal; Fusion-C é só alternativa.
12. Documentar extensões MSX separadamente da compatibilidade dBASE.
13. Não assumir memória linear por causa do Memory Mapper.
14. Nunca sacrificar integridade de dados por otimização prematura.

### 6.3 Plataforma e perfis
**Decisão (2026-10-26): o mínimo absoluto é 512 KB de Memory Mapper; o alvo é 1024 KB.** Perfis com 128/256 KB deixam de ser objetivo e não devem limitar o projeto. Pode-se usar mais RAM mapeada para cache de índices, buffers de registros, bytecode e overlays, sem se preocupar com máquinas pequenas.
- Alvo: MSX2+/turbo R, Z80 3,58 MHz ou mais (overclock bem-vindo), MSX-DOS 2, Memory Mapper 1 MB, HDD/SD/IDE/Nextor.
- Mínimo: MSX2 com 512 KB de mapper, modo texto 80 colunas.
- O Z80 continua limitado a 64 KB lógicos por vez (ver 6.4); a RAM extra é acessada por segmentos de 16 KB.

### 6.4 Memória do Z80 (64 KB lógicos)
Mesmo com 512 KB ou 1 MB instalados, o Z80 enxerga 64 KB por vez. Toda alocação importante deve considerar paginação, troca de segmentos (16 KB) e custo de cópia.
- Residente: núcleo de I/O, gerenciador de memória, laço principal, parser mínimo, dispatcher, estado das áreas de trabalho, UI básica, tratamento de erros.
- Candidatos a overlay/segmento mapeado: relatórios, editor de comandos, ordenação, manutenção de índices, memo, ajuda online, UI avançada, comandos raros, compilador de bytecode.

### 6.5 Arquitetura em camadas
```text
Interface MSX: prompt, editor, menus, janelas
Interpretador xBase / parser / máquina virtual
Comandos, expressões, variáveis e runtime
Áreas de trabalho e engine de registros
DBF | NDX | DBT/Memo | filtros | relatórios
Cache, buffers, mapper, overlays e memória
MSXgl / abstração de plataforma / MSX-DOS
BIOS, BDOS, VDP, teclado, mouse e armazenamento
```
Regras: a engine DBF e o interpretador **não dependem** da biblioteca de UI nem de MSXgl diretamente; usam a camada `platform` (arquivos, mapper, console, teclado). Isso permite testes no PC e troca de biblioteca.

Módulos planejados: `core` (runtime, erros, valores), `parser` (lexer, parser, compiler), `vm` (bytecode, executor), `dbf`, `index` (ndx), `memo` (dbt), `workarea` (áreas, filtro, relações), `commands`, `ui` (console, editor, janelas, menus, mouse), `platform` (dos, mapper, vdp, teclado, arquivos), `tests`.

### 6.6 Linguagem
Caminho: `.PRG` → lexer → tokens → parser → bytecode compacto → máquina virtual → engine. No modo interativo, cada linha é compilada para um buffer temporário e executada. Evitar cadeias de comparação de strings para comandos (a tabela de comandos atual é transitória, até o lexer existir). `.PRG` deve evoluir para: sequências, variáveis, expressões, IF/DO WHILE, procedimentos, parâmetros e retorno.

### 6.7 Tipos de dados
Fase 1: Character, Numeric, Logical. Fase 2: Date. Fase 3: Memo (`.DBT`). Evitar ponto flutuante; avaliar decimal fixo ou BCD para Numeric. Tratar largura, casas decimais, espaços, sinal, overflow, vazio, comparação e conversão texto/número.

### 6.8 Arquivos DBF
- **Variante implementada primeiro: dBASE III sem memo (versão 0x03; 0x83 também aceita no cabeçalho)**, por ser o formato mais documentado e interoperável. O suporte ao dBASE II clássico (versão 0x02, cabeçalho de 8 bytes) continua exigido pela seção 3.2 e fica para um marco próprio.
- Cuidados: variante exata, cabeçalho, descritores, tamanho do registro, marca de exclusão, fim lógico (0x1A), codificação, integridade após interrupção, gravação segura do cabeçalho e da contagem de registros.
- Segurança em operações destrutivas (`PACK`, `SORT`, `INDEX`, reconstrução, memo): usar arquivos temporários, validar espaço livre, preservar o original até o fim, permitir recuperação e não atualizar metadados críticos cedo demais.
- Limites atuais da engine: 32 campos e registro de até 256 bytes (buffer estático). Ampliar quando houver necessidade e medição de RAM.

### 6.9 Índices NDX
Etapa 1: leitura de índice simples, `FIND`/`SEEK`, navegação ordenada, uma chave. Etapa 2: criação, atualização após APPEND/REPLACE/DELETE, vários índices, expressões de chave. Etapa 3: cache, reconstrução, detecção de inconsistência, validação da árvore. Nunca carregar o índice inteiro na RAM: cache de páginas e leitura sob demanda.

### 6.10 Versionamento e releases
- Versão `X.Y`: `X` identifica um marco funcional com codinome inspirado em Unix/C clássicos; `Y` avança após cada compilação bem-sucedida.
- `build.ps1 -KeepVersion` permite recompilar sem avançar `Y` durante uma rodada de testes; a operação `clean` também não altera a versão.
- O `X` atual e seu codinome ficam em `project/version.h`. O comando `VER` mostra a versão incorporada ao executável.
- Tags Git usam `X.Y`. Títulos de releases usam `<codinome>: <descrição curta>` e são registrados em `RELEASE.md`; `Changelog.MD` contém o resumo por versão.
- Codinomes não usam nomes de pessoas. Série inicial: `1 = Shell`.

### 6.10 Memo (DBT)
Desejável, mas não bloqueia o MVP. Separar da engine DBF, definir blocos e alocação, tratar fragmentação, validar referências, ter ferramenta de verificação/reconstrução, não manter textos completos residentes e editar por páginas.

### 6.11 Áreas de trabalho
MVP: 2 áreas (PRIMARY/SECONDARY), `SELECT`. Evolução: 4 áreas e até 10 conforme memória. Cada área guarda: DBF, registro atual, BOF/EOF, filtro, índice ativo, lista de índices, buffer, marca de alteração, alias, relações.

### 6.12 Comandos por grupo
- Essencial: `USE CLOSE SELECT LIST DISPLAY APPEND DELETE RECALL PACK REPLACE GO SKIP LOCATE CONTINUE COUNT SUM AVERAGE SET DELETED SET EXACT SET FILTER TO`
- Índices: `INDEX ON  SET INDEX TO  FIND  SEEK  REINDEX`
- Programação: `DO  DO WHILE  ENDDO  IF  ELSE  ENDIF  STORE  RELEASE  RETURN  PARAMETERS  PROCEDURE`
- Ferramentas: `CREATE MODIFY STRUCTURE MODIFY COMMAND DIR COPY SORT TOTAL JOIN REPORT LABEL`
- Inspirados no dBASE III: Date, Memo, mais áreas, aliases, procedimentos estruturados, expressões ricas, editor integrado.
Cada comando deve ter especificação própria: sintaxe, parâmetros, comportamento, erros, interação com filtro e índice, consumo de memória, compatibilidade e testes.

### 6.13 Interface nativa MSX
Desejado: cores configuráveis, setas, Home/End/PgUp/PgDn, edição de linha, histórico, janelas, menus, caracteres gráficos, caixas de diálogo, barra de estado, ajuda contextual, mouse opcional, 40 e 80 colunas. A UI não pode contaminar a engine DBF nem o interpretador. Deve sempre existir modo console (máquinas menores, automação, `.PRG`, depuração, testes).

### 6.14 Desempenho
Gargalos esperados: disco, índices, troca de segmentos, parsing repetitivo, ordenação, cópia de registros, pouca RAM linear. Estratégias: buffers alinhados a setor/bloco, cache pequeno e previsível, leitura antecipada, gravação agrupada onde for seguro, bytecode, índices por páginas, overlays, evitar gráficos em lote, modo silencioso, medir em hardware real e emulador.

### 6.15 Extensões MSX
Nomes propostos (não fechados): `SET COLOR TO`, `SET SCREEN TO`, `SET MOUSE ON`, `SET WINDOW TO`, `MSXINFO()`, `MAPPERFREE()`. Devem ser isoladas e documentadas à parte; um programa só com o subconjunto compatível deve ser portável.

### 6.16 Testes obrigatórios
- Formato: DBF vazio, com excluídos, campos no limite, negativos, decimais, registro truncado, cabeçalho corrompido, EOF ausente, arquivo maior que a RAM.
- Integridade: interrupção em APPEND/PACK, disco cheio, somente leitura, índice desatualizado, DBT ausente, referência de memo inválida.
- Plataforma: MSX2, MSX2+, turbo R; 512 KB e 1 MB de mapper; MSX-DOS 1 (se suportado) e 2; interfaces de armazenamento; emuladores e hardware real.
- Golden files: bancos pequenos conhecidos, comparados byte a byte após operações. O primeiro é `project/data/CLIENTES.DBF` (gerado por `project/data/make_sample_dbf.ps1`).

### 6.17 Critérios para aceitar uma funcionalidade
1. É núcleo dBASE II, recurso do III ou extensão MSX? 2. Em qual marco entra? 3. Impacto na RAM residente? 4. Exige segmento mapeado/overlay? 5. Altera DBF/NDX/DBT? 6. Como será testada? 7. Funciona sem UI gráfica? 8. Funciona em arquivo maior que a memória? 9. Mantém integridade após erro? 10. Precisa ser compatível com arquivos de outra implementação?

### 6.18 Relação entre os documentos
- `SPEC.md`: especificação completa e viva (comportamento, formatos, arquitetura, decisões). Prevalece para o que é implementável e testável.
- `OUTLINE.md`: documento vivo de progresso: fases, sub-fases, o que foi feito, o que falta, o que for sendo incluído.
- `resume.txt`: resumo curto do projeto, do que foi feito na sessão e dos próximos passos, para retomar em outro PC ou com outra IA.
- Em conflito, vale a decisão mais recente e explicitamente aprovada; corrigir os documentos para não divergirem.

---

## 7. Notas Técnicas Verificadas (lições do ambiente MSXgl / MSX-DOS 2)

Fatos confirmados na prática; respeitar ao escrever código novo.
- **A ROM principal não está mapeada sob MSX-DOS.** Ler variáveis/rotinas da ROM (ex.: `BIOS_GetMSXVersion()`, fonte da BIOS `g_CGTABL`) devolve lixo. Por isso: não checar versão do MSX por essa via, usar fonte embutida (`Print_SetTextFont(g_Font_MGL_Std0, 1)`) e `CheckVersion = false` em `project_config.js`.
- **Config do MSXgl** (`project/msxgl_config.h`): `VDP_USE_MODE_T2`, `PRINT_USE_TEXT`, `PRINT_WIDTH_1` e `PRINT_HEIGHT_1` (em modo texto cada caractere avança 1 posição; com o padrão 8 o texto sai espaçado).
- **Política de E/S (decisão 2026-10-26): vídeo e teclado nunca usam o console do MSX-DOS/CP/M.** Vídeo = acesso direto ao VDP (ou BIOS quando fizer sentido); teclado = BIOS (`CHSNS`/`CHGET` via `CALSLT`, slot em `EXPTBL`). O MSX-DOS só é usado para arquivos.
- **Teclado:** `DIRIN` do MSX-DOS fazia a BIOS desenhar um cursor fixo na tela (mesmo com `M_CSRSW` = 0). Solução: `CHSNS` sem espera e `CHGET` só com tecla no buffer, aguardando com `halt`. `M_CSRSW` continua sendo zerado e restaurado ao sair (`Console_Done`). Setas retornam 0x1C-0x1F.
- **Console:** Text2 80x24; tabela de nomes em `g_ScreenLayoutLow`; padrão 0 = célula em branco (a fonte é carregada com offset 1). Há uma cópia da tabela de nomes em RAM (1920 bytes): a rolagem é um `LDIR` na RAM seguido de uma única escrita ao VDP; textos são enviados em trechos por linha, sem `Print_*` por caractere.
- **Cores:** `Console_SetColor(texto, fundo)` grava o registrador 7 do VDP. Comando `SET COLOR TO W+/B` (estilo dBASE: N B G R W, combinações BG/RB/GR, `+` = brilhante) ou `SET COLOR TO 15,4` (paleta MSX 0-15, extensão MSX); sem argumento volta a W+/N. Alias `COLOR`.
- **Arquivos:** handles do MSX-DOS 2 (`DOS_OpenHandle`, `DOS_ReadHandle`, `DOS_SeekHandle`), `HANDLE_INVALID` = 0xFF; erro de leitura devolve 0xFFFF.
- **Mapper:** `DOSMapper_Init()` + `DOSMapper_GetVarTable()` (módulo `dos_mapper`); cada segmento = 16 KB.
- **Build:** `.\build.ps1` na raiz (usa `project/`). Módulos do projeto e bibliotecas ficam em `project/project_config.js`. Arquivos de dados vão em `DiskFiles`. **Fechar o openMSX antes de compilar**, senão a cópia do `.dsk` falha.
- **Executar:** `openmsx -machine Boosted_MSX2_EN -ext msxdos2 -diska .\project\emul\dsk\DOS2_msxdbase.dsk`

---

## 8. Decisões em Aberto (a fechar neste SPEC)
- Variante exata de DBF para compatibilidade dBASE II (versão 0x02) e política de migração entre II e III.
- Nível de compatibilidade NDX; formato Memo/DBT; codificação de caracteres.
- Número mínimo e máximo de áreas de trabalho.
- Requisitos mínimos definitivos (mínimo 512 KB e alvo 1 MB já decididos; falta definir MSX-DOS 1 e uso concreto da RAM extra).
- Sintaxe completa da linguagem e formato do bytecode.
- Política de extensões MSX; estratégia final de overlays.
- **Ordem entre UI e índices:** a seção 4 coloca a interface (Fase 2) antes dos índices (Fase 3); as diretrizes (6.x e marcos) colocam índices antes da interface. Está seguida a ordem da seção 4 até decisão em contrário.
- Nome definitivo do projeto (provisório: "MSX xBASE").

