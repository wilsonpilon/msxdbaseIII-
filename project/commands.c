#include "commands.h"
#include "console.h"
#include "dbf.h"
#include "platform.h"
#include "version.h"

#define KEY_ESC        0x1B
#define PAGE_LINES     22   // linhas de dados entre pausas do LIST
#define NAME_MAX_LEN   63

// Area de trabalho unica por enquanto (SELECT/multiplas areas ficam para o Marco 7)
static Dbf s_Db;
static c8  s_DbName[NAME_MAX_LEN + 5];  // nome + ".DBF"
static bool s_Quit;

//=============================================================================
// AUXILIARES DE PARSING
//=============================================================================

static u8 Len(const c8* s)
{
	u8 n = 0;
	while (s[n])
		n++;
	return n;
}

static c8 Upper(c8 c)
{
	return ((c >= 'a') && (c <= 'z')) ? (c - ('a' - 'A')) : c;
}

// Extrai a proxima palavra de *p. Espacos e virgulas separam palavras.
// Retorna o tamanho (0 = fim da linha).
static u8 NextWord(const c8** p, const c8** word)
{
	const c8* s = *p;
	while ((*s == ' ') || (*s == ','))
		s++;
	*word = s;
	while ((*s != 0) && (*s != ' ') && (*s != ','))
		s++;
	*p = s;
	return (u8)(s - *word);
}

// Compara a palavra digitada com o nome do comando, sem diferenciar caixa.
// Como no dBASE, aceita abreviacao a partir de 4 letras (DISP, LIST, ...).
static bool WordIs(const c8* word, u8 wlen, const c8* name)
{
	u8 nlen = Len(name);
	if ((wlen > nlen) || ((wlen < nlen) && (wlen < 4)))
		return FALSE;
	for (u8 i = 0; i < wlen; i++)
		if (Upper(word[i]) != name[i])
			return FALSE;
	return TRUE;
}

// Converte um inteiro com sinal opcional. Retorna FALSE se nao for numero.
static bool ParseInt(const c8* word, u8 wlen, i32* out)
{
	bool neg = FALSE;
	u8 i = 0;
	if ((wlen > 0) && ((word[0] == '-') || (word[0] == '+')))
	{
		neg = (word[0] == '-');
		i++;
	}
	if (i >= wlen)
		return FALSE;
	i32 v = 0;
	for (; i < wlen; i++)
	{
		if ((word[i] < '0') || (word[i] > '9'))
			return FALSE;
		v = v * 10 + (word[i] - '0');
	}
	*out = neg ? -v : v;
	return TRUE;
}

//=============================================================================
// AUXILIARES DE SAIDA
//=============================================================================

static void PrintSpaces(u8 n)
{
	while (n--)
		Console_PutChar(' ');
}

// Escreve value com pelo menos width digitos (zeros a esquerda)
static void PrintNum2(u8 value)
{
	Console_PutChar('0' + value / 10);
	Console_PutChar('0' + value % 10);
}

static void PrintError(u8 err)
{
	switch (err)
	{
	case DBF_ERR_OPEN:   Console_Print("*** Arquivo nao encontrado\n"); break;
	case DBF_ERR_READ:   Console_Print("*** Erro de leitura\n"); break;
	case DBF_ERR_FORMAT: Console_Print("*** Formato DBF nao suportado pelo leitor atual\n"); break;
	case DBF_ERR_LIMITS: Console_Print("*** DBF excede os limites (campos ou tamanho do registro)\n"); break;
	case DBF_ERR_RANGE:  Console_Print("*** Registro fora do arquivo\n"); break;
	case DBF_ERR_WRITE:  Console_Print("*** Falha de escrita ou formato sem suporte para mutacao\n"); break;
	}
}

static bool RequireDb(void)
{
	if (Dbf_IsOpen(&s_Db))
		return TRUE;
	Console_Print("*** Nenhum banco em USE\n");
	return FALSE;
}

//=============================================================================
// EXIBICAO DE CAMPOS E REGISTROS
//=============================================================================

// Largura exibida do campo
static u8 DisplayWidth(const DbfField* f)
{
	switch (f->Type)
	{
	case 'L': return 3;   // .T.
	case 'D': return 8;   // MM/DD/AA
	case 'M': return 4;   // memo
	default:  return f->Length;
	}
}

// Largura da coluna: cabe o dado e o nome do campo
static u8 ColumnWidth(const DbfField* f)
{
	u8 w = DisplayWidth(f);
	u8 n = Len(f->Name);
	return (n > w) ? n : w;
}

// Escreve o valor do campo i e retorna quantos caracteres usou
static u8 PrintFieldValue(u8 i)
{
	const DbfField* f = &s_Db.Fields[i];
	const u8* d = Dbf_FieldData(&s_Db, i);

	switch (f->Type)
	{
	case 'L':
		Console_PutChar('.');
		Console_PutChar(((d[0] == 'T') || (d[0] == 't') || (d[0] == 'Y') || (d[0] == 'y')) ? 'T'
			: ((d[0] == 'F') || (d[0] == 'f') || (d[0] == 'N') || (d[0] == 'n')) ? 'F' : '?');
		Console_PutChar('.');
		return 3;
	case 'D':
		if (f->Length != 8)
			break;
		// AAAAMMDD -> MM/DD/AA
		Console_PrintN(d + 4, 2);
		Console_PutChar('/');
		Console_PrintN(d + 6, 2);
		Console_PutChar('/');
		Console_PrintN(d + 2, 2);
		return 8;
	case 'M':
		Console_Print("memo");
		return 4;
	}
	Console_PrintN(d, f->Length);
	return f->Length;
}

// Cabecalho das colunas (como o LIST do dBASE)
static void PrintHeader(void)
{
	Console_Print("Record#  ");
	for (u8 i = 0; i < s_Db.FieldCount; i++)
	{
		const DbfField* f = &s_Db.Fields[i];
		Console_Print(f->Name);
		PrintSpaces(ColumnWidth(f) - Len(f->Name) + 1);
	}
	Console_NewLine();
}

// Uma linha: numero do registro, marca de exclusao e campos
static void PrintRecord(void)
{
	// Numero alinhado a direita em 6 colunas
	u32 n = s_Db.RecNo;
	u8 digits = (n >= 100000) ? 6 : (n >= 10000) ? 5 : (n >= 1000) ? 4 : (n >= 100) ? 3 : (n >= 10) ? 2 : 1;
	PrintSpaces(6 - digits);
	Console_PrintNum(n);
	Console_PutChar(' ');
	Console_PutChar(Dbf_IsDeleted(&s_Db) ? '*' : ' ');
	Console_PutChar(' ');

	for (u8 i = 0; i < s_Db.FieldCount; i++)
	{
		u8 used = PrintFieldValue(i);
		PrintSpaces(ColumnWidth(&s_Db.Fields[i]) - used + 1);
	}
	Console_NewLine();
}

// Largura total de uma linha (para estimar quantas linhas de tela ocupa)
static u16 RowWidth(void)
{
	u16 w = 9;
	for (u8 i = 0; i < s_Db.FieldCount; i++)
		w += ColumnWidth(&s_Db.Fields[i]) + 1;
	return w;
}

static void PrintStructure(void)
{
	Console_Print("Structure for database : ");
	Console_Print(s_DbName);
	Console_Print("\nNumber of data records : ");
	Console_PrintNum(s_Db.RecCount);
	Console_Print("\nDate of last update    : ");
	PrintNum2(s_Db.LastUpdate[1]);
	Console_PutChar('/');
	PrintNum2(s_Db.LastUpdate[2]);
	Console_PutChar('/');
	PrintNum2(s_Db.LastUpdate[0] % 100);
	Console_Print("\nField  Field Name  Type       Width    Dec\n");

	for (u8 i = 0; i < s_Db.FieldCount; i++)
	{
		const DbfField* f = &s_Db.Fields[i];
		PrintSpaces((i + 1 < 10) ? 4 : 3);
		Console_PrintNum(i + 1);
		Console_Print("  ");
		Console_Print(f->Name);
		PrintSpaces(12 - Len(f->Name));
		switch (f->Type)
		{
		case 'C': Console_Print("Character"); break;
		case 'N': Console_Print("Numeric  "); break;
		case 'L': Console_Print("Logical  "); break;
		case 'D': Console_Print("Date     "); break;
		case 'M': Console_Print("Memo     "); break;
		default:  Console_Print("?        "); break;
		}
		PrintSpaces(f->Length < 10 ? 5 : 4);
		Console_PrintNum(f->Length);
		if (f->Type == 'N')
		{
			PrintSpaces(f->Decimals < 10 ? 5 : 4);
			Console_PrintNum(f->Decimals);
		}
		Console_NewLine();
	}
	Console_Print("** Total **");
	PrintSpaces(21);
	Console_PrintNum((u32)s_Db.RecLen);
	Console_NewLine();
}

//=============================================================================
// COMANDOS
//=============================================================================

static void Cmd_Help(const c8* args);

static void Cmd_Quit(const c8* args)
{
	(void)args;
	s_Quit = TRUE;
}

static void Cmd_Clear(const c8* args)
{
	(void)args;
	Console_Clear();
}

// Cor no estilo dBASE: letras N B G R W (combinaveis: BG, RB, GR) com '+' para brilhante.
// Converte para a paleta MSX. Retorna FALSE se nao reconhecer.
static bool ParseDbaseColor(const c8* s, u8 len, u8* out)
{
	bool bright = FALSE;
	bool n = FALSE, w = FALSE, r = FALSE, g = FALSE, b = FALSE;

	for (u8 i = 0; i < len; i++)
	{
		switch (Upper(s[i]))
		{
		case '+': bright = TRUE; break;
		case 'N': n = TRUE; break;
		case 'W': w = TRUE; break;
		case 'R': r = TRUE; break;
		case 'G': g = TRUE; break;
		case 'B': b = TRUE; break;
		default:  return FALSE;
		}
	}

	if (w)            *out = bright ? 15 : 14;   // branco / cinza
	else if (n)       *out = bright ? 14 : 1;    // preto / cinza
	else if (g && b)  *out = 7;                  // ciano
	else if (r && b)  *out = bright ? 9 : 13;    // magenta
	else if (r && g)  *out = bright ? 11 : 10;   // amarelo
	else if (r)       *out = bright ? 8 : 6;     // vermelho
	else if (g)       *out = bright ? 3 : 12;    // verde
	else if (b)       *out = bright ? 5 : 4;     // azul
	else              return FALSE;
	return TRUE;
}

// Numero da paleta MSX (0 a 15)
static bool ParseMsxColor(const c8* s, u8 len, u8* out)
{
	i32 v;
	if (!ParseInt(s, len, &v) || (v < 0) || (v > 15))
		return FALSE;
	*out = (u8)v;
	return TRUE;
}

// SET COLOR TO W+/B       (estilo dBASE: texto/fundo)
// SET COLOR TO 15,4       (extensao MSX: numeros da paleta 0-15)
// SET COLOR TO            (volta ao padrao W+/N)
static void Cmd_Color(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);
	u8 fg = 15, bg = 1;

	if ((wlen > 0) && WordIs(word, wlen, "TO"))
		wlen = NextWord(&args, &word);

	if (wlen > 0)
	{
		bool ok;
		if ((word[0] >= '0') && (word[0] <= '9'))
		{
			// Dois numeros: texto e fundo
			ok = ParseMsxColor(word, wlen, &fg);
			wlen = NextWord(&args, &word);
			ok = ok && (wlen > 0) && ParseMsxColor(word, wlen, &bg);
		}
		else
		{
			// Uma palavra "texto/fundo"
			u8 slash = 0;
			while ((slash < wlen) && (word[slash] != '/'))
				slash++;
			ok = (slash < wlen)
				&& ParseDbaseColor(word, slash, &fg)
				&& ParseDbaseColor(word + slash + 1, wlen - slash - 1, &bg);
		}
		if (!ok)
		{
			Console_Print("*** Uso: SET COLOR TO W+/B  ou  SET COLOR TO 15,4\n");
			return;
		}
	}
	Console_SetColor(fg, bg);
}

static void Cmd_Set(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);

	if ((wlen > 0) && WordIs(word, wlen, "COLOR"))
		Cmd_Color(args);
	else
		Console_Print("*** SET: opcao desconhecida (COLOR)\n");
}

static void Cmd_Ver(const c8* args)
{
	(void)args;
	Console_Print("MSX dBASE ");
	Console_PrintNum(APP_VERSION_MAJOR);
	Console_PutChar('.');
	Console_PrintNum(APP_VERSION_MINOR);
	Console_Print(" (" APP_RELEASE_CODENAME ")\n");
	Console_Print("MSX-DOS ");
	Console_PrintNum(g_Platform.DosKernel == 0 ? 1 : g_Platform.DosKernel);
	Console_PutChar('.');
	Console_PrintNum(g_Platform.DosKernelMinor);
	Console_NewLine();
	if (g_Platform.HasMapper)
	{
		Console_Print("Memory Mapper: ");
		Console_PrintNum(g_Platform.MapperKB);
		Console_Print(" KB (");
		Console_PrintNum(g_Platform.MapperFreeKB);
		Console_Print(" KB livres)\n");
	}
	else
		Console_Print("Memory Mapper: nao encontrado\n");
}

// USE [arquivo]  -  sem argumento fecha o banco
static void Cmd_Use(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);

	if (wlen == 0)
	{
		Dbf_Close(&s_Db);
		return;
	}
	if (wlen > NAME_MAX_LEN)
	{
		Console_Print("*** Nome muito longo\n");
		return;
	}

	// Copia o nome em maiusculas e acrescenta .DBF se nao houver extensao
	bool hasExt = FALSE;
	for (u8 i = 0; i < wlen; i++)
	{
		s_DbName[i] = Upper(word[i]);
		if (word[i] == '.')
			hasExt = TRUE;
	}
	s_DbName[wlen] = 0;
	if (!hasExt)
	{
		s_DbName[wlen++] = '.';
		s_DbName[wlen++] = 'D';
		s_DbName[wlen++] = 'B';
		s_DbName[wlen++] = 'F';
		s_DbName[wlen] = 0;
	}

	u8 err = Dbf_Open(&s_Db, s_DbName);
	if (err != DBF_OK)
	{
		PrintError(err);
		return;
	}
	Console_PrintNum(s_Db.RecCount);
	Console_Print(" registro(s)\n");
}

// LIST [STRUCTURE]  -  todos os registros, com pausa a cada tela
static void ListAll(void)
{
	u16 perRecord = (RowWidth() + CON_COLS - 1) / CON_COLS;
	u16 lines = 0;

	PrintHeader();
	lines += 1;
	for (u32 n = 1; n <= s_Db.RecCount; n++)
	{
		if (Dbf_Goto(&s_Db, n) != DBF_OK)
		{
			Console_Print("*** Erro de leitura\n");
			return;
		}
		if (lines + perRecord > PAGE_LINES)
		{
			Console_Print("-- tecla para continuar, ESC para parar --");
			c8 key = Console_ReadKey();
			Console_NewLine();
			if (key == KEY_ESC)
				return;
			lines = 0;
		}
		PrintRecord();
		lines += perRecord;
	}
}

// Procura STRUCTURE na lista de argumentos
static bool WantsStructure(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);
	return (wlen > 0) && WordIs(word, wlen, "STRUCTURE");
}

static void Cmd_List(const c8* args)
{
	if (!RequireDb())
		return;
	if (WantsStructure(args))
		PrintStructure();
	else if (s_Db.RecCount == 0)
		Console_Print("*** Banco vazio\n");
	else
	{
		ListAll();
		// LIST deixa o ponteiro no fim, como no dBASE
		s_Db.RecNo = s_Db.RecCount + 1;
		s_Db.Eof = TRUE;
	}
}

// DISPLAY [STRUCTURE | ALL]  -  sem argumento mostra o registro atual
static void Cmd_Display(const c8* args)
{
	if (!RequireDb())
		return;

	const c8* word;
	const c8* p = args;
	u8 wlen = NextWord(&p, &word);
	if ((wlen > 0) && WordIs(word, wlen, "STRUCTURE"))
	{
		PrintStructure();
		return;
	}
	if ((wlen > 0) && WordIs(word, wlen, "ALL"))
	{
		Cmd_List("");
		return;
	}
	if (s_Db.Eof || s_Db.Bof)
	{
		Console_Print("*** Fim de arquivo\n");
		return;
	}
	PrintHeader();
	PrintRecord();
}

// GO / GOTO  TOP | BOTTOM | n
static void Cmd_Go(const c8* args)
{
	if (!RequireDb())
		return;

	const c8* word;
	u8 wlen = NextWord(&args, &word);
	i32 n;
	u8 err;

	if (s_Db.RecCount == 0)
	{
		Console_Print("*** Banco vazio\n");
		return;
	}
	if ((wlen > 0) && WordIs(word, wlen, "TOP"))
		err = Dbf_Goto(&s_Db, 1);
	else if ((wlen > 0) && WordIs(word, wlen, "BOTTOM"))
		err = Dbf_Goto(&s_Db, s_Db.RecCount);
	else if (ParseInt(word, wlen, &n) && (n >= 1))
		err = Dbf_Goto(&s_Db, (u32)n);
	else
	{
		Console_Print("*** Uso: GO TOP | BOTTOM | n\n");
		return;
	}
	PrintError(err);
}

// SKIP [n]
static void Cmd_Skip(const c8* args)
{
	if (!RequireDb())
		return;

	const c8* word;
	u8 wlen = NextWord(&args, &word);
	i32 n = 1;
	if ((wlen > 0) && !ParseInt(word, wlen, &n))
	{
		Console_Print("*** Uso: SKIP [n]\n");
		return;
	}
	u8 err = Dbf_Skip(&s_Db, n);
	PrintError(err);
	if (s_Db.Eof)
		Console_Print("*** Fim de arquivo\n");
	else if (s_Db.Bof)
		Console_Print("*** Inicio de arquivo\n");
}

// APPEND BLANK - acrescenta um registro em branco
static void Cmd_Append(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);
	if ((wlen == 0) || !WordIs(word, wlen, "BLANK"))
	{
		Console_Print("*** Uso: APPEND BLANK\n");
		return;
	}
	if (!RequireDb())
		return;

	u8 err = Dbf_AppendBlank(&s_Db, s_DbName);
	if (err == DBF_OK)
		Console_Print("1 registro adicionado\n");
	else
		PrintError(err);
}

//=============================================================================
// TABELA DE COMANDOS
//=============================================================================

typedef struct Command
{
	const c8* Name;
	void (*Handler)(const c8* args);
	const c8* Help;
} Command;

static const Command s_Commands[] =
{
	{ "USE",     Cmd_Use,     "USE [arquivo]            abre/fecha um .DBF" },
	{ "LIST",    Cmd_List,    "LIST [STRUCTURE]         lista os registros" },
	{ "DISPLAY", Cmd_Display, "DISPLAY [STRUCTURE|ALL]  registro atual" },
	{ "GO",      Cmd_Go,      "GO TOP|BOTTOM|n          move o ponteiro" },
	{ "GOTO",    Cmd_Go,      0 },
	{ "SKIP",    Cmd_Skip,    "SKIP [n]                 avanca/volta n registros" },
	{ "APPEND",  Cmd_Append,  "APPEND BLANK             acrescenta registro vazio" },
	{ "CLEAR",   Cmd_Clear,   "CLEAR                    limpa a tela" },
	{ "SET",     Cmd_Set,     "SET COLOR TO W+/B|15,4   cores (texto/fundo)" },
	{ "COLOR",   Cmd_Color,   0 },
	{ "VER",     Cmd_Ver,     "VER                      versao e memoria" },
	{ "HELP",    Cmd_Help,    "HELP                     esta lista" },
	{ "QUIT",    Cmd_Quit,    "QUIT                     sai do programa" },
};

#define COMMAND_COUNT (sizeof(s_Commands) / sizeof(s_Commands[0]))

static void Cmd_Help(const c8* args)
{
	(void)args;
	for (u8 i = 0; i < COMMAND_COUNT; i++)
	{
		if (s_Commands[i].Help)
		{
			Console_Print(s_Commands[i].Help);
			Console_NewLine();
		}
	}
}

void Command_Init(void)
{
	Dbf_Init(&s_Db);
}

void Command_Done(void)
{
	Dbf_Close(&s_Db);
}

bool Command_Execute(const c8* line)
{
	const c8* word;
	u8 wlen = NextWord(&line, &word);
	if (wlen == 0)
		return TRUE;                          // linha vazia

	s_Quit = FALSE;
	for (u8 i = 0; i < COMMAND_COUNT; i++)
	{
		if (WordIs(word, wlen, s_Commands[i].Name))
		{
			s_Commands[i].Handler(line);
			return !s_Quit;
		}
	}
	Console_Print("*** Comando desconhecido\n");
	return TRUE;
}
