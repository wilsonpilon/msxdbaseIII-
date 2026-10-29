# Changelog

## 3.1 Grep

- `INSERT [BEFORE] [BLANK]` e `SET CONFIRM ON/OFF` para os formularios DBF II.
- `HELP` sem parametros usa um resumo local, evitando abrir/indexar um arquivo externo no caminho mais comum.
- Inicializacao explicita do estado do indice HELP; limite reduzido a 160 topicos para liberar RAM residente.
- Moldura SCREEN corrigida e redesenho incremental da area de texto; a verificacao visual final do viewer permanece pendente.
- `DB8SEMSG.TXT` passou a ser opcional no build e nao e distribuido no release.
- Build de MSX-DOS 2 validado; DSK local atualizado. GitHub Release inclui apenas o `.COM`, sem o DSK com recursos originais.

## 3.0 Grep

- Formulario simultaneo `APPEND` para DBF II com navegacao, edicao, cancelamento e campos C/N/L; `SET COLON OFF` oculta ambos os delimitadores.
- `SET DELETED ON/OFF` validado em banco com registro excluido; o filtro atua em `LIST`, `DISPLAY` e `REPLACE ALL`.
- `DISPLAY FILES` e padroes literais com `LIKE` validados com e sem correspondencias.
- `REPLACE ALL ... FOR` validado com comparacao numerica seletiva.
- Manual do usuario, README com capturas do openMSX e pacote local em `release/3.0/`.

## 2.0 Fork

- Implementado `APPEND BLANK` em DBF II MSX, com atualização do cabeçalho, marcador EOF e padding físico conforme o dBASE II original.
- Validada a escrita byte a byte em cópias de `PESSOAL.DBF` e `ALUNOS.DBF` (`RecLen=36` e `RecLen=41`).
- Adicionado `ALUNOS.DBF` à imagem de teste e suporte à leitura correta de EOF nos wrappers da plataforma.
- Limitações e testes pendentes detalhados em `RELEASE.md`.

## 1.3 Shell

- Leitor inicial de DBF II MSX (`0x02`) com cabeçalho de 521 bytes e descritores de 16 bytes.
- Amostra original `PESSOAL.DBF` incluída no disco de teste; leitura DBF III mantida para interoperabilidade.
- Primitivos de criar, abrir, escrever, renomear e apagar arquivos na camada platform.
- Fonte ASCII do MSXgl no console para pontuação correta e cursor alinhado à base.
- Estado do build e limitações conhecidos detalhados em `RELEASE.md`.

## 1.2 Shell

- Console MSX de 80 colunas, prompt e cores configuraveis.
- Comandos basicos de navegacao e leitura de DBF dBASE III.
- Banco de demonstracao e build para MSX-DOS 2.