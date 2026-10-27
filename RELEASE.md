# Releases

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