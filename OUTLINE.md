# MSX dBASE III- / dBASE II+ Clone — OUTLINE VIVO

Última atualização: 2026-10-27
Status geral: **Leitor DBF II MSX implementado e compilando; falta validar no openMSX; versão 1.3 Shell preparada**

Este arquivo é o **diário de progresso**: o que foi feito, o que falta e o que vamos incluindo. A especificação completa está em [SPEC.md](SPEC.md); o resumo curto para retomada está em [resume.txt](resume.txt). Os dois devem ser atualizados junto com este ao fim de cada sessão.

Legenda: `[x]` feito · `[~]` feito, falta validar · `[ ]` a fazer · `[?]` decisão pendente

---

## 1. Visão em uma linha
Ambiente xBase nativo para MSX2/2+/turbo R: núcleo compatível com dBASE II, recursos selecionados do dBASE III, em C (SDCC + MSXgl), sobre MSX-DOS 2 e Memory Mapper. Projeto de estudo: código modular e bem comentado. Detalhes em [SPEC.md](SPEC.md) (seção 6).

## 2. Mapa de fases e sub-fases

| Fase | Tema | Marcos das diretrizes | Estado |
|---|---|---|---|
| 0 | Fundação e infraestrutura | Marco 0 | concluída |
| 1 | Compatibilidade dBASE II Plus para MSX | DBF, comandos, NDX, modo programável | em andamento |
| 2 | Extensões dBASE III/III+ | Date, Memo, múltiplas áreas | não iniciada |
| 3 | Interface MSX avançada | TUI e assistência visual | não iniciada |

Matriz de conformidade e evidências: [DBASE_II_MSX_COMPATIBILITY.md](DBASE_II_MSX_COMPATIBILITY.md).

---

## 3. Fase 0 — Fundação (Marco 0) — CONCLUÍDA

- [x] 0.1 Repositório, build via `build.ps1`, MSXgl + SDCC, `.COM` para MSX-DOS 2, disco `.dsk` para o openMSX
- [x] 0.2 Console de texto 80x24 em Text2: rolagem, cursor, escrita, números, edição de linha (`console.c`)
- [x] 0.3 Prompt `.` e laço de comandos (`prompt.c`)
- [x] 0.4 Camada de plataforma (`platform.c`): versão do MSX-DOS, detecção de Memory Mapper e arquivos por handle
- [x] 0.5 Lições do ambiente registradas (SPEC seção 7): ROM inacessível sob DOS, `PRINT_WIDTH_1`, cursor da BIOS, DIRIN
- [x] 0.5b Vídeo direto no VDP com cópia da tela em RAM (rolagem rápida, escrita em trechos) e teclado pela BIOS (CHSNS/CHGET via CALSLT); nada de console do MSX-DOS. Correção do cursor fixo: `[~]` falta validar
- [x] 0.5c Extensão MSX `SET COLOR TO W+/B` e `SET COLOR TO 15,4` (alias `COLOR`)
- [~] Fonte ASCII embutida do MSXgl selecionada; substituiu `resource/vram.dat`, cuja pontuação não corresponde aos códigos ASCII; falta conferir texto e cursor no openMSX
- [ ] 0.6 Detecção de MSX1 x MSX2 sem ler a ROM (a checagem antiga de versão foi removida porque lia a ROM)
- [ ] 0.7 Teste em hardware real e em outras configurações de memória (512 KB e 1 MB, turbo R)
- [~] 0.8 Gravação de arquivos na camada de plataforma (criar, escrever, renomear, apagar); wrappers compilam, falta teste em runtime

## 4. Fase 1 — Núcleo dBASE II+ (CLI)

### 1.1 DBF somente leitura (Marco 1)
- [~] Engine `dbf.c`: leitor DBF II MSX (`0x02`) e DBF III (`0x03/0x83`), navegação, marca de exclusão e validações (`DBF_ERR_*`); falta validar DBF II no emulador
- [~] Comandos: `USE`, `LIST [STRUCTURE]`, `DISPLAY [STRUCTURE|ALL]`, `GO TOP|BOTTOM|n`, `SKIP [n]`, `CLEAR`, `VER`, `HELP`, `QUIT`
- [~] Exibição de tipos C, N, L, D e M (memo apenas como texto "memo"); `LIST` com pausa por tela
- [~] Banco de teste `project/data/CLIENTES.DBF` (5 registros, 1 excluído) gerado por `make_sample_dbf.ps1`
- [x] **Validar o leitor atual no openMSX** (validação do usuário; cobre a amostra dBASE III, não conformidade dBASE II MSX)
- [~] `resource/PESSOAL.DBF` confirma o layout DBF II e está incluído na imagem de teste; falta `USE PESSOAL` no openMSX
- [ ] `DISPLAY` com `FOR`, `RECORD n`, listas de campos (depende de 1.3)
- [ ] `COUNT`, `SET DELETED ON/OFF`

### 1.2 Escrita segura (Marco 2)
- [~] Operações de arquivo na camada platform (abrir, criar, ler, escrever, renomear, apagar); compila, falta teste em execução
- [ ] `APPEND BLANK`, `REPLACE`, `DELETE`, `RECALL`
- [ ] Atualização segura do cabeçalho (contagem de registros, data) e marca 0x1A
- [ ] `PACK` com arquivo temporário e recuperação após falha
- [ ] `CREATE` (definição de estrutura) e `COPY STRUCTURE`
- [ ] Testes de integridade (interrupção, disco cheio, somente leitura)

### 1.3 Expressões e filtros (Marco 3)
- [ ] Lexer e tipos de valor (C, N, L, D); numérico sem ponto flutuante (decimal fixo/BCD) — decidir
- [ ] Operadores, funções básicas, `STORE` e variáveis de memória
- [ ] Cláusulas `FOR`/`WHILE`, `SET FILTER TO`
- [ ] `LOCATE` / `CONTINUE`, `COUNT`, `SUM`, `AVERAGE`
- [ ] Substituir a tabela de comandos por tokens/lexer (ver SPEC 6.6)

### 1.4 Programas `.PRG` e bytecode (Marco 4)
- [ ] Parser e compilador para bytecode; máquina virtual
- [ ] `DO`, `DO WHILE/ENDDO`, `IF/ELSE/ENDIF`, `RETURN`, `PROCEDURE`, `PARAMETERS`
- [ ] Execução de arquivos `.PRG`; modo silencioso

### 1.5 Conformidade dBASE II MSX
- [x] Confirmar cabeçalho de 521 bytes e descritores de 16 bytes com `resource/PESSOAL.DBF`
- [x] Implementar leitura DBF II (`0x02`) mantendo DBF III apenas para fixture/interoperabilidade
- [ ] Confirmar no emulador e criar testes byte a byte para gravação
- [ ] Entender padding/EOF antes de implementar escrita e `PACK`

## 5. Índices NDX (núcleo dBASE II MSX)
- [ ] Cobrir `INDEX`, `FIND`, `REINDEX`, `SET INDEX` conforme o manual MSX
- [ ] Leitura, criação e atualização após alterações
- [ ] Cache de páginas no Memory Mapper (nunca o índice inteiro)
- [ ] Validação e reconstrução

## 6. Fase 2 — Extensões dBASE III/III+
- [ ] Mais áreas de trabalho (`SELECT`), aliases, `SET RELATION TO`
- [ ] Campo Date completo, Memo/DBT
- [ ] Relatórios (`REPORT`, `LABEL`), `SORT`, `TOTAL`, `JOIN`
- [ ] Overlays e uso do Memory Mapper para código/dados

## 7. Fase 3 — Interface MSX avançada
- [ ] Editor de linha melhor: setas, Home/End, histórico
- [ ] Janelas, menus, caixas, barra de estado, cores configuráveis
- [ ] `@ lin,col SAY/GET`, `BROWSE`, `EDIT`, `APPEND` em formulário
- [ ] Menu estilo Assistant; `HELP <comando>` e F1
- [ ] Mouse opcional; 40/80 colunas
- [ ] Manter sempre o modo console

---

## 8. Estado do código (referência rápida)

| Arquivo | Papel |
|---|---|
| `project/main.c` | entrada: detecta plataforma, inicia console, roda o prompt |
| `project/console.c/.h` | tela 80x24, rolagem, teclado, edição de linha |
| `project/prompt.c/.h` | laço do prompt `.` |
| `project/commands.c/.h` | tabela e execução de comandos (transitória até o lexer) |
| `project/dbf.c/.h` | engine DBF somente leitura (sem UI) |
| `project/platform.c/.h` | MSX-DOS, Memory Mapper, arquivos |
| `project/project_config.js` | módulos do projeto, bibliotecas, `DiskFiles` |
| `project/msxgl_config.h` | opções do MSXgl |
| `project/data/` | banco de teste e script gerador |

Regra: nova funcionalidade de dados entra em módulo próprio sem depender de UI; só `platform.c` fala com MSX-DOS.

## 9. Decisões tomadas (log)
- 2026-10-24: projeto clean-room, C + SDCC + MSXgl, alvo MSX-DOS 2.
- 2026-10-25/26: DBASE_MSX_DIRETRIZES incorporado ao SPEC; documentos vivos definidos (SPEC / OUTLINE / resume.txt).
- 2026-10-26: prioridade definida como dBASE II Plus para MSX; dBASE III passa a ser extensão posterior.
- 2026-10-27: layout DBF II confirmado com `resource/PESSOAL.DBF`; parser `0x02` implementado. Fonte ASCII do MSXgl substituiu `VRAM.DAT` por incompatibilidade de pontuação.
- 2026-10-26: leitura de teclas via MSX-DOS DIRIN; cursor da BIOS desligado; fonte embutida.
- 2026-10-26: Marco 0 declarado concluído; Marco 1 implementado, falta validação.
- 2026-10-26: **mínimo absoluto 512 KB de mapper; alvo 1024 KB** (128/256 KB não são mais objetivo).
- 2026-10-26: **vídeo e teclado sem console do MSX-DOS/CP/M**: VDP direto e BIOS; DOS só para arquivos.
- 2026-10-26: primeira extensão MSX implementada (`SET COLOR TO`) antes da próxima fase.
- 2026-10-26: versionamento `X.Y`; `X` marca marcos com codinomes Unix/C e `Y` avança em builds bem-sucedidos (pausável com `-KeepVersion`). Primeiro release: `1.2 Shell`.
- 2026-10-27: release `1.3 Shell`, leitor inicial DBF II MSX e correção de fonte/cursor.

## 10. Backlog e ideias (adicionar aqui conforme surgirem)
- Ferramenta no PC (host) para ler/validar DBF e comparar golden files.
- Testes da engine DBF fora do MSX (camada `platform` permite).
- Medir RAM residente e tempo de `LIST` em emulador e hardware real.
- Decidir representação de Numeric (BCD x decimal fixo).
- Nome definitivo do projeto.

## 11. Rotina de fim de sessão
1. Compilar e, se possível, testar no openMSX.
2. Atualizar este OUTLINE (marcar itens, registrar decisões).
3. Atualizar o SPEC se houve decisão ou fato técnico novo.
4. Atualizar o `resume.txt` (feito hoje + próximos passos).
