# MSX dBASE III- / dBASE II+ Clone — OUTLINE VIVO

Data da base do documento: 2026-10-24
Última atualização: 2026-10-24
Status geral: shell inicial / sem núcleo de banco funcional

## 0. Documento vivo e contexto

Este arquivo deve funcionar como memória operacional do projeto para a continuidade em outro PC, em outra IA ou em uma nova sessão de trabalho.

Ele deve responder rapidamente a estas perguntas:
- onde o projeto está?
- o que foi validado?
- o que foi decidido?
- o que falta?
- qual é o próximo passo prático?

---

## 1. Visão geral

Este projeto é um clone clean-room de dBASE II+ para MSX, evoluindo para um ambiente compatível com recursos de dBASE III/III+ em arquitetura MSX 2+/turboR, com foco em:

- MSX 2+ / MSX turboR
- 512 KB de RAM mapeada (Memory Mapper)
- MSX-DOS 2
- armazenamento em SD/HD/IDE/Nextor
- compatibilidade com arquivos .dbf e scripts .prg
- execução nativa como .COM em MSX-DOS 2

O objetivo não é apenas recriar um editor de banco de dados superficial; é criar um ambiente de banco de dados funcional, compatível com o espírito do dBASE clássico, preservando e evoluindo recursos do dBASE II e III em hardware limitado do Z80.

---

## 2. Contexto técnico e de produto

### 2.1 Hardware alvo
- Plataforma: MSX 2+
- CPU: Z80A, com suporte a overclock de 5/7/10 MHz
- RAM: mínimo 256 KB, recomendado 512 KB mapeada
- VDP: V9958 em modo texto 80 colunas / TUI
- SO: MSX-DOS 2.x
- Armazenamento: SD, IDE, Nextor, MegaFlashROM SD, Sunrise IDE

### 2.2 Stack de desenvolvimento
- Linguagem: C (SDCC)
- Biblioteca principal: MSXgl
- Compilação: executável .COM para MSX-DOS 2
- Convenção: uso de tipos menores onde possível, buffers estáticos, otimização para memória mapeada e VDP

---

## 3. Fundação do spec

O arquivo [SPEC.md](SPEC.md) define as quatro fases de evolução previstas:

### Fase 1 — dBASE II+ Core & Compatibility (CLI)
- parser de comandos
- leitura/escrita de .dbf
- USE, DISPLAY, LIST, APPEND, REPLACE, DELETE, PACK, LOCATE, STORE, DO WHILE, IF...ENDIF

### Fase 2 — dBASE II++ Visual Revolution (TUI & Assistant)
- TUI em 80 colunas
- janelas, cores e menu estilo Assistant
- @ linha,coluna SAY/GET
- HELP contextual

### Fase 3 — dBASE III- (B-Tree Indexing .ndx)
- INDEX ON, SEEK, FIND, SET INDEX TO
- árvore B em memória mapper

### Fase 4 — dBASE III / III+ Full Features
- SELECT 1, 2...
- SET RELATION TO
- memo (.dbt), data/hora, texto avançado

---

## 4. Estado atual do projeto

### Situação real verificada
O projeto está em fase inicial, praticamente como um shell de execução com tela inicial.

Itens já existentes:
- estrutura base do repositório
- build do projeto com MSXgl e SDCC
- script de compilação via build.bat
- executável nativo para MSX-DOS 2
- tela de identificação com texto e validação de MSX 2+
- saída com ESC para sair

Itens ainda inexistentes:
- prompt do ponto (.)
- parser CLI
- leitura real de DBF
- estruturas de dados de tabela
- comandos SQL/dBASE command set
- gestão de arquivos .dbf/.ndx/.dbt
- TUI de janelas
- índice e cache em memória mapper
- suporte a SD / DOS 2 em nível de aplicação

### Status atual resumido
- Estado funcional: boot shell / launcher
- Estado do núcleo DB: 0%
- Estado da compatibilidade dBASE II: 0%
- Estado das interfaces: 0%
- Estado da fase de índice e queries: 0%

---

## 5. Regras para desenvolvimento e IA

As regras principais do projeto, conforme [SPEC.md](SPEC.md), são:

1. Priorizar eficiência em Z80 e uso de tipos menores sempre que útil.
2. Evitar alocação dinâmica pesada; preferir buffers estáticos e memória mapper.
3. Desenvolver em C compatível com SDCC.
4. Usar MSXgl para VRAM, paginação e hardware alvo.
5. Tratar manipulação direta de hardware via macros e blocos __asm comentados.

---

## 6. O que foi validado nesta sessão

### Arquivos-chave lidos
- [SPEC.md](SPEC.md)
- [README.md](README.md)
- [main.c](main.c)

### Observação importante
A partir do código atual, o programa apenas:
- detecta versão MSX
- se MSX1, mostra erro e exige ESC
- em MSX2+ entra em modo texto 80 colunas
- exibe um banner inicial
- aguarda ESC para sair

Portanto, a base de execução existe, mas o sistema de banco ainda não foi implementado.

### Decisões de implementação
- O próximo foco não é interface visual, e sim núcleo de dados.
- A arquitetura de memória e o uso do Memory Mapper devem ser tratados logo no início para evitar refatoração futura.
- A compatibilidade com dBASE II+ deve vir antes de qualquer recurso do dBASE III+.

### Bloqueios atuais
- sem prompt de comando
- sem parser
- sem tabelas .dbf em funcionamento
- sem suporte a índice
- sem validação em emulador real com dados de banco

---

## 7. Roadmap prático recomendado

### Próximo bloco: Fundação de dados
- definir estrutura de cabeçalho DBF clássico
- definir tipos de campo e limites
- implementar leitura/escrita de registros fixos
- validar compatibilidade com tabelas simples

### Bloco seguinte: parser e comandos
- prompt do ponto
- lexer/parser simples
- comandos USE, LIST, DISPLAY, APPEND, REPLACE
- tratamento de strings, nums e datas

### Bloco de interface
- modo de terminal 80 colunas
- janelas de comando e resultados
- help e menu assistente

### Bloco de índice e performance
- memória mapper
- cache de registros / leitura sequencial otimizada
- B-tree básico para .ndx

### Bloco de expansão
- relaçao entre tabelas
- memo (.dbt)
- multi-SELECT e área de trabalho

---

## 8. Checklist de projeto

### Feito
- [x] estrutura e organização do repo
- [x] leitura do spec e alinhamento arquitetural
- [x] build base do executável MSX-DOS 2
- [x] banner inicial com detecção de MSX 2+
- [x] saída controlada com ESC

### Em progresso
- [ ] análise profunda do formato .dbf
- [ ] design da memória e buffers de leitura
- [ ] parser de comandos dBASE II+
- [ ] engine de tabela

### Faltando
- [ ] tabela de campos e tipos
- [ ] import/export de .dbf
- [ ] indexação
- [ ] TUI
- [ ] comandos avançados do dBASE III+
- [ ] validação em emulador real

### Ordem imediata de execução
1. Definir estruturas de cabeçalho DBF e tipos de campo.
2. Implementar leitura e gravação em memória.
3. Criar prompt e parser simples.
4. Validar uso de USE/LIST/DISPLAY/APPEND.
5. Estender para DELETE, REPLACE e PACK.
6. Só então avançar para TUI e índices.

---

## 9. Notas para continuidade em outro PC ou outra IA

- O projeto parte de uma base limpa, com foco em compatibilidade e evolução.
- O arquivo [SPEC.md](SPEC.md) é a fonte principal de arquitetura e requisitos.
- a base atual é um skeleton operacional, não um sistema funcional de banco.
- o próximo passo lógico é implementar o núcleo de leitura de DBF e a CLI do dBASE II+ antes de tocar em TUI ou índice.
- recomenda-se manter todos os arquivos de documentação em português para contexto de desenvolvimento local e para futuras IA.

---

## 10. Resumo executivo

Este é um projeto de engenharia de software para MSX 2+ pensado como ambiente de banco de dados compatível com dBASE II+ e evolutivo para dBASE III/III+, utilizando C + SDCC + MSXgl. A especificação é clara, mas a implementação atual ainda corresponde ao shell inicial do programa. A prioridade imediata é construir o núcleo de armazenamento e comandos antes de avançar para a interface visual, índices e recursos do dBASE III+.
