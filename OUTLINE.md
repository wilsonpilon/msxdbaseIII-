# MSX dBASE III- / dBASE II+ Clone — OUTLINE VIVO

Última atualização: 2026-10-29
Status geral: **Versão 3.1 Grep publicada; build validado, HELP TXT sem bloqueio; SCREEN ainda precisa de validação visual**

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
- [x] 0.8 Gravação de arquivos na camada de plataforma (criar, escrever, renomear, apagar); wrappers testados em runtime no openMSX

## 4. Fase 1 — Núcleo dBASE II+ (CLI)

### 1.1 DBF somente leitura (Marco 1)
- [x] Engine `dbf.c`: leitor DBF II MSX (`0x02`) e DBF III (`0x03/0x83`), navegação, marca de exclusão e validações (`DBF_ERR_*`); DBF II validado pelo usuário no openMSX
- [~] Comandos: `USE`, `LIST [STRUCTURE]`, `DISPLAY [FILES|STRUCTURE|ALL]`, `GO TOP|BOTTOM|n`, `SKIP [n]`, `APPEND [BLANK]`, `INSERT [BEFORE] [BLANK]`, `CLEAR`, `SET COLOR/DELETED/COLON/CONFIRM/HELP/MSX`, `VER`, `HELP`, `QUIT`
- [x] `HELP` sem argumentos mostra resumo local sem acessar disco; estado do índice inicializado em `Help_Init`; limite do índice 160 tópicos para reduzir RAM
- [~] `HELP DBASE` e `HELP <COMANDO>` dependem de `DB8SEMSG.TXT` local; recurso opcional, não redistribuído; testar novamente abertura e verbetes completos
- [~] Viewer SCREEN tem quadro corrigido, cores estáveis e redesenho incremental; quadro e resumo local confirmados visualmente; falta validar busca, rolagem, cores e saída, além dos tópicos externos
- [~] `SET MSX OFF/ON` controla cores e HELP SCREEN; aplicar o gate a futuras melhorias visuais/de entrada conforme forem implementadas
- [x] `DISPLAY FILES`: sem padrão lista DBFs com contagem/data do cabeçalho; `LIKE` usa padrões literais, sem exigir `USE`; runtime com e sem correspondências validado no openMSX
- [~] Exibição de tipos C, N, L, D e M (memo apenas como texto "memo"); `LIST` com pausa por tela
- [~] Banco de teste `project/data/CLIENTES.DBF` (5 registros, 1 excluído) gerado por `make_sample_dbf.ps1`
- [x] **Validar o leitor no openMSX** com `resource/PESSOAL.DBF` (DBF II): `USE PESSOAL`, `LIST`, `DISPLAY STRUCTURE`, `GO 6` e `DISPLAY` funcionaram
- [x] `resource/PESSOAL.DBF`: layout DBF II e leitura funcional confirmados no emulador
- [x] `resource/ALUNOS.DBF`: DBF II com `RecLen=41`; usuário validou `USE`, `LIST`, `DISPLAY STRUCTURE`, `GO 6` e `DISPLAY` no openMSX
- [ ] `DISPLAY` com `FOR`, `RECORD n`, listas de campos (depende de 1.3)
- [ ] `COUNT`
- [x] `SET DELETED ON/OFF`: default OFF; `LIST` comprovado ocultando somente o registro marcado em `CLIENTES.DBF`; `DISPLAY` e `REPLACE ALL` continuam cobertos pelo mesmo filtro no código

### 1.2 Escrita segura (Marco 2)
- [x] Operações de arquivo na camada platform (abrir, criar, ler, escrever, seek, tamanho, renomear e apagar); teste em runtime passou no openMSX
- [x] `APPEND` interativo DBF II: formulário simultâneo com faixas `:`; Tab/Down/Enter avançam, Up volta, Left/Right e Backspace/Delete editam, ESC cancela; C/N/L, correção após erro e vazios validados no openMSX
- [x] `APPEND BLANK` para DBF II: grava registro de espaços, atualiza contagem/data, move `0x1A` e preenche o tail físico com `0xFF`; comparado byte a byte no scratch para `PESSOAL` (`RecLen=36`) e `ALUNOS` (`RecLen=41`)
- [x] `INSERT [BEFORE] [BLANK]` para DBF II: insere antes/depois do ponteiro, compartilha o formulário C/N/L e mantém o ponteiro no novo registro; inclusão interativa e BLANK validados por runtime/bytes. Limites BOF/EOF e rollback ainda pendentes
- [x] `SET CONFIRM ON/OFF`: ON (padrão) mantém o campo cheio até confirmação; OFF avança ao preencher e abre outro registro após o último campo; validado em `APPEND` e `INSERT`
- [x] DBF III (`CLIENTES.DBF`, versão `0x03`) é recusado por `APPEND BLANK` com a mensagem de formato sem suporte; falta comparar o arquivo byte a byte para confirmar que não mudou
- [x] `REPLACE [ALL] campo WITH expressão` (aritmética numérica básica), `DELETE` e `RECALL` para DBF II; testes manuais reportados pelo usuário
- [x] Atualização do cabeçalho (contagem de registros e data) e marca `0x1A` validada em cópias
- [ ] Testar rollback em erro de seek/escrita, EOF inválido, arquivo DBF III recusado sem alteração byte a byte e limite de 65.535 registros
- [x] `PACK` gera `.TMP`, preserva o original como `.BAK` durante a troca; testes manuais reportados pelo usuário. Recuperação após falha ainda não exercitada
- [ ] `CREATE` (definição de estrutura) e `COPY STRUCTURE`
- [ ] Testes de integridade (interrupção, disco cheio, somente leitura)
- [x] `SET COLON ON/OFF`: mostra/oculta os delimitadores de abertura e fechamento da faixa; padrão ON; ON/OFF validados no openMSX

### 1.3 Expressões e filtros (Marco 3)
- [~] Subconjunto decimal em `REPLACE`: `+`, `-`, `*`, `/`, parênteses e campos N; escala intermediária até 6 casas, sem ponto flutuante
- [ ] Lexer completo e tipos de valor (C, N, L, D); definir representação geral de Numeric
- [ ] Operadores, funções básicas, `STORE` e variáveis de memória
- [x] `FOR` em `REPLACE ALL`: uma comparação numérica (`=`, `<>`, `!=`, `<`, `<=`, `>`, `>=`); sem `AND`/`OR`; `IDADE < 12` alterou 3/6 registros no openMSX
- [ ] `WHILE`, `SET FILTER TO`
- [ ] `LOCATE` / `CONTINUE`, `COUNT`, `SUM`, `AVERAGE`
- [ ] Substituir a tabela de comandos por tokens/lexer (ver SPEC 6.6)

### 1.4 Programas `.PRG` e bytecode (Marco 4)
- [ ] Parser e compilador para bytecode; máquina virtual
- [ ] `DO`, `DO WHILE/ENDDO`, `IF/ELSE/ENDIF`, `RETURN`, `PROCEDURE`, `PARAMETERS`
- [ ] Execução de arquivos `.PRG`; modo silencioso

### 1.5 Conformidade dBASE II MSX
- [x] Confirmar cabeçalho de 521 bytes e descritores de 16 bytes com `resource/PESSOAL.DBF`
- [x] Implementar leitura DBF II (`0x02`) mantendo DBF III apenas para fixture/interoperabilidade
- [x] Confirmar leitura DBF II no emulador
- [x] Testar wrappers de arquivo da plataforma em runtime no openMSX
- [x] Testes byte a byte de `APPEND BLANK` em cópias DBF II `PESSOAL` e `ALUNOS`
- [x] Determinar a semântica de `APPEND BLANK` após EOF no dBASE II original; padding anterior varia, mas o original preenche com `0xFF` após mover o EOF

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
- [ ] Editor de tela cheia integrado para arquivos `.PRG`, usando o TED de `msx2026/` como referência e fonte autorizada para adaptação; separar buffer/edição, UI e acesso a arquivos pela plataforma, sem iniciar `TED.COM` como processo externo
- [ ] Evoluir o editor interno com busca, blocos e teclas de função inspirados no TED
- [ ] Janelas, menus, caixas, barra de estado, cores configuráveis
- [ ] `@ lin,col SAY/GET`, `BROWSE`, `EDIT`, `APPEND` em formulário
- [ ] Menu estilo Assistant; `HELP <comando>` e F1
- [ ] Mouse opcional; 40/80 colunas
- [ ] Manter sempre o modo console
- [ ] Menu da suíte para dBASE, WordStar 3.3/4.0 e SuperCalc 2/3 como aplicativos independentes; testar compatibilidade no MSX-DOS 2 e definir formatos de troca de arquivos

---

## 8. Estado do código (referência rápida)

| Arquivo | Papel |
|---|---|
| `project/main.c` | entrada: detecta plataforma, inicia console, roda o prompt |
| `project/console.c/.h` | tela 80x24, rolagem, teclado, edição de linha |
| `project/prompt.c/.h` | laço do prompt `.` |
| `project/commands.c/.h` | tabela e execução de comandos (transitória até o lexer) |
| `project/dbf.c/.h` | engine DBF II/III de leitura e mutações DBF II (`APPEND BLANK`, `REPLACE`, `DELETE`, `RECALL`, `PACK`; sem UI) |
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
- 2026-10-27: release `2.0 Fork`, primeira mutação de DBF II (`APPEND BLANK`) validada byte a byte em cópias de PESSOAL e ALUNOS.
- 2026-10-27: marco `2.0 Fork`; `APPEND BLANK` DBF II observado no dBASE II original e implementado. Testado byte a byte em cópias de `PESSOAL` e `ALUNOS`; `CLIENTES.DBF` (`0x03`) é recusado para mutação.
- 2026-10-27: decidido planejar um editor interno de programas inspirado no TED de `msx2026/`; aproveitar sua arquitetura e, dentro da autorização do autor, adaptar código em módulos próprios do dBASE. O TED não será aberto como processo externo; menus e editor avançado ficam para a fase de interface, após estabilizar o núcleo DBF.
- 2026-10-27: usuário validou `USE PESSOAL`, `LIST`, `DISPLAY STRUCTURE`, `GO 6` e `DISPLAY` com DBF II no openMSX. Probe runtime dos wrappers passou; `PFile_Read` normaliza `DOS_ERR_EOF` para zero.
- 2026-10-27: o dBASE II original foi executado numa cópia scratch. `APPEND BLANK` mudou contagem 6→7, data para `10/27/26`, substituiu o EOF em `0x2E1` por 36 espaços, gravou `0x1A` em `0x305` e deixou 250 bytes `0xFF` até o tamanho físico de 1024.
- 2026-10-27: `APPEND BLANK` do projeto validado byte a byte em cópias de `PESSOAL` (`RecLen=36`) e `ALUNOS` (`RecLen=41`); registros anteriores permaneceram idênticos, novo EOF e padding conferidos.
- 2026-10-28: implementados `REPLACE campo WITH valor`, `DELETE`, `RECALL` e `PACK` DBF II. O build passou; comparação byte a byte e testes de rollback/recuperação dessas operações ainda pendentes.
- 2026-10-28: corrigido o README: detecção de MSX1 não está implementada; o alvo de build MSX2 não equivale a uma detecção em runtime.
- 2026-10-28: prompt tem histórico de 8 comandos, TAB cíclico por prefixo e edição com setas esquerda/direita; descer além do comando mais recente limpa a linha. Usuário confirmou o teste desses comportamentos.
- 2026-10-28: `REPLACE [ALL] campo WITH expressão` aceita aritmética decimal simples e referências a campos numéricos; usuário confirmou o teste. `FOR` numérico foi adicionado depois; `WHILE`, funções e gramática xBase completa continuam pendentes.
- 2026-10-28: adicionado `FOR` numérico em `REPLACE ALL`; build passou. A condição suporta uma comparação de expressões Numeric por comando, sem `AND`/`OR`; falta validar no openMSX.
- 2026-10-28: implementado `SET DELETED ON/OFF` (default OFF); filtra `LIST`, `DISPLAY` atual e as duas passagens de `REPLACE ALL`. `GO`/`SKIP` seguem físicos; build passou.
- 2026-10-28: implementado `DISPLAY FILES` pela camada platform; sem padrão lista contagem/data DBF; `LIKE padrao` lista literalmente nome/tamanho/data DOS. Não exige `USE`; build passou, teste runtime pendente.
- 2026-10-28: `APPEND` sem argumento abre formulário DBF II C/N/L, valida todas as entradas e então grava; `APPEND BLANK` permanece. `SET COLON ON/OFF` controla o delimitador, padrão ON. Build passou, teste runtime pendente.
- 2026-10-28: usuário informou que os testes anteriores de mutação e `PACK` funcionaram; sem resultados detalhados registrados por fixture/comando.
- 2026-10-29: `APPEND` validado no openMSX em cópia nova de `ALUNOS.DBF` (6 registros iniciais): valor N/C válido, entrada Numeric inválida com reentrada, formulário vazio e `APPEND BLANK`; `LIST` terminou com 9 registros. Campo L ainda não testado.
- 2026-10-29: `SET COLON ON/OFF` e os comandos `SET DELETED ON/OFF` validados no openMSX. `SET COLON OFF` oculta os dois delimitadores. Corrigido o parser: `OFF` tem três caracteres, mas os dois comandos aceitavam apenas modos de dois caracteres. O efeito de `SET DELETED` em registros excluídos ainda precisa de teste.
- 2026-10-29: `APPEND` passou a mostrar os campos juntos, com faixas delimitadas, navegação e edição no lugar. No openMSX, Up permitiu alterar `SEXO` sem perder `SERIE`; cancelar não gravou e o formulário vazio criou um registro em branco. Corrigida também a sobreposição dos terminadores entre buffers de campos.
- 2026-10-29: fechados os testes de `APPEND L`, `SET DELETED`, `DISPLAY FILES [LIKE]` e `REPLACE ALL ... FOR`; capturas guardadas em `docs/screenshots/`.
- 2026-10-29: marco `3.0 Grep`; release local empacotado com `.COM`, `.DSK` e notas. Publicacao externa/tag Git nao realizada.
- 2026-10-29: preparado `3.1 Grep`; `HELP` sem argumentos usa resumo local, `Help_Init` inicializa o índice e `DB8SEMSG.TXT` é opcional no build. O `.COM` e o DSK local compilam; o release público não inclui o DSK nem o arquivo original.
- 2026-10-29: `HELP` TXT com resumo local confirmado em cópia temporária do DSK. Abertura de tópicos externos e validação visual final do viewer SCREEN continuam pendentes.
- 2026-10-29: commit `597dce8`, tag `3.1` e GitHub Release publicados em `origin`; assets limitados ao `.COM` e notas ZIP.
- 2026-10-29: `INSERT [BEFORE] [BLANK]` implementado e validado em cópias DBF II; duas inclusões vazias foram ordenadas antes/depois do ponteiro, e `INSERT BEFORE` interativo gravou os dados no local correto sem deixar `.BAK`.
- Próximo passo: validar limites BOF/EOF, limite de 65.535 registros e rollback do `INSERT`; depois iniciar `WHILE`/`SET FILTER TO` e os índices NDX.
- 2026-10-27: registrada a intenção de uma suíte integrada com dBASE, WordStar 3.3 ou 4.0 e SuperCalc 2 ou 3, mantendo os aplicativos independentes e integrando-os por menu e troca de arquivos; versões e formatos ainda precisam ser avaliados.

## 10. Backlog e ideias (adicionar aqui conforme surgirem)
- Ferramenta no PC (host) para ler/validar DBF e comparar golden files.
- Testes da engine DBF fora do MSX (camada `platform` permite).
- Medir RAM residente e tempo de `LIST` em emulador e hardware real.
- Decidir representação de Numeric (BCD x decimal fixo).
- Nome definitivo do projeto.
- Avaliar a execução no MSX-DOS 2 das versões WordStar 3.3/4.0 e SuperCalc 2/3 e definir a integração por menu e formatos de intercâmbio.

## 11. Rotina de fim de sessão
1. Compilar e, se possível, testar no openMSX.
2. Atualizar este OUTLINE (marcar itens, registrar decisões).
3. Atualizar o SPEC se houve decisão ou fato técnico novo.
4. Atualizar o `resume.txt` (feito hoje + próximos passos).
