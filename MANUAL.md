# Manual do usuario

## Iniciar

No openMSX, monte a imagem do MSX-DOS 2:

```powershell
openmsx -machine Boosted_MSX2_EN -ext msxdos2 -diska .\project\emul\dsk\DOS2_msxdbase.dsk
```

No prompt `.`, use `HELP` para ver introducao/resumos ou `VER` para consultar a versao.

## Ajuda online

`HELP` sem argumentos mostra imediatamente o resumo local dos comandos. `HELP DBASE` e `HELP <COMANDO>` tentam abrir topicos completos de `DB8SEMSG.TXT`, por exemplo `HELP INSERT`. Esse arquivo original e proprietario e nao e distribuido com o projeto; sem ele, o resumo local continua disponivel e os topicos detalhados informam que o arquivo esta ausente.

O padrao e `SET MSX OFF` com `SET HELP TXT`: a ajuda aparece no prompt e pausa por pagina. `SET HELP SCREEN` seleciona um visualizador em janela; para ativar a janela, use `SET MSX ON`.

- `Up`/`Down`: rolar uma linha.
- `Space`, `Enter` ou `Tab`: proxima pagina; `Backspace`: pagina anterior.
- `/` ou `F`: buscar texto; `N`: proxima ocorrencia.
- `Left`/`Right`: deslocamento horizontal; `Esc`: fechar a janela.

`SET MSX OFF` e o padrao: cores e viewer SCREEN ficam desativados e HELP usa TXT. `SET MSX ON` habilita cores e permite o viewer. A escolha de `SET HELP SCREEN` e preservada ao desligar MSX e volta a valer quando MSX e ligado. A janela foi implementada, mas a validacao visual final da versao 3.1 ainda esta pendente.

## Abrir e consultar bancos

`USE nome` abre um arquivo DBF do diretorio atual. A extensao `.DBF` pode ser omitida.

```text
USE ALUNOS
DISPLAY STRUCTURE
LIST
GO 3
DISPLAY
SKIP -1
GO BOTTOM
```

`LIST` mostra os registros; `DISPLAY` mostra o registro atual. `GO` posiciona por numero ou em `TOP`/`BOTTOM`; `SKIP` move a posicao atual. `DISPLAY STRUCTURE` mostra os campos.

`DISPLAY FILES` lista os arquivos DBF do diretorio atual. Para outros nomes, use `LIKE` com um padrao literal:

```text
DISPLAY FILES
DISPLAY FILES LIKE *.*
DISPLAY FILES LIKE P*
DISPLAY FILES LIKE *.FRM
```

O padrao nao recebe uma extensao automatica. Uma busca sem resultados informa que nenhum arquivo corresponde.

## Acrescentar registros

Em DBF II MSX, `APPEND` abre todos os campos C, N e L em uma unica tela. Os dois-pontos delimitam o inicio e o fim de cada faixa; `SET COLON OFF` oculta ambos e `SET COLON ON` restaura ambos.

- `Enter`, `Tab` ou `Down`: proximo campo; `Enter` no ultimo salva.
- `Up`: campo anterior.
- `Left` e `Right`: mover o cursor dentro do campo.
- `Backspace` e `Delete`: apagar antes ou sob o cursor.
- `Esc`: cancelar sem gravar.

Valores invalidos permanecem no campo para correcao. Campos vazios sao gravados como espacos. Texto fica alinhado a esquerda, Numeric a direita, e Logical aceita `T`, `Y`, `F` ou `N`, normalizando para `T`/`F`.

`APPEND BLANK` acrescenta diretamente um registro preenchido com espacos. Mutacoes sao suportadas somente em DBF II MSX (`0x02`); DBF III permanece disponivel para leitura.

`SET CONFIRM ON` e o padrao: preencher toda a faixa nao avanca sozinho; use Enter, Tab ou Down para confirmar. Com `SET CONFIRM OFF`, o preenchimento do campo avanca automaticamente. Ao completar o ultimo campo, o registro e gravado e um novo formulario comeca. Isso se aplica a `APPEND` e `INSERT`; `Esc` cancela o formulario atual, mas nao desfaz registros ja gravados automaticamente.

## Inserir na posicao atual

`INSERT [BEFORE] [BLANK]` insere em relacao ao ponteiro de registro atual. Sem `BEFORE`, o novo registro fica depois do atual; com `BEFORE`, fica antes. O ponteiro termina no registro inserido. No BOF a insercao fica no inicio; no EOF fica no fim.

- `INSERT`: abre o formulario C/N/L e grava os valores validados.
- `INSERT BEFORE`: abre o formulario e insere antes do registro atual.
- `INSERT BLANK`: insere um registro preenchido com espacos depois do atual.
- `INSERT BEFORE BLANK`: insere um registro vazio antes do atual.

Os dois modificadores sao opcionais e devem aparecer nesta ordem. O comando aceita somente DBF II MSX e recusa a operacao se `.TMP` ou `.BAK` do banco ja existirem.

## Atualizar registros

`REPLACE campo WITH valor` atualiza o campo no registro atual. `REPLACE ALL` atualiza todos os registros, com expressoes numericas basicas (`+`, `-`, `*`, `/`, parenteses e campos N):

```text
REPLACE IDADE WITH 25
REPLACE ALL SALARIO WITH SALARIO * 1.1
```

Uma condicao numerica pode limitar `REPLACE ALL`:

```text
REPLACE ALL IDADE WITH IDADE + 5 FOR IDADE < 12
```

Por enquanto, `FOR` aceita uma comparacao numerica (`=`, `<>`, `!=`, `<`, `<=`, `>` ou `>=`) por comando, sem `AND`/`OR`.

`DELETE` marca o registro atual; `RECALL` remove a marca. `SET DELETED ON` oculta registros marcados em `LIST`, `DISPLAY` do registro atual e `REPLACE ALL`; `SET DELETED OFF` volta a mostra-los. O padrao e `OFF`; `GO` e `SKIP` continuam navegando registros fisicos.

`PACK` remove fisicamente registros excluidos. Use-o somente em uma copia de seguranca ate que os testes de recuperacao estejam completos.

## Console

O prompt guarda os ultimos oito comandos. `Up` e `Down` percorrem o historico, `Left`/`Right` movem o cursor, `Backspace`/`Delete` editam a linha e `Tab` completa nomes de comandos pelo prefixo.

`SET COLOR TO` muda as cores do console. `CLEAR` limpa a tela; `QUIT` encerra o programa.

## Limites atuais

- O formato de escrita e DBF II MSX; bancos DBF III sao somente para leitura.
- `FOR` esta limitado a uma comparacao numerica em `REPLACE ALL`.
- A linguagem de expressoes ainda e um subconjunto xBase; recursos como `WHILE`, `SET FILTER`, `AND` e `OR` nao estao implementados.
- Mantenha copias dos bancos antes de usar operacoes de mutacao.
