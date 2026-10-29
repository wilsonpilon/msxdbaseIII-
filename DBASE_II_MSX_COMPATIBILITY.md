# Matriz de compatibilidade: dBASE II Plus para MSX

Esta matriz acompanha a conformidade com a edição MSX, não com uma implementação genérica de CP/M ou dBASE III. Os manuais impressos foram consultados localmente; os PDFs comerciais não são distribuídos neste repositório. Os números de página abaixo são os impressos nos livros/manuais; OCR foi usado para localizar conteúdo e pode conter erros, então detalhes de formato e regras críticas devem ser conferidos visualmente numa cópia licenciada.

Legenda: **Confirmado** = comportamento encontrado na fonte; **Parcial** = há evidência, mas falta definir ou testar detalhes; **Pendente** = ainda não implementado ou a fonte não foi localizada; **Validado** = executado no projeto.

## Arquivos e dados

| Requisito | Evidência MSX | Estado no projeto |
|---|---|---|
| Arquivo de dados `.DBF`, com registros e campos de tamanho fixo | Manual oficial, cap. 4, pp. 4-1 a 4-5; `resource/PESSOAL.DBF` | Leitor DBF II (`0x02`) validado no openMSX; DBF III (`0x03/0x83`) permanece para fixture/interoperabilidade |
| Tipos de campo compatíveis: Character, Numeric e Logical (`C/N/L`) | Manual oficial, cap. 4, pp. 4-3 a 4-5; descritores de `PESSOAL.DBF` | Leitor preserva e exibe C/N e interpreta L; validado com DBF II no openMSX |
| Nome de campo com até 10 caracteres; campo Character com até 254 posições | Manual oficial, cap. 4, pp. 4-3 a 4-5 | Pendente: confirmar limites completos e implementar |
| Precisão numérica publicada de 16 dígitos | Manual oficial, cap. 4, p. 4-5; apresentação do produto, cap. 1 | Pendente: definir representação numérica sem ponto flutuante |
| Até 65.535 registros | Manual oficial, cap. 4, p. 4-1 | Pendente: confirmar codificação do contador no DBF MSX |
| Cabeçalho DBF II | `PESSOAL.DBF`: versão `0x02`, contador `u16` little-endian nos bytes 1-2, data MM/DD/YY nos bytes 3-5, tamanho do registro `u16` nos bytes 6-7; cabeçalho fixo de 521 bytes | Parser e leitura validados no openMSX |
| Descritores DBF II | `PESSOAL.DBF`: 32 slots de 16 bytes desde byte 8; nome 0-10, tipo 11, tamanho 12, endereço em memória 13-14 (ignorado), decimais 15; byte 520 = `0x00` se menos de 32 campos ou `0x0D` com 32 | Leitura e `DISPLAY STRUCTURE` validados no openMSX; quatro campos conferidos |
| Registros DBF II | `PESSOAL.DBF`: início no byte 521; `RecLen=36`, seis registros, um byte de exclusão mais campos `C(15), C(10), N(5), C(5)` | `LIST`, `GO 6` e `DISPLAY` validados pelo usuário no openMSX |
| Registros DBF II | `ALUNOS.DBF`: início no byte 521; `RecLen=41`, seis registros e campos `N(5), C(30), N(3), C(1), N(1)` | `USE ALUNOS`, `LIST`, `DISPLAY STRUCTURE`, `GO 6` e `DISPLAY` validados pelo usuário no openMSX |
| Padding/EOF físico e `APPEND BLANK` | DSK original e `PESSOAL.DBF` usam `RecLen=36`, EOF `0x2E1`; `ALUNOS.DBF` usa `RecLen=41`, EOF `0x2FF`. No original, append substitui EOF por registro de espaços, atualiza contador/data, grava novo `0x1A` e preenche o tail até o tamanho físico com `0xFF` | Implementado e comparado byte a byte em cópias de `PESSOAL` e `ALUNOS`; tamanhos ficaram 1024 bytes e os registros anteriores permaneceram iguais. `PACK` e rollback em erro ainda pendentes |

## Comandos e comportamento

| Comando/área | Regra confirmada para MSX | Estado no projeto |
|---|---|---|
| `USE` | Abre ou fecha o arquivo ativo; o manual descreve extensão `.DBF` | `USE PESSOAL` e `USE ALUNOS` validados com DBF II no openMSX |
| `CREATE` | Define estrutura por nome, tipo, tamanho e casas decimais; tipos listados: `C/N/L` | Pendente |
| `LIST`, `DISPLAY`, `GO`, `SKIP` | Referência do manual, cap. 6; navegação por registro físico/ordem ativa | `LIST`, `DISPLAY STRUCTURE`, `GO 6` e `DISPLAY` validados com DBF II (`PESSOAL` e `ALUNOS`); `SKIP` permanece validado com DBF III |
| `DISPLAY FILES` | Extensão utilitária do projeto; não faz parte do conjunto oficial dBASE II | Sem padrão lista DBFs com contagem/data do cabeçalho; `LIKE` usa padrão literal; runtime com e sem correspondências validado no openMSX |
| `APPEND` | Entrada interativa de um registro por vez; atualiza índices ativos | Formulário C/N/L DBF II testado no openMSX: navegação, correção de Numeric/Logical, campos vazios, cancelamento e gravação. Índices não implementados |
| `APPEND BLANK` | Adiciona um registro em branco sem entrar na tela de inclusão; no original MSX, substitui EOF, atualiza contador/data e mantém o tamanho físico preenchendo o tail com `0xFF` | Implementado e validado byte a byte em DBF II (`PESSOAL`, `ALUNOS`); `CLIENTES.DBF` (`0x03`) é recusado como esperado; falta confirmar por comparação que o arquivo recusado não mudou |
| `INSERT [BEFORE] [BLANK]` | Insere em relacao ao ponteiro atual; `BEFORE` posiciona antes; `BLANK` suprime a entrada interativa | Implementado para DBF II com troca `.TMP/.BAK`; insercoes interativas e vazias testadas em copia. Limites BOF/EOF, rollback de falha e indice ativo pendentes |
| `SET CONFIRM ON/OFF` | ON (padrao) exige confirmacao para avancar; OFF avanca ao preencher o campo e, no ultimo, inicia outro registro | Implementado para os formularios `APPEND` e `INSERT`; ON/OFF validados no openMSX |
| `HELP`, `SET HELP`, `SET MSX` | dBASE II MSX inclui arquivo de mensagens com introducao e topicos de comando; o modo interativo usa HELP por palavra-chave | `HELP` sem argumentos mostra resumo local; `HELP DBASE` e `HELP <COMANDO>` dependem de `DB8SEMSG.TXT` fornecido pelo usuario. TXT validado; viewer SCREEN implementado, validacao visual final pendente |
| `REPLACE` | Altera campos; sem escopo/cláusula, atua no registro atual; índices ativos precisam ser atualizados | `REPLACE [ALL] campo WITH expressão [FOR condição]` implementado para Numeric; `FOR` aceita uma comparação numérica. Runtime validado: `IDADE < 12` alterou 3 de 6 registros. `WHILE` e gramática completa pendentes |
| `DELETE` | Marca o registro com `*`; não o remove fisicamente | Implementado para o registro atual em DBF II; teste funcional reportado pelo usuário, comparação byte a byte pendente |
| `RECALL` | Recupera registros marcados, sujeito ao escopo; não recupera registros já removidos por `PACK` | Implementado para o registro atual em DBF II; teste funcional reportado pelo usuário, comparação byte a byte pendente |
| `PACK` | Remove fisicamente registros marcados; o manual informa atualização de índices e liberação do espaço após fechar o arquivo | Implementado com `.TMP` e `.BAK` para DBF II; teste funcional reportado pelo usuário; recuperação de falhas e integração com NDX pendentes |
| `SET DELETED` | Controla a visibilidade de registros marcados para outros comandos | `SET DELETED ON/OFF` implementado (padrão OFF); ocultação do registro marcado validada em `LIST`; `DISPLAY` e `REPLACE ALL` usam o mesmo filtro. `GO`/`SKIP` continuam físicos |
| `INDEX`, `FIND`, `REINDEX`, `SET INDEX` | Fazem parte da edição MSX; índices `.NDX`, até sete arquivos ativos e chave de até 100 caracteres, conforme manual | Pendente; pertence ao núcleo dBASE II MSX, não a extensão dBASE III |
| Modo interativo | Prompt `.` e comandos executados diretamente; livro interativo detalha navegação, edição e exclusão | Parcial: histórico, TAB, edição horizontal e limpeza da linha ao descer além do histórico implementados e testados pelo usuário; ainda faltam outras funções de edição e interação dos comandos de tela |
| Modo programável | Arquivos de comando executados com `DO`; livros descrevem entrada, controle de fluxo e edição de dados via programas | Pendente: lexer/parser, variáveis, expressões e execução `.PRG` |

## Classificação das extensões

| Recurso | Classificação | Decisão |
|---|---|---|
| DBF III (`0x03/0x83`) usado por `CLIENTES.DBF` | Formato de outra versão | Manter como fixture de desenvolvimento/interoperabilidade, não como formato de saída do núcleo MSX |
| Campos Date e Memo/DBT | Não listados entre os tipos básicos `C/N/L` do manual MSX consultado | Tratar como extensão posterior, a menos que outra seção da referência MSX confirme suporte |
| Áreas adicionais, aliases e relações | Além das duas áreas Primary/Secondary descritas para dBASE II | Extensão posterior; verificar cada comando contra o manual MSX |
| Limitações de comandos específicas da edição MSX | Manual oficial, cap. 1 e notas dos comandos; há aviso de comandos não implementados na versão MSX | Pendente: identificar e catalogar individualmente antes de declarar cobertura total |

## Fontes consultadas localmente

- Manual oficial dBASE II Plus para MSX, capítulos 1, 4, 6 e 7 (cópia licenciada local; não incluída no release).
- Nelson Casari, *dBASE II Plus para MSX Interativo*, 2ª ed. (referência local; não incluída no release).
- Nelson Casari, *dBASE II Plus para MSX Programável*, 2ª ed. (referência local; não incluída no release).
- `resource/PESSOAL.DBF`: arquivo criado no dBASE II para MSX, usado para confirmar o cabeçalho e os descritores binários da variante `0x02`.

A matriz é incremental. “Compatibilidade 100%” só deve ser declarada quando os itens aplicáveis estiverem implementados e cobertos por testes contra arquivos/comportamentos observados no software dBASE II Plus para MSX original.
