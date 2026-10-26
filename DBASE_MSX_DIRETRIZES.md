# Diretrizes de Desenvolvimento — xBase/dBASE para MSX

> Documento complementar ao `OUTLINE.md` e ao `SPEC.md`.
>
> Finalidade: consolidar as decisões, raciocínios e diretrizes discutidas sobre a criação de um ambiente xBase inspirado no dBASE II/dBASE III, mas projetado especificamente para computadores MSX.

---

## 1. Visão do projeto

O projeto não deve ser um simples porte da versão CP/M do dBASE II+ nem uma tentativa de reproduzir integralmente o dBASE III.

A proposta é criar um **xBase nativo para MSX**, informalmente entendido como um **“dBASE 2.5/3 para MSX”**:

- compatível, quando viável, com comandos e conceitos do dBASE II;
- incorporando seletivamente recursos importantes do dBASE III;
- aproveitando recursos próprios do MSX2, MSX2+ e MSX turbo R;
- adequado às limitações do Z80 e ao espaço lógico de 64 KB;
- capaz de utilizar Memory Mapper, MSX-DOS 2 e armazenamento moderno;
- com interface mais amigável do que a versão CP/M existente;
- desenvolvido de forma modular, testável e progressiva.

O produto deve se comportar como um pequeno ambiente integrado de banco de dados e programação, não apenas como um visualizador de arquivos DBF.

---

## 2. Contexto e motivação

O MSX recebeu uma versão dBASE II+ para CP/M, destinada originalmente a uma configuração muito mais limitada:

- MSX1;
- Z80 em torno de 3,58 MHz;
- 64 KB de RAM;
- unidade de 360 KB;
- ambiente semelhante ao CP/M/MSX-DOS 1;
- interface textual genérica;
- pouco ou nenhum aproveitamento de recursos específicos do MSX.

Essa versão não explora adequadamente:

- cores;
- teclas de direção;
- caracteres gráficos;
- janelas e menus;
- mouse;
- VDP dos MSX2/2+;
- Memory Mapper;
- maior capacidade de RAM;
- MSX-DOS 2;
- discos rígidos, SD e interfaces modernas de armazenamento;
- processadores acelerados e o R800 do MSX turbo R.

O alvo deste projeto é uma máquina muito mais capaz do que o MSX1 mínimo para o qual o software antigo foi concebido.

---

## 3. Comparação conceitual: dBASE II e dBASE III

A passagem do dBASE II para o dBASE III foi uma evolução substancial, não apenas uma atualização cosmética.

### 3.1 dBASE II

Características relevantes:

- forte associação histórica com CP/M;
- arquivos `.DBF`;
- índices `.NDX`;
- duas áreas de trabalho, tradicionalmente PRIMARY e SECONDARY;
- tipos de campo básicos, principalmente Character, Numeric e Logical;
- arquivos de comandos;
- linguagem de programação funcional, porém mais limitada;
- comandos de banco e relatório já suficientemente ricos para aplicações comerciais.

Comandos importantes associados ao modelo dBASE II incluem:

```text
USE
SELECT
LIST
DISPLAY
APPEND
EDIT
DELETE
RECALL
PACK
REPLACE
LOCATE
CONTINUE
GO
SKIP
INDEX
FIND
SEEK
SORT
COUNT
SUM
AVERAGE
TOTAL
JOIN
REPORT
MODIFY COMMAND
SAVE
RESTORE
SET FILTER TO
SET INDEX TO
SET DELETED
SET EXACT
```

### 3.2 dBASE III

Avanços que inspiram este projeto:

- implementação de nova geração;
- linguagem de programação mais ampla;
- mais áreas de trabalho;
- campo Date;
- campo Memo associado a arquivos `.DBT`;
- maior capacidade para aplicações completas;
- melhor manipulação de variáveis e expressões;
- ambiente mais próximo de uma plataforma de desenvolvimento;
- evolução dos formatos e limites de arquivos.

### 3.3 Conclusão para o projeto MSX

O objetivo não é reproduzir cada detalhe do dBASE III. O projeto deve selecionar recursos que tragam alto valor ao MSX sem impor complexidade incompatível com o Z80.

A prioridade deve ser:

1. núcleo dBASE II funcional e compatível;
2. arquitetura extensível;
3. recursos selecionados do dBASE III;
4. interface nativa e confortável no MSX;
5. estabilidade dos arquivos e do interpretador;
6. desempenho aceitável em armazenamento real de MSX.

---

## 4. Plataforma-alvo

### 4.1 Alvo recomendado

```text
MSX2 ou MSX2+
Z80 a partir de 3,58 MHz
MSX-DOS 2 recomendado
Memory Mapper
512 KB recomendados
1 MB ideal
HDD, SD ou armazenamento equivalente
```

O MSX turbo R deve ser suportado e pode receber otimizações, mas não deve ser o único alvo.

### 4.2 Configuração mínima proposta

```text
MSX2
128 KB de RAM
MSX-DOS 1 ou 2, conforme o subconjunto funcional
armazenamento em disco
modo texto de 80 colunas quando disponível
```

A configuração mínima exata deverá ser validada por protótipos e testes de consumo de memória.

### 4.3 Perfis sugeridos

#### Perfil básico

- MSX2;
- 128 KB;
- operações DBF fundamentais;
- interpretador reduzido;
- uma ou duas áreas de trabalho;
- sem memo ou com memo opcional;
- interface baseada em teclado.

#### Perfil recomendado

- MSX2/2+;
- 512 KB;
- MSX-DOS 2;
- múltiplas áreas de trabalho;
- índices;
- memo;
- editor de comandos;
- relatórios;
- interface com janelas e mouse opcional.

#### Perfil avançado

- MSX2+ ou turbo R;
- 1 MB ou mais;
- armazenamento rápido;
- cache ampliado;
- mais buffers e áreas de trabalho;
- otimizações específicas para CPU acelerada/R800.

---

## 5. Restrição central: memória do Z80

Mesmo com 512 KB ou 1 MB instalados, o Z80 continua trabalhando com um espaço lógico de 64 KB por vez.

Portanto, o projeto não deve ser estruturado como um programa monolítico que presume RAM linear abundante.

A arquitetura deve separar:

- código residente;
- módulos carregáveis;
- buffers de registros;
- páginas de índice;
- cache de disco;
- bytecode/programas compilados;
- estruturas temporárias;
- dados da interface.

O Memory Mapper deve ser usado para segmentos de 16 KB quando disponível.

### 5.1 Uso recomendado do espaço residente

Devem permanecer residentes apenas os componentes indispensáveis:

- núcleo de I/O;
- gerenciador de memória;
- laço principal;
- parser mínimo;
- dispatcher de comandos;
- estado das áreas de trabalho;
- interface básica;
- tratamento de erros.

### 5.2 Candidatos a segmentos mapeados ou overlays

- engine de relatórios;
- editor de comandos;
- rotinas de ordenação;
- manutenção de índices;
- suporte a memo;
- ajuda online;
- drivers de interface avançada;
- comandos raramente usados;
- compilador para bytecode.

### 5.3 Regra de projeto

Não assumir que “1 MB de RAM” equivale a “1 MB linear disponível”. Toda alocação importante deve considerar paginação, troca de segmentos e custo de cópia.

---

## 6. Ferramentas de desenvolvimento

### 6.1 Escolha principal

A recomendação discutida foi:

```text
SDCC + MSXgl + MSX-DOS 2 + Memory Mapper
```

### 6.2 Motivos para preferir MSXgl

O MSXgl foi considerado mais apropriado para este projeto por reunir infraestrutura útil para:

- programas MSX-DOS;
- teclado;
- VDP;
- impressão e strings;
- heap e memória;
- RTC;
- slots;
- mouse;
- Memory Mapper;
- suporte a V9938/V9958;
- geração de programas em C para MSX.

Embora tenha origem e nome associados a jogos, suas bibliotecas podem servir como camada de hardware e sistema para uma aplicação de banco de dados.

### 6.3 Fusion-C

Fusion-C também é tecnicamente viável e oferece recursos importantes, incluindo:

- SDCC;
- arquivos sob MSX-DOS;
- teclado;
- vídeo;
- Memory Mapper no MSX-DOS 2.

Mesmo assim, a preferência inicial é por MSXgl, principalmente pela arquitetura e cobertura esperada para um projeto novo e modular.

### 6.4 Regra de portabilidade

A engine de banco, o parser e a máquina virtual não devem depender diretamente da biblioteca de interface.

Criar camadas de abstração, por exemplo:

```c
platform_file_open();
platform_file_read();
platform_file_write();
platform_alloc_segment();
platform_map_segment();
platform_console_write();
platform_keyboard_read();
platform_screen_set_mode();
```

Com isso será possível:

- trocar MSXgl por outra camada, se necessário;
- criar testes em ambiente host;
- portar ferramentas auxiliares para Windows/Linux;
- testar formatos DBF/NDX fora do MSX;
- manter o núcleo independente da UI.

---

## 7. Arquitetura geral proposta

```text
+--------------------------------------------------+
| Interface MSX: prompt, editor, menus, janelas    |
+--------------------------------------------------+
| Interpretador xBase / parser / máquina virtual   |
+--------------------------------------------------+
| Comandos, expressões, variáveis e runtime        |
+--------------------------------------------------+
| Áreas de trabalho e engine de registros          |
+--------------------------------------------------+
| DBF | NDX | DBT/Memo | filtros | relatórios      |
+--------------------------------------------------+
| Cache, buffers, mapper, overlays e memória        |
+--------------------------------------------------+
| MSXgl / abstração de plataforma / MSX-DOS        |
+--------------------------------------------------+
| BIOS, BDOS, VDP, teclado, mouse e armazenamento  |
+--------------------------------------------------+
```

### 7.1 Módulos sugeridos

```text
src/
├── core/
│   ├── runtime
│   ├── errors
│   ├── values
│   └── environment
├── parser/
│   ├── lexer
│   ├── parser
│   ├── expressions
│   └── compiler
├── vm/
│   ├── bytecode
│   ├── executor
│   └── stack
├── dbf/
│   ├── dbf_reader
│   ├── dbf_writer
│   ├── record_buffer
│   └── field_types
├── index/
│   ├── ndx_reader
│   ├── ndx_writer
│   └── seek
├── memo/
│   └── dbt
├── workarea/
│   ├── workarea
│   ├── filter
│   └── relations
├── commands/
│   ├── data
│   ├── navigation
│   ├── programming
│   ├── report
│   └── system
├── ui/
│   ├── console
│   ├── editor
│   ├── windows
│   ├── menus
│   └── mouse
├── platform/
│   ├── dos
│   ├── mapper
│   ├── vdp
│   ├── keyboard
│   └── filesystem
└── tests/
```

---

## 8. Estratégia de implementação da linguagem

Não implementar o interpretador como uma grande cadeia de comparações de strings.

A abordagem recomendada é:

```text
fonte .PRG
   ↓
lexer
   ↓
tokens
   ↓
parser
   ↓
representação intermediária ou bytecode compacto
   ↓
máquina virtual
   ↓
engine DBF/UI/sistema
```

### 8.1 Benefícios do bytecode

- menor custo de interpretar repetidamente palavras-chave;
- programas `.PRG` podem ser pré-compilados;
- comandos podem ser identificados por códigos compactos;
- execução tende a ser mais previsível;
- módulos do interpretador podem ser paginados;
- facilita depuração e testes;
- permite cache de programas compilados.

### 8.2 Modo interativo

O prompt deve continuar aceitando comandos diretamente:

```text
. USE CLIENTES
. LIST FOR ESTADO = "SP"
```

Nesse modo, uma linha pode ser tokenizada, compilada para um pequeno buffer temporário e executada imediatamente.

### 8.3 Programas

Arquivos `.PRG` devem permitir, progressivamente:

- sequências de comandos;
- variáveis;
- expressões;
- condições;
- laços;
- procedimentos;
- parâmetros;
- retorno;
- arquivos incluídos ou módulos, se a memória permitir.

---

## 9. Tipos de dados

### 9.1 Primeira fase

- Character;
- Numeric;
- Logical.

### 9.2 Segunda fase

- Date.

### 9.3 Terceira fase

- Memo, com arquivo associado `.DBT` ou formato compatível definido pelo projeto.

### 9.4 Valores internos

Evitar dependência excessiva de ponto flutuante.

Para campos Numeric, avaliar representação decimal fixa ou BCD, especialmente para preservar comportamento previsível de aplicações comerciais.

Toda conversão deve tratar explicitamente:

- largura;
- casas decimais;
- espaços;
- sinal;
- overflow;
- valor vazio;
- comparação;
- conversão texto/número.

---

## 10. Arquivos DBF

### 10.1 Objetivo inicial

Ler e escrever um subconjunto claramente definido de DBF compatível com os formatos escolhidos como referência.

### 10.2 Cuidados obrigatórios

- identificação exata da variante DBF;
- cabeçalho;
- descritores de campos;
- tamanho de registro;
- marca de exclusão;
- fim lógico do arquivo;
- codificação de caracteres;
- compatibilidade entre dBASE II e dBASE III;
- integridade após interrupção;
- gravação segura do cabeçalho e da contagem de registros.

### 10.3 Estratégia de segurança

Para operações destrutivas ou de reorganização:

- usar arquivos temporários;
- validar espaço livre antes da operação;
- preservar o original até o encerramento correto;
- permitir recuperação após falha;
- evitar atualizar metadados críticos cedo demais.

Isso se aplica especialmente a:

```text
PACK
SORT
INDEX
reconstrução de DBF
operações de memo
```

---

## 11. Índices NDX

O suporte a índices deve ser implementado em etapas.

### Etapa 1

- leitura de índices simples;
- `FIND`/`SEEK`;
- navegação ordenada;
- uma chave por índice.

### Etapa 2

- criação de índice;
- atualização após APPEND/REPLACE/DELETE;
- múltiplos índices abertos;
- expressões de chave.

### Etapa 3

- otimizações de cache;
- reconstrução;
- detecção de inconsistência;
- validação de árvore.

A implementação deve evitar carregar o índice inteiro na RAM. Usar cache de páginas e leitura sob demanda.

---

## 12. Campos Memo e DBT

Memo é um recurso desejável por aproximar o sistema do dBASE III, mas não deve bloquear o MVP.

Diretrizes:

- separar a engine Memo do núcleo DBF;
- definir blocos e alocação de forma explícita;
- tratar fragmentação;
- validar referências inválidas;
- fornecer ferramenta de reconstrução/verificação;
- evitar manter textos completos residentes na memória;
- editar memo por páginas ou buffers.

---

## 13. Áreas de trabalho

O dBASE II trabalha com duas áreas. O dBASE III amplia significativamente esse conceito.

A proposta deve ser progressiva:

### MVP

- duas áreas de trabalho;
- compatibilidade conceitual com PRIMARY/SECONDARY;
- `SELECT`;
- operações entre duas tabelas.

### Evolução

- quatro áreas em configuração intermediária;
- até dez áreas em máquinas com memória suficiente;
- quantidade configurável conforme o perfil de memória.

Cada área deve manter, no mínimo:

```text
arquivo DBF
registro atual
estado BOF/EOF
filtro
índice ativo
lista de índices
buffer de registro
marca de alteração
alias
relações
```

---

## 14. Comandos por fase

### 14.1 Fase essencial

```text
USE
CLOSE
SELECT
LIST
DISPLAY
APPEND
DELETE
RECALL
PACK
REPLACE
GO
SKIP
LOCATE
CONTINUE
COUNT
SUM
AVERAGE
SET DELETED
SET EXACT
SET FILTER TO
```

### 14.2 Índices

```text
INDEX ON
SET INDEX TO
FIND
SEEK
REINDEX
```

### 14.3 Programação

```text
DO
DO WHILE
ENDDO
IF
ELSE
ENDIF
STORE
RELEASE
RETURN
PARAMETERS
PROCEDURE
```

### 14.4 Ferramentas

```text
CREATE
MODIFY STRUCTURE
MODIFY COMMAND
DIR
COPY
SORT
TOTAL
JOIN
REPORT
LABEL
```

### 14.5 Recursos inspirados no dBASE III

- Date;
- Memo;
- mais áreas de trabalho;
- aliases;
- procedimentos mais estruturados;
- expressões mais ricas;
- editor integrado melhorado;
- ambiente de desenvolvimento mais coeso.

Cada comando deve ter especificação própria com:

- sintaxe;
- parâmetros;
- comportamento;
- erros;
- interação com filtro e índice;
- consumo de memória;
- compatibilidade pretendida;
- testes.

---

## 15. Interface nativa MSX

O projeto não deve repetir a experiência genérica CP/M da versão antiga.

### 15.1 Recursos desejados

- cores configuráveis;
- teclas de direção;
- Home/End/Page Up/Page Down quando disponíveis;
- edição de linha;
- histórico de comandos;
- janelas;
- menus;
- caracteres gráficos;
- caixas de diálogo;
- barra de estado;
- ajuda contextual;
- mouse opcional;
- modos de 40 e 80 colunas;
- atalhos por teclado.

### 15.2 Princípio de separação

A UI não pode contaminar a engine DBF nem o interpretador.

Funções esperadas:

```c
ui_window();
ui_message();
ui_prompt();
ui_get();
ui_browse();
ui_menu();
ui_editor();
```

### 15.3 Compatibilidade sem UI avançada

Deve existir modo console para:

- máquinas menores;
- automação;
- execução de `.PRG`;
- depuração;
- testes;
- uso em telas simples.

---

## 16. Funções internas desejadas

Exemplos de APIs conceituais:

```c
dbf_open();
dbf_close();
dbf_goto();
dbf_skip();
dbf_append();
dbf_read_field();
dbf_write_field();

ndx_open();
ndx_close();
ndx_seek();
ndx_insert();
ndx_rebuild();

workarea_select();
workarea_set_filter();

lexer_next();
parser_statement();
compiler_emit();
vm_execute();

ui_window();
ui_get();
ui_browse();
```

Esses nomes representam responsabilidades arquiteturais, não uma API final obrigatória.

---

## 17. Desempenho

Os principais gargalos esperados são:

- acesso ao disco;
- busca e atualização de índices;
- troca de segmentos;
- parsing repetitivo;
- ordenação;
- cópia de registros;
- baixa RAM linear.

### Estratégias

- buffers alinhados ao tamanho útil de setor/bloco;
- cache pequeno e previsível;
- leitura antecipada em navegação sequencial;
- gravação agrupada onde for segura;
- bytecode;
- índices por páginas;
- overlays;
- evitar chamadas gráficas durante processamento em lote;
- permitir modo silencioso para programas;
- medir desempenho em hardware real e emuladores.

---

## 18. Compatibilidade e extensões MSX

Comandos compatíveis devem manter sintaxe xBase sempre que razoável.

Extensões próprias do MSX devem ser claramente identificadas e isoladas, por exemplo:

```text
SET COLOR TO
SET SCREEN TO
SET MOUSE ON
SET WINDOW TO
MSXINFO()
MAPPERFREE()
```

Esses nomes são propostas, não especificação fechada.

Uma aplicação escrita apenas com o subconjunto compatível deve ter maior chance de portabilidade. Recursos MSX podem existir como extensões opcionais.

---

## 19. Estratégia de desenvolvimento incremental

### Marco 0 — infraestrutura

- build SDCC;
- execução `.COM`;
- abstração de arquivos;
- console;
- testes em emulador e hardware;
- detecção de MSX/MSX-DOS/Mapper.

### Marco 1 — DBF somente leitura

- abrir DBF;
- ler cabeçalho;
- listar campos;
- navegar registros;
- `USE`, `LIST`, `DISPLAY`, `GO`, `SKIP`.

### Marco 2 — escrita segura

- APPEND;
- REPLACE;
- DELETE;
- RECALL;
- atualização segura do cabeçalho.

### Marco 3 — expressões e filtros

- lexer;
- valores;
- operadores;
- `FOR`;
- `SET FILTER TO`;
- `LOCATE`/`CONTINUE`.

### Marco 4 — programas PRG

- parser de comandos;
- bytecode;
- variáveis;
- IF/ELSE;
- DO WHILE;
- execução de arquivos.

### Marco 5 — índices

- leitura NDX;
- SEEK/FIND;
- criação e atualização;
- reconstrução.

### Marco 6 — interface MSX

- editor de linha;
- browse;
- janelas;
- menus;
- ajuda;
- cores;
- mouse opcional.

### Marco 7 — recursos dBASE III selecionados

- Date;
- Memo/DBT;
- mais áreas;
- procedimentos;
- relatórios ampliados.

---

## 20. Testes obrigatórios

### 20.1 Testes de formato

- DBF vazio;
- DBF com registros excluídos;
- campos no limite de tamanho;
- números negativos;
- decimais;
- registros truncados;
- cabeçalho corrompido;
- EOF ausente;
- arquivos maiores que a RAM.

### 20.2 Testes de integridade

- interrupção durante APPEND;
- interrupção durante PACK;
- disco cheio;
- arquivo somente leitura;
- índice desatualizado;
- DBT ausente;
- referência memo inválida.

### 20.3 Testes de plataforma

- MSX2 básico;
- MSX2+;
- turbo R;
- 128/256/512 KB e 1 MB;
- MSX-DOS 1 quando suportado;
- MSX-DOS 2;
- diferentes interfaces de armazenamento;
- emuladores e máquinas reais.

### 20.4 Golden files

Manter pequenos bancos conhecidos e comparar byte a byte os resultados esperados após operações.

---

## 21. Decisões que devem permanecer explícitas

1. **Não portar literalmente o dBASE III.**
2. **Não se limitar à experiência CP/M.**
3. **Preservar compatibilidade xBase onde ela trouxer valor.**
4. **Escolher recursos do dBASE III com base em utilidade e custo.**
5. **Usar Memory Mapper e modularização, não um executável monolítico.**
6. **Separar engine, linguagem, UI e plataforma.**
7. **Priorizar DBF íntegro antes de interface sofisticada.**
8. **Fazer índices e memo em fases posteriores ao núcleo DBF.**
9. **Manter modo console mesmo com UI avançada.**
10. **Projetar para hardware real, não apenas emuladores.**
11. **Usar SDCC + MSXgl como caminho principal inicial.**
12. **Tratar Fusion-C como alternativa viável, não como dependência simultânea.**
13. **Documentar extensões MSX separadamente da compatibilidade dBASE.**
14. **Não assumir memória linear por causa do Memory Mapper.**
15. **Nunca sacrificar integridade de dados por uma otimização prematura.**

---

## 22. Relação com OUTLINE e SPEC

Este documento deve ser usado como a terceira peça de orientação:

```text
OUTLINE.md
    define visão, fases e organização geral

SPEC.md
    define comportamentos, formatos, interfaces e requisitos verificáveis

DBASE_MSX_DIRETRIZES.md
    preserva decisões arquiteturais, prioridades, justificativas e limites
```

Em caso de conflito:

1. prevalece a decisão mais recente e explicitamente aprovada;
2. `SPEC.md` deve prevalecer para comportamento implementável e testável;
3. este documento deve orientar arquitetura e intenção;
4. `OUTLINE.md` deve orientar sequência e escopo;
5. conflitos devem ser corrigidos nos três documentos, evitando divergência permanente.

---

## 23. Instruções para o Copilot

Ao desenvolver este projeto, o Copilot deve:

- ler `OUTLINE.md`, `SPEC.md` e este arquivo antes de propor mudanças estruturais;
- não transformar o projeto em clone integral do dBASE III;
- não remover compatibilidade já implementada sem justificativa;
- manter módulos pequenos e responsabilidades claras;
- considerar limites reais de memória do Z80;
- evitar bibliotecas que presumam RAM linear ampla;
- não misturar a UI com a engine DBF;
- propor testes para cada alteração em formatos de arquivo;
- documentar incompatibilidades;
- preservar arquivos existentes e migrar formatos somente com ferramentas explícitas;
- evitar grandes refatorações sem necessidade;
- implementar um marco funcional por vez;
- manter o projeto compilável após cada etapa;
- diferenciar claramente código MSX, código portável e ferramentas host;
- informar impacto em RAM, disco e desempenho quando adicionar funcionalidades;
- preferir comportamento determinístico e integridade de dados;
- não inventar compatibilidade com dBASE sem teste ou documentação;
- atualizar documentação e testes junto com o código.

### Critérios para aceitar uma nova funcionalidade

Antes de implementar, verificar:

1. faz parte do núcleo dBASE II, de um recurso selecionado do III ou de uma extensão MSX?
2. em qual marco ela entra?
3. qual o impacto na RAM residente?
4. exige segmento mapeado ou overlay?
5. altera DBF, NDX ou DBT?
6. como será testada?
7. funciona sem UI gráfica?
8. funciona em arquivo maior que a memória?
9. mantém integridade após erro?
10. precisa ser compatível com arquivos de outra implementação?

---

## 24. Definição resumida do produto

> Um ambiente xBase nativo para MSX, inspirado no dBASE II e enriquecido seletivamente com recursos do dBASE III, desenvolvido em C com SDCC/MSXgl, capaz de explorar MSX-DOS 2, Memory Mapper, armazenamento moderno e uma interface própria do MSX sem comprometer a compatibilidade, a integridade dos dados ou a viabilidade no Z80.

---

## 25. Nome provisório

Enquanto o nome definitivo não for escolhido, a documentação pode usar:

```text
MSX xBASE
```

ou:

```text
dBASE 2.5/3 para MSX
```

O segundo é descritivo, mas não deve ser tratado automaticamente como nome comercial definitivo.

---

## 26. Estado das decisões

### Decidido

- criar uma implementação nova e nativa para MSX;
- usar o dBASE II como base de compatibilidade;
- incorporar seletivamente recursos do dBASE III;
- usar C/SDCC;
- preferir MSXgl inicialmente;
- adotar arquitetura modular;
- explorar MSX-DOS 2 e Memory Mapper;
- aproveitar recursos próprios do MSX;
- manter o Z80 e seus 64 KB lógicos como restrição central.

### Ainda deve ser fechado no SPEC

- variante exata de DBF;
- nível exato de compatibilidade NDX;
- formato Memo/DBT;
- codificação de caracteres;
- número mínimo e máximo de áreas;
- requisitos mínimos definitivos;
- sintaxe completa da linguagem;
- formato do bytecode;
- política de extensões MSX;
- estratégia final de overlays;
- suporte oficial a MSX-DOS 1;
- nome definitivo do projeto.

---

*Documento preparado como consolidação técnica da conversa sobre dBASE II, dBASE III e a viabilidade de um xBase moderno para MSX. Não é uma transcrição literal; é uma reconstrução estruturada das decisões e recomendações discutidas, pronta para uso conjunto com `OUTLINE.md` e `SPEC.md`.*
