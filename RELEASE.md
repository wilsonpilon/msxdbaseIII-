# Releases

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