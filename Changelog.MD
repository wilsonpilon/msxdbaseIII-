# Changelog

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