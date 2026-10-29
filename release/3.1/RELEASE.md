# Grep 3.1

Release 3.1 do MSX dBASE II+ para MSX2/MSX-DOS 2.

## Destaques

- `INSERT [BEFORE] [BLANK]` para DBF II, com insercao posicional e formulario C/N/L.
- `SET CONFIRM ON/OFF` controla o avanco dos formularios `APPEND` e `INSERT`.
- `HELP` sem argumentos mostra um resumo local e nao tenta abrir um arquivo externo.
- `Help_Init` inicializa o estado do indice; a tabela tem limite de 160 topicos.
- O build aceita `DB8SEMSG.TXT` opcionalmente. O arquivo original nao e distribuido.

## Validacao

- Build completo SDCC/MSX-DOS 2 passou.
- `HELP` em modo TXT foi executado em uma copia temporaria e mostrou o resumo local.
- Mutacoes DBF II foram exercitadas em copias temporarias.
- O viewer SCREEN foi implementado, mas a validacao visual final desta versao continua pendente.

## Instalacao

Copie `msxdbase-3.1.com` para um disco MSX-DOS 2. Para ativar `HELP DBASE` e `HELP <COMANDO>`, use sua propria copia licenciada de `DB8SEMSG.TXT` no diretorio atual. Sem esse arquivo, o resumo local de `HELP` continua disponivel.

## Conteudo

- `msxdbase-3.1.com`: executavel MSX-DOS 2.

O release nao inclui imagem DSK, manuais comerciais ou arquivos originais do dBASE, WordStar ou SuperCalc.
