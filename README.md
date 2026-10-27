# MSX dBASE

Clone *clean room* do dBASE II para MSX, com melhorias de interface e evolucao
gradual de recursos do dBASE III. O escopo e o roteiro estao em
[SPEC.md](./SPEC.md).

Versao atual: **2.0 Fork**. Veja as [notas do release](./RELEASE.md) e o
[Changelog](./Changelog.MD).

## Estado atual

O projeto usa C/SDCC, MSXgl e MSX-DOS 2. A CLI oferece console de 80 colunas,
comandos de navegação e leitura de DBF II MSX e DBF III para interoperabilidade.
`APPEND BLANK` grava registros vazios em DBF II; os demais comandos de mutação e
`PACK` ainda estão em desenvolvimento.

## Compilar no Windows

Execute `.\build.ps1` na raiz do repositorio. O script chama o builder do
MSXgl usando os arquivos da pasta [project](./project/). É necessário ter
Node.js disponível no `PATH`, além das ferramentas SDCC e MSXhex esperadas em
`MSXgl/tools`.
O console usa a fonte ASCII embutida do MSXgl para que pontuação e símbolos
correspondam aos códigos das strings e para manter o cursor na linha de base.
O disco de teste inclui `PESSOAL.DBF` e `ALUNOS.DBF`, ambos DBF II MSX. A leitura
dos dois foi validada no openMSX; `APPEND BLANK` foi comparado byte a byte em
cópias, incluindo arquivos com tamanhos de registro diferentes.

O perfil está configurado para MSX2 (`Machine = "2"`) e MSX-DOS 2
(`Target = "DOS2"`). O build gera `project/out/msxdbase.com` e, quando a
versão Windows `MSXgl/tools/build/msxtar/msxtar.exe` está disponível, também
gera a imagem de disco `project/emul/dsk/DOS2_msxdbase.dsk`, com o sistema
MSX-DOS 2 e o executável.
O alvo de uso previsto é um MSX2 com 1024 KB de RAM; essa capacidade é uma
característica do hardware e não uma opção de compilação do formato `.COM`.

Argumentos adicionais são encaminhados ao builder do MSXgl, por exemplo:

```powershell
.\build.ps1 clean
```

O comando `VER` mostra a versao embutida no executavel. Cada compilacao
bem-sucedida incrementa automaticamente `Y` para a proxima compilacao. Durante
uma rodada de testes, mantenha a versao usando `.build.ps1 -KeepVersion`;
`clean` tambem nao incrementa a versao. O codinome pertence a `X` e muda quando
um novo marco comeca; o incremento de `X` e a publicacao do release sao feitos
deliberadamente, com tag `X.Y`, `RELEASE.md` e `Changelog.MD` atualizados juntos.

O esquema combina referencias a Unix e C classicos sem usar nomes de pessoas.
O primeiro codinome, **Shell**, evoca o shell do Unix e a familia C Shell.

Execute o `.COM` em um MSX2 com MSX-DOS 2. No prompt `.`, digite `HELP` para
ver os comandos disponiveis.

Se executado em MSX1, o programa agora exibe uma mensagem informando que MSX2+
e necessario e permite sair com ESC, em vez de aparentar travamento.
