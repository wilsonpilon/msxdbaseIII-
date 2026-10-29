# Releases

## 3.1 Grep: inserção, formulários e ajuda sem bloqueio

Release: `3.1`, codinome `Grep`, 2026-10-29.

- `INSERT [BEFORE] [BLANK]` posiciona registros relativamente ao ponteiro e reutiliza o formulário C/N/L.
- `SET CONFIRM ON/OFF` controla avanço automático dos formulários.
- `HELP` sem argumentos exibe o resumo local sem abrir um arquivo externo; o índice HELP é inicializado explicitamente e limita a tabela a 160 tópicos.
- A moldura SCREEN usa cantos corrigidos, cor estável e redesenho incremental. A validação visual final dessa janela ainda está pendente.
- `DB8SEMSG.TXT` é opcional no build e não é distribuído. Sem ele, o resumo local funciona; os verbetes originais não.

Validação: build SDCC/MSX-DOS 2 passou. `HELP` TXT foi executado em cópia temporária do DSK e exibiu o resumo local. Operações destrutivas foram testadas em cópias. O DSK padrão contém recursos locais que não fazem parte do release público.

Artefatos publicados: executável `.COM`. O DSK não é anexado porque contém dados originais de terceiros. O usuário pode gerar sua própria imagem com `build.ps1` e adicionar uma cópia licenciada de `DB8SEMSG.TXT` localmente.

### Snapshot local 3.0

Veja `release/3.0/`. Esse pacote é histórico e não foi publicado como GitHub Release.

## 3.0 Grep: formulario, filtros e consulta de arquivos

Titulo do release: `Grep: formulario, filtros e consulta de arquivos`.

- `APPEND` apresenta todos os campos C/N/L na mesma tela, com navegacao, edicao no lugar, validacao, cancelamento e gravacao somente depois da confirmacao.
- `SET COLON ON/OFF` controla os delimitadores de abertura e fechamento das faixas do formulario.
- `SET DELETED ON/OFF` testado em DBF III com um registro excluido; `LIST` omite somente esse registro quando ON.
- `DISPLAY FILES`, `DISPLAY FILES LIKE *.*` e `DISPLAY FILES LIKE *.FRM` testados com correspondencias e sem resultados.
- `REPLACE ALL IDADE WITH IDADE + 5 FOR IDADE < 12` alterou 3 de 6 registros em DBF II.
- `APPEND L` testado numa copia temporaria de DBF II: entrada invalida `X` foi rejeitada e `Y` gravada como `T`.
- Build completo MSX-DOS 2 e imagens/capturas do openMSX incluidos no pacote local.

Limitacoes conhecidas: mutacoes DBF III nao sao suportadas; `FOR` aceita uma comparacao numerica, sem `AND`/`OR`; `WHILE`, `SET FILTER`, NDX e recuperacao de falhas de escrita continuam pendentes. A fixture Logical usada no teste foi temporaria e nao foi adicionada ao disco padrao.

Downloads locais:
- [Executavel MSX-DOS 2](release/3.0/msxdbase-3.0.com)
- [Imagem de disco openMSX](release/3.0/DOS2_msxdbase-3.0.dsk)
- [Pacote ZIP](release/msxdbase-3.0-downloads.zip)

Tag sugerida: `3.0` (nao criada nem publicada).

## 2.0 Fork: APPEND BLANK para DBF II MSX

Titulo do release GitHub: `Fork: APPEND BLANK para DBF II MSX`.

- Implementado `APPEND BLANK` para DBF II MSX (`0x02`); DBF III é recusado para mutações.
- O comando grava um registro preenchido com espaços, atualiza contador e data, move o marcador `0x1A` e preenche o restante do tamanho físico com `0xFF`.
- A data vem de MSX-DOS 2; corrigido o uso do dia do mês (`DOS_Time.Date`) em vez do dia da semana.
- O EOF de leitura do MSX-DOS (`DOS_ERR_EOF`) é normalizado para zero em `PFile_Read`, sem mascarar outros erros.
- Teste de referência executado com dBASE II original em DSK scratch; `APPEND BLANK` alterou a contagem 6→7 e preservou o tamanho físico de 1024 bytes.
- Comparação byte a byte do clone passou em cópias de `PESSOAL.DBF` (`RecLen=36`) e `ALUNOS.DBF` (`RecLen=41`); registros anteriores permaneceram idênticos.
- A imagem inclui `CLIENTES.DBF`, `PESSOAL.DBF` e `ALUNOS.DBF`.

Limitações conhecidas: `REPLACE`, `DELETE`, `RECALL` e `PACK` não estão implementados. Ainda faltam testes de rollback sob falhas de seek/escrita, arquivo com EOF inválido, limite de registros e confirmação byte a byte de que DBF III recusado permaneça intacto. A página de GitHub Release depende de autenticação GitHub; os artefatos locais estão em `release/2.0/`.

Downloads locais:
- [Executável MSX-DOS 2](release/2.0/msxdbase-2.0.com)
- [Imagem de disco openMSX](release/2.0/DOS2_msxdbase-2.0.dsk)
- [Pacote ZIP](release/msxdbase-2.0-downloads.zip)

Tag: `2.0`
Commit enviado para `origin/main`: `f499fc1`.
Tag `2.0` enviada para `origin`. A página GitHub Release ainda não foi criada porque o `gh` não está autenticado; após login, publicar com os três artefatos locais acima.

## 1.3 Shell: leitura inicial de DBF II MSX

Titulo do release GitHub: `Shell: leitura inicial de DBF II MSX`.

- Leitura da variante DBF II MSX (`0x02`): cabeçalho fixo de 521 bytes e descritores de 16 bytes.
- Inclusão de `PESSOAL.DBF`, criado no dBASE II para MSX, na imagem de teste.
- Mantida a leitura DBF III (`0x03/0x83`) para fixture e interoperabilidade.
- Adicionadas operações de arquivo na camada platform: abrir para escrita, criar, escrever, renomear e apagar.
- Trocada a tabela de caracteres externa por fonte ASCII embutida do MSXgl, corrigindo pontuação incompatível e o desenho do cursor.
- Build SDCC/MSX-DOS 2 e geração de DSK concluídos.

Estado conhecido: a leitura DBF II, a fonte e o cursor ainda precisam de validação visual/funcional no openMSX. As operações de escrita DBF e `PACK` não estão implementadas; padding/EOF da amostra original ainda precisam ser entendidos.

Tag: `1.3`

## 1.2 Shell: console, comandos e leitura de DBF

Titulo do release GitHub: `Shell: console, comandos e leitura de DBF`.

Primeiro release do projeto como ambiente xBase executavel para MSX-DOS 2.

- Console de texto 80x24 com rolagem, entrada de linha e cores MSX.
- Comandos `HELP`, `CLEAR`, `QUIT`, `VER` e `SET COLOR TO`.
- Leitura de DBF dBASE III: `USE`, `LIST`, `DISPLAY`, `GO`, `SKIP` e estrutura.
- Banco de demonstracao `CLIENTES.DBF` e script para gera-lo.
- Build para MSX-DOS 2 com MSXgl e SDCC.

Tag: `1.2`

Estado conhecido: o build compila; a validacao funcional no openMSX ainda esta
pendente, incluindo cursor, cores e navegacao/exibicao dos registros.

## Convencao de versao

- `X` identifica um marco funcional e recebe um codinome inspirado em conceitos
  de Unix/C classicos. Avanca ao iniciar uma nova feature ou concluir um marco.
- `Y` identifica uma compilacao bem-sucedida e avanca automaticamente no build.
  Use `build.ps1 -KeepVersion` durante compilacoes repetidas de teste.
- A tag Git usa exatamente `X.Y`, sem prefixo. O titulo do release usa
  `<codinome>: <descricao curta>`.
- Para mudar `X`, atualize o major e o codinome em `project/version.h` e registre
  o novo marco em `RELEASE.md` e `Changelog.MD`.

Codinomes usados: `1 = Shell`.