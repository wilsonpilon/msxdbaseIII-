# Matriz de compatibilidade: dBASE II Plus para MSX

Esta matriz acompanha a conformidade com a edição MSX, não com uma implementação genérica de CP/M ou dBASE III. Os PDFs em `resource/` são as fontes primárias. Os números de página abaixo são os impressos nos livros/manuais; OCR foi usado para localizar conteúdo e pode conter erros, então detalhes de formato e regras críticas devem ser conferidos visualmente na página original.

Legenda: **Confirmado** = comportamento encontrado na fonte; **Parcial** = há evidência, mas falta definir ou testar detalhes; **Pendente** = ainda não implementado ou a fonte não foi localizada; **Validado** = executado no projeto.

## Arquivos e dados

| Requisito | Evidência MSX | Estado no projeto |
|---|---|---|
| Arquivo de dados `.DBF`, com registros e campos de tamanho fixo | Manual oficial, cap. 4, pp. 4-1 a 4-5 | Pendente: a engine atual lê DBF III, não a variante MSX |
| Tipos de campo compatíveis: Character, Numeric e Logical (`C/N/L`) | Manual oficial, cap. 4, pp. 4-3 a 4-5 | Pendente na engine MSX |
| Nome de campo com até 10 caracteres; campo Character com até 254 posições | Manual oficial, cap. 4, pp. 4-3 a 4-5 | Pendente: confirmar limites completos e implementar |
| Precisão numérica publicada de 16 dígitos | Manual oficial, cap. 4, p. 4-5; apresentação do produto, cap. 1 | Pendente: definir representação numérica sem ponto flutuante |
| Até 65.535 registros | Manual oficial, cap. 4, p. 4-1 | Pendente: confirmar codificação do contador no DBF MSX |
| Cabeçalho DBF II | `PESSOAL.DBF`: versão `0x02`, contador `u16` little-endian nos bytes 1-2, data MM/DD/YY nos bytes 3-5, tamanho do registro `u16` nos bytes 6-7; cabeçalho fixo de 521 bytes | Implementado no leitor; ainda validar no openMSX |
| Descritores DBF II | `PESSOAL.DBF`: 32 slots de 16 bytes desde byte 8; nome 0-10, tipo 11, tamanho 12, endereço em memória 13-14 (ignorado), decimais 15; byte 520 = `0x00` se menos de 32 campos ou `0x0D` com 32 | Implementado no leitor; quatro campos conferidos |
| Registros DBF II | `PESSOAL.DBF`: início no byte 521; `RecLen=36`, seis registros, um byte de exclusão mais campos `C(15), C(10), N(5), C(5)` | Implementado no leitor; conteúdo decodificado por análise binária, falta execução no emulador |
| Padding/EOF físico | A amostra tem 1024 bytes embora os seis registros ocupem 216 bytes após o cabeçalho | Pendente: não presumir terminador nem semântica de padding para escrita/`PACK` |

## Comandos e comportamento

| Comando/área | Regra confirmada para MSX | Estado no projeto |
|---|---|---|
| `USE` | Abre ou fecha o arquivo ativo; o manual descreve extensão `.DBF` | Parcial: parser agora aceita DBF II MSX e DBF III; testar `USE PESSOAL` no openMSX |
| `CREATE` | Define estrutura por nome, tipo, tamanho e casas decimais; tipos listados: `C/N/L` | Pendente |
| `LIST`, `DISPLAY`, `GO`, `SKIP` | Referência do manual, cap. 6; navegação por registro físico/ordem ativa | Validado pelo usuário no openMSX com DBF III; executar novamente usando `PESSOAL.DBF` |
| `APPEND` | Entrada interativa de um registro por vez; atualiza índices ativos | Pendente |
| `APPEND BLANK` | Adiciona um registro em branco sem entrar na tela de inclusão | Pendente |
| `REPLACE` | Altera campos; sem escopo/cláusula, atua no registro atual; índices ativos precisam ser atualizados | Pendente |
| `DELETE` | Marca o registro com `*`; não o remove fisicamente | Pendente |
| `RECALL` | Recupera registros marcados, sujeito ao escopo; não recupera registros já removidos por `PACK` | Pendente |
| `PACK` | Remove fisicamente registros marcados; o manual informa atualização de índices e liberação do espaço após fechar o arquivo | Pendente; requer substituição segura e testes de falha |
| `SET DELETED` | Controla a visibilidade de registros marcados para outros comandos | Pendente |
| `INDEX`, `FIND`, `REINDEX`, `SET INDEX` | Fazem parte da edição MSX; índices `.NDX`, até sete arquivos ativos e chave de até 100 caracteres, conforme manual | Pendente; pertence ao núcleo dBASE II MSX, não a extensão dBASE III |
| Modo interativo | Prompt `.` e comandos executados diretamente; livro interativo detalha navegação, edição e exclusão | Parcial: prompt básico existe; falta a interação compatível dos comandos de tela |
| Modo programável | Arquivos de comando executados com `DO`; livros descrevem entrada, controle de fluxo e edição de dados via programas | Pendente: lexer/parser, variáveis, expressões e execução `.PRG` |

## Classificação das extensões

| Recurso | Classificação | Decisão |
|---|---|---|
| DBF III (`0x03/0x83`) usado por `CLIENTES.DBF` | Formato de outra versão | Manter como fixture de desenvolvimento/interoperabilidade, não como formato de saída do núcleo MSX |
| Campos Date e Memo/DBT | Não listados entre os tipos básicos `C/N/L` do manual MSX consultado | Tratar como extensão posterior, a menos que outra seção da referência MSX confirme suporte |
| Áreas adicionais, aliases e relações | Além das duas áreas Primary/Secondary descritas para dBASE II | Extensão posterior; verificar cada comando contra o manual MSX |
| Limitações de comandos específicas da edição MSX | Manual oficial, cap. 1 e notas dos comandos; há aviso de comandos não implementados na versão MSX | Pendente: identificar e catalogar individualmente antes de declarar cobertura total |

## Fontes consultadas

- `resource/Dbase 2 Plus Msx.pdf`: manual oficial, capítulos 1, 4, 6 e 7.
- `resource/dbase_ii_plus_interativo_para_msx.pdf`: Nelson Casari, *dBASE II Plus para MSX Interativo*, 2ª ed.; capítulos sobre estrutura, modos, alteração e exclusão.
- `resource/dbase_programavel_msx.pdf`: Nelson Casari, *dBASE II Plus para MSX Programável*, 2ª ed.; capítulos sobre arquivos de comando, entrada/alteração de dados e fluxo de programa.
- `resource/PESSOAL.DBF`: arquivo criado no dBASE II para MSX, usado para confirmar o cabeçalho e os descritores binários da variante `0x02`.

A matriz é incremental. “Compatibilidade 100%” só deve ser declarada quando os itens aplicáveis estiverem implementados e cobertos por testes contra arquivos/comportamentos observados no software dBASE II Plus para MSX original.
