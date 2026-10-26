# MSX dBASE

Clone *clean room* do dBASE II para MSX, com melhorias de interface e evolucao
gradual de recursos do dBASE III. O escopo e o roteiro estao em
[SPEC.md](./SPEC.md).

## Estado atual

O projeto comeca pela base executavel para MSX-DOS 2, usando C/SDCC e a
biblioteca MSXgl incluida neste repositorio. O programa inicial apenas mostra
uma tela de identificacao em modo texto de 80 colunas; parser, comandos e
arquivos DBF serao implementados em marcos seguintes.

## Compilar no Windows

Execute `build.bat` na raiz do repositorio. O script usa o Node.js incluido em
`MSXgl` quando disponivel e gera os arquivos de build em `out/`, incluindo
`msxdbase.com`.

Para compilar manualmente com Node.js instalado:

```text
node MSXgl/engine/script/js/build.js
```

Execute o `.COM` em um sistema ou emulador MSX com MSX-DOS 2. Pressione ESC
para sair da tela inicial.

Se executado em MSX1, o programa agora exibe uma mensagem informando que MSX2+
e necessario e permite sair com ESC, em vez de aparentar travamento.
