#include "commands.h"
#include "console.h"
#include "dbf.h"
#include "help.h"
#include "platform.h"
#include "version.h"

#define KEY_ESC        0x1B
#define PAGE_LINES     22   // linhas de dados entre pausas do LIST
#define NAME_MAX_LEN   63
#define APPEND_FORM_FIRST_ROW 3
#define APPEND_FORM_VISIBLE_ROWS (CON_ROWS - APPEND_FORM_FIRST_ROW - 2)
#define APPEND_KEY_CR   0x0D
#define APPEND_KEY_TAB  0x09
#define APPEND_KEY_UP   0x1E
#define APPEND_KEY_DOWN 0x1F
#define COMMAND_COUNT (sizeof(s_Commands) / sizeof(s_Commands[0]))

// Area de trabalho unica por enquanto (SELECT/multiplas areas ficam para o Marco 7)
static Dbf s_Db;
static Dbf s_FileListDb;
static c8  s_DbName[NAME_MAX_LEN + 5];  // nome + ".DBF"
static bool s_Quit;
static bool s_DeletedOn;
static bool s_ColonOn;
static bool s_ConfirmOn;
static bool s_MsxOn;
static bool s_HelpScreen;

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

static bool IsCurrentRecordHidden(void)
{
	return s_DeletedOn && Dbf_IsDeleted(&s_Db);
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
static const c8* HelpCommandSummary(u8 index);

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
	if (!s_MsxOn)
	{
		Console_Print("*** SET COLOR requer SET MSX ON\n");
		return;
	}
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
	else if ((wlen > 0) && WordIs(word, wlen, "DELETED"))
	{
		const c8* mode;
		u8 modeLen = NextWord(&args, &mode);
		const c8* extra;
		u8 extraLen = NextWord(&args, &extra);
		if ((extraLen > 0) || ((modeLen != 2) && (modeLen != 3)))
		{
			Console_Print("*** Uso: SET DELETED ON|OFF\n");
			return;
		}
		if ((modeLen == 2) && (Upper(mode[0]) == 'O') && (Upper(mode[1]) == 'N'))
			s_DeletedOn = TRUE;
		else if ((modeLen == 3) && (Upper(mode[0]) == 'O') && (Upper(mode[1]) == 'F') && (Upper(mode[2]) == 'F'))
			s_DeletedOn = FALSE;
		else
		{
			Console_Print("*** Uso: SET DELETED ON|OFF\n");
			return;
		}
		Console_Print(s_DeletedOn ? "SET DELETED ON\n" : "SET DELETED OFF\n");
	}
	else if ((wlen > 0) && WordIs(word, wlen, "COLON"))
	{
		const c8* mode;
		u8 modeLen = NextWord(&args, &mode);
		const c8* extra;
		u8 extraLen = NextWord(&args, &extra);
		if ((extraLen > 0) || ((modeLen != 2) && (modeLen != 3)))
		{
			Console_Print("*** Uso: SET COLON ON|OFF\n");
			return;
		}
		if ((modeLen == 2) && (Upper(mode[0]) == 'O') && (Upper(mode[1]) == 'N'))
			s_ColonOn = TRUE;
		else if ((modeLen == 3) && (Upper(mode[0]) == 'O') && (Upper(mode[1]) == 'F') && (Upper(mode[2]) == 'F'))
			s_ColonOn = FALSE;
		else
		{
			Console_Print("*** Uso: SET COLON ON|OFF\n");
			return;
		}
		Console_Print(s_ColonOn ? "SET COLON ON\n" : "SET COLON OFF\n");
	}
	else if ((wlen > 0) && WordIs(word, wlen, "CONFIRM"))
	{
		const c8* mode;
		u8 modeLen = NextWord(&args, &mode);
		const c8* extra;
		u8 extraLen = NextWord(&args, &extra);
		if ((extraLen > 0) || ((modeLen != 2) && (modeLen != 3)))
		{
			Console_Print("*** Uso: SET CONFIRM ON|OFF\n");
			return;
		}
		if ((modeLen == 2) && (Upper(mode[0]) == 'O') && (Upper(mode[1]) == 'N'))
			s_ConfirmOn = TRUE;
		else if ((modeLen == 3) && (Upper(mode[0]) == 'O')
			&& (Upper(mode[1]) == 'F') && (Upper(mode[2]) == 'F'))
			s_ConfirmOn = FALSE;
		else
		{
			Console_Print("*** Uso: SET CONFIRM ON|OFF\n");
			return;
		}
		Console_Print(s_ConfirmOn ? "SET CONFIRM ON\n" : "SET CONFIRM OFF\n");
	}
	else if ((wlen > 0) && WordIs(word, wlen, "HELP"))
	{
		const c8* mode;
		u8 modeLen = NextWord(&args, &mode);
		const c8* extra;
		if ((NextWord(&args, &extra) > 0)
			|| ((modeLen != 3) && (modeLen != 6)))
		{
			Console_Print("*** Uso: SET HELP TXT|SCREEN\n");
			return;
		}
		if ((modeLen == 3) && WordIs(mode, modeLen, "TXT"))
			s_HelpScreen = FALSE;
		else if ((modeLen == 6) && WordIs(mode, modeLen, "SCREEN"))
			s_HelpScreen = TRUE;
		else
		{
			Console_Print("*** Uso: SET HELP TXT|SCREEN\n");
			return;
		}
		Console_Print(s_HelpScreen ? "SET HELP SCREEN\n" : "SET HELP TXT\n");
	}
	else if ((wlen > 0) && WordIs(word, wlen, "MSX"))
	{
		const c8* mode;
		u8 modeLen = NextWord(&args, &mode);
		const c8* extra;
		if ((NextWord(&args, &extra) > 0)
			|| ((modeLen != 2) && (modeLen != 3)))
		{
			Console_Print("*** Uso: SET MSX ON|OFF\n");
			return;
		}
		if ((modeLen == 2) && WordIs(mode, modeLen, "ON"))
			s_MsxOn = TRUE;
		else if ((modeLen == 3) && WordIs(mode, modeLen, "OFF"))
			s_MsxOn = FALSE;
		else
		{
			Console_Print("*** Uso: SET MSX ON|OFF\n");
			return;
		}
		Console_SetColor(15, s_MsxOn ? 1 : 0);
		Console_Print(s_MsxOn ? "SET MSX ON\n" : "SET MSX OFF\n");
	}
	else
		Console_Print("*** SET: opcao desconhecida (COLOR, DELETED, COLON, CONFIRM, HELP, MSX)\n");
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

// LIST [STRUCTURE] - todos os registros, com pausa a cada tela
static void ListAll(void)
{
	u16 perRecord = (RowWidth() + CON_COLS - 1) / CON_COLS;
	u16 lines = 0;
	u32 visibleCount = 0;

	PrintHeader();
	lines += 1;
	for (u32 n = 1; n <= s_Db.RecCount; n++)
	{
		if (Dbf_Goto(&s_Db, n) != DBF_OK)
		{
			Console_Print("*** Erro de leitura\n");
			return;
		}
		if (IsCurrentRecordHidden())
			continue;
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
		visibleCount++;
	}
	if (visibleCount == 0)
		Console_Print("*** Nenhum registro visivel\n");
}

// Procura STRUCTURE na lista de argumentos
static bool WantsStructure(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);
	return (wlen > 0) && WordIs(word, wlen, "STRUCTURE");
}

static void PrintDate(u8 day, u8 month, u16 year)
{
	PrintNum2(day);
	Console_PutChar('/');
	PrintNum2(month);
	Console_PutChar('/');
	Console_PrintNum(year);
}

static void PrintFileName83(const c8* name)
{
	u8 i = 0;
	u8 baseLength = 0;
	while (name[i] && (name[i] != '.') && (baseLength < 10))
	{
		Console_PutChar(name[i++]);
		baseLength++;
	}
	PrintSpaces((baseLength < 10) ? 10 - baseLength : 1);
	if (name[i] == '.')
	{
		i++;
		while (name[i])
			Console_PutChar(name[i++]);
	}
}

static bool CopyFilePattern(const c8* source, u8 sourceLen, c8* pattern)
{
	if ((sourceLen == 0) || (sourceLen >= 64))
		return FALSE;
	for (u8 i = 0; i < sourceLen; i++)
		pattern[i] = source[i];
	pattern[sourceLen] = 0;
	return TRUE;
}

static void DisplayDbfFiles(void)
{
	PlatformFileEntry entry;
	if (!PFile_FindFirst("*.DBF", &entry))
	{
		Console_Print("*** Nenhum arquivo .DBF no diretorio atual\n");
		return;
	}
	Console_Print("Arquivo de dados      # Registros         Ultima data\n");
	do
	{
		PrintFileName83(entry.Name);
		PrintSpaces(9);
		Dbf_Init(&s_FileListDb);
		if (Dbf_Open(&s_FileListDb, entry.Name) == DBF_OK)
		{
			u32 count = s_FileListDb.RecCount;
			for (u32 divisor = 100000; divisor; divisor /= 10)
				Console_PutChar('0' + (u8)((count / divisor) % 10));
			PrintSpaces(15);
			PrintDate(s_FileListDb.LastUpdate[2], s_FileListDb.LastUpdate[1],
				1900 + s_FileListDb.LastUpdate[0]);
			Dbf_Close(&s_FileListDb);
		}
		else
		{
			Console_Print("------");
			PrintSpaces(15);
			PrintDate(entry.Day, entry.Month, entry.Year);
		}
		Console_NewLine();
	} while (PFile_FindNext(&entry));
}

static void DisplayMatchingFiles(const c8* source, u8 sourceLen)
{
	c8 pattern[64];
	PlatformFileEntry entry;
	if (!CopyFilePattern(source, sourceLen, pattern))
	{
		Console_Print("*** Padrao de arquivo muito longo\n");
		return;
	}
	if (!PFile_FindFirst(pattern, &entry))
	{
		Console_Print("*** Nenhum arquivo correspondente\n");
		return;
	}
	Console_Print("Arquivo                  Tamanho       Data de modificacao\n");
	do
	{
		Console_Print(entry.Name);
		u8 nameLen = Len(entry.Name);
		PrintSpaces((nameLen < 25) ? 25 - nameLen : 1);
		Console_PrintNum(entry.Size);
		PrintSpaces(10);
		PrintDate(entry.Day, entry.Month, entry.Year);
		Console_NewLine();
	} while (PFile_FindNext(&entry));
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
	const c8* word;
	const c8* p = args;
	u8 wlen = NextWord(&p, &word);
	if ((wlen > 0) && WordIs(word, wlen, "FILES"))
	{
		const c8* option;
		u8 optionLen = NextWord(&p, &option);
		if (optionLen == 0)
		{
			DisplayDbfFiles();
			return;
		}
		if (!WordIs(option, optionLen, "LIKE"))
		{
			Console_Print("*** Uso: DISPLAY FILES [LIKE padrao]\n");
			return;
		}
		const c8* pattern;
		u8 patternLen = NextWord(&p, &pattern);
		const c8* extra;
		if ((patternLen == 0) || (NextWord(&p, &extra) > 0))
		{
			Console_Print("*** Uso: DISPLAY FILES [LIKE padrao]\n");
			return;
		}
		DisplayMatchingFiles(pattern, patternLen);
		return;
	}
	if (!RequireDb())
		return;
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
	if (IsCurrentRecordHidden())
	{
		Console_Print("*** Registro excluido oculto por SET DELETED ON\n");
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

static bool StoreAppendValue(const DbfField* field, const c8* value, u8 valueLen, u8* record)
{
	if (valueLen > field->Length)
		return FALSE;
	u8* target = record + field->Offset;
	for (u8 i = 0; i < field->Length; i++)
		target[i] = ' ';
	if (valueLen == 0)
		return TRUE;
	if (field->Type == 'C')
	{
		for (u8 i = 0; i < valueLen; i++)
			target[i] = value[i];
		return TRUE;
	}
	if (field->Type == 'L')
	{
		if (valueLen != 1)
			return FALSE;
		c8 logical = Upper(value[0]);
		if ((logical != 'T') && (logical != 'Y') && (logical != 'F') && (logical != 'N'))
			return FALSE;
		target[0] = ((logical == 'T') || (logical == 'Y')) ? 'T' : 'F';
		return TRUE;
	}
	if (field->Type != 'N')
		return FALSE;

	bool hasDigit = FALSE;
	bool hasDecimal = FALSE;
	u8 decimals = 0;
	for (u8 i = 0; i < valueLen; i++)
	{
		c8 chr = value[i];
		if ((chr >= '0') && (chr <= '9'))
		{
			hasDigit = TRUE;
			if (hasDecimal)
				decimals++;
		}
		else if ((chr == '-') && (i == 0))
			continue;
		else if ((chr == '.') && !hasDecimal)
			hasDecimal = TRUE;
		else
			return FALSE;
	}
	if (!hasDigit || (decimals > field->Decimals))
		return FALSE;
	for (u8 i = 0; i < valueLen; i++)
		target[field->Length - valueLen + i] = value[i];
	return TRUE;
}

static u8 AppendInputStartX(const DbfField* field)
{
	u8 digits = 1;
	u8 size = field->Length;
	while (size >= 10)
	{
		size /= 10;
		digits++;
	}
	return Len(field->Name) + digits + 3;
}

static u8 AppendInputWidth(const DbfField* field)
{
	u8 prefixWidth = AppendInputStartX(field);
	u8 suffixWidth = s_ColonOn ? 1 : 0;
	u8 available = CON_COLS - prefixWidth - suffixWidth;
	return (field->Length < available) ? field->Length : available;
}

static void DrawAppendForm(const c8* commandName, const c8* values,
	const u16* valueOffsets, const u8* valueLengths, u8 firstVisible,
	u8 currentField, const c8* status)
{
	Console_Clear();
	Console_SetPos(0, 0);
	Console_Print(commandName);
	Console_Print(" | Enter: proximo/salvar  Tab/Down: proximo  Up: anterior");
	Console_SetPos(0, 1);
	Console_Print("Left/Right edita  Backspace/Delete apaga  ESC cancela");
	Console_SetPos(0, 2);
	Console_Print("Campo ");
	Console_PrintNum(currentField + 1);
	Console_Print(" de ");
	Console_PrintNum(s_Db.FieldCount);

	for (u8 fieldIndex = firstVisible;
		(fieldIndex < s_Db.FieldCount) && (fieldIndex < firstVisible + APPEND_FORM_VISIBLE_ROWS);
		fieldIndex++)
	{
		const DbfField* field = &s_Db.Fields[fieldIndex];
		u8 row = APPEND_FORM_FIRST_ROW + fieldIndex - firstVisible;
		Console_SetPos(0, row);
		Console_Print(field->Name);
		Console_PutChar('[');
		Console_PrintNum(field->Length);
		Console_PutChar(']');
		Console_PutChar(s_ColonOn ? ':' : ' ');

		u8 width = AppendInputWidth(field);
		const c8* value = values + valueOffsets[fieldIndex];
		for (u8 i = 0; i < width; i++)
			Console_PutChar((i < valueLengths[fieldIndex]) ? value[i] : ' ');
		if (s_ColonOn)
			Console_PutChar(':');
	}

	Console_SetPos(0, CON_ROWS - 1);
	Console_Print(status);
}

static bool ReadInteractiveRecord(u8* record, const c8* commandName, bool* advanceRecord)
{
	*advanceRecord = FALSE;
	if (s_Db.FieldCount == 0)
	{
		Console_Print("*** Estrutura sem campos para inclusao\n");
		return FALSE;
	}
	for (u8 i = 0; i < s_Db.FieldCount; i++)
	{
		c8 type = s_Db.Fields[i].Type;
		if ((type != 'C') && (type != 'N') && (type != 'L'))
		{
			Console_Print("*** Tipo de campo nao suportado na inclusao\n");
			return FALSE;
		}
	}

	c8 values[DBF_MAX_RECLEN + DBF_MAX_FIELDS];
	u16 valueOffsets[DBF_MAX_FIELDS];
	u8 valueLengths[DBF_MAX_FIELDS];
	// Um byte terminador por campo evita que a edição sobrescreva o campo seguinte.
	for (u16 i = 0; i < s_Db.RecLen; i++)
		record[i] = ' ';
	record[0] = ' ';
	for (u16 i = 0; i < DBF_MAX_RECLEN + DBF_MAX_FIELDS; i++)
		values[i] = 0;
	u16 valueOffset = 0;
	for (u8 i = 0; i < DBF_MAX_FIELDS; i++)
	{
		valueOffsets[i] = valueOffset;
		valueLengths[i] = 0;
		if (i < s_Db.FieldCount)
			valueOffset += s_Db.Fields[i].Length + 1;
	}

	const c8* status = "Enter: proximo/salvar  Tab/Down: proximo  Up: anterior";
	for (u8 fieldIndex = 0, firstVisible = 0;;)
	{
		if (fieldIndex < firstVisible)
			firstVisible = fieldIndex;
		else if (fieldIndex >= firstVisible + APPEND_FORM_VISIBLE_ROWS)
			firstVisible = fieldIndex - APPEND_FORM_VISIBLE_ROWS + 1;

		const DbfField* field = &s_Db.Fields[fieldIndex];
		DrawAppendForm(commandName, values, valueOffsets, valueLengths,
			firstVisible, fieldIndex, status);
		u8 width = AppendInputWidth(field);
		u8 row = APPEND_FORM_FIRST_ROW + fieldIndex - firstVisible;
		u8 inputX = AppendInputStartX(field);
		c8* value = values + valueOffsets[fieldIndex];
		u8 key = Console_EditField(value, &valueLengths[fieldIndex],
			field->Length, width, inputX, row, !s_ConfirmOn);
		if (key == KEY_ESC)
		{
			Console_Clear();
			Console_Print("Operacao cancelada\n");
			return FALSE;
		}
		if (!StoreAppendValue(field, value,
			valueLengths[fieldIndex], record))
		{
			status = "*** Valor invalido para o campo; tente novamente";
			continue;
		}
		status = "Enter: proximo/salvar  Tab/Down: proximo  Up: anterior";
		if (key == CONSOLE_EDIT_FIELD_FULL)
		{
			if (fieldIndex + 1 == s_Db.FieldCount)
			{
				*advanceRecord = TRUE;
				break;
			}
			fieldIndex++;
			continue;
		}

		if ((key == APPEND_KEY_CR) && (fieldIndex + 1 == s_Db.FieldCount))
			break;
		if ((key == APPEND_KEY_CR) || (key == APPEND_KEY_TAB) || (key == APPEND_KEY_DOWN))
			fieldIndex = (fieldIndex + 1) % s_Db.FieldCount;
		else if (key == APPEND_KEY_UP)
			fieldIndex = (fieldIndex == 0) ? s_Db.FieldCount - 1 : fieldIndex - 1;
	}
	return TRUE;
}


static void Cmd_AppendInteractive(void)
{
	if (!RequireDb())
		return;
	if (s_Db.Version != 0x02)
	{
		PrintError(DBF_ERR_WRITE);
		return;
	}
	for (;;)
	{
		u8 record[DBF_MAX_RECLEN];
		bool advanceRecord;
		for (u16 i = 0; i < s_Db.RecLen; i++)
			record[i] = ' ';
		record[0] = ' ';
		if (!ReadInteractiveRecord(record, "APPEND", &advanceRecord))
			return;

		u8 err = Dbf_AppendRecord(&s_Db, s_DbName, record);
		Console_Clear();
		if (err != DBF_OK)
		{
			PrintError(err);
			return;
		}
		if (!advanceRecord)
		{
			Console_Print("1 registro adicionado\n");
			return;
		}
	}
}

static void Cmd_Insert(const c8* args)
{
	const c8* p = args;
	const c8* word;
	bool before = FALSE;
	bool blank = FALSE;
	u8 wordLen = NextWord(&p, &word);
	if ((wordLen > 0) && WordIs(word, wordLen, "BEFORE"))
	{
		before = TRUE;
		wordLen = NextWord(&p, &word);
	}
	if ((wordLen > 0) && WordIs(word, wordLen, "BLANK"))
	{
		blank = TRUE;
		wordLen = NextWord(&p, &word);
	}
	if (wordLen > 0)
	{
		Console_Print("*** Uso: INSERT [BEFORE] [BLANK]\n");
		return;
	}
	if (!RequireDb())
		return;
	if (s_Db.Version != 0x02)
	{
		PrintError(DBF_ERR_WRITE);
		return;
	}

	u32 position;
	if (s_Db.Bof || (s_Db.RecNo < 1))
		position = 1;
	else if (s_Db.Eof || (s_Db.RecNo > s_Db.RecCount))
		position = s_Db.RecCount + 1;
	else
		position = before ? s_Db.RecNo : s_Db.RecNo + 1;

	for (;;)
	{
		u8 record[DBF_MAX_RECLEN];
		bool advanceRecord = FALSE;
		for (u16 i = 0; i < s_Db.RecLen; i++)
			record[i] = ' ';
		record[0] = ' ';
		if (!blank && !ReadInteractiveRecord(record, "INSERT", &advanceRecord))
			return;

		u8 err = Dbf_InsertRecord(&s_Db, s_DbName, position, record);
		Console_Clear();
		if (err != DBF_OK)
		{
			PrintError(err);
			return;
		}
		if (!advanceRecord)
		{
			Console_Print("1 registro inserido\n");
			return;
		}
		position = s_Db.RecNo + 1;
	}
}

// APPEND [BLANK] - formulario interativo ou registro em branco
static void Cmd_Append(const c8* args)
{
	const c8* word;
	u8 wlen = NextWord(&args, &word);
	const c8* extra;
	if (wlen == 0)
	{
		Cmd_AppendInteractive();
		return;
	}
	if (!WordIs(word, wlen, "BLANK") || (NextWord(&args, &extra) > 0))
	{
		Console_Print("*** Uso: APPEND [BLANK]\n");
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

static i16 FindField(const c8* name, u8 nameLen)
{
	for (u8 i = 0; i < s_Db.FieldCount; i++)
	{
		const c8* fieldName = s_Db.Fields[i].Name;
		u8 fieldLen = Len(fieldName);
		if (fieldLen != nameLen)
			continue;
		u8 j = 0;
		while ((j < nameLen) && (Upper(name[j]) == fieldName[j]))
			j++;
		if (j == nameLen)
			return i;
	}
	return -1;
}

typedef struct DecimalValue
{
	i32 Mantissa;
	u8 Scale;
} DecimalValue;

typedef struct ExpressionParser
{
	const c8* Source;
	u8 Depth;
} ExpressionParser;

#define EXPRESSION_MAX_SCALE 6
#define DECIMAL_MAX_VALUE 0x7FFFFFFFL
#define REPLACE_TEXT_MAX 80

static c8 s_ReplaceExpression[REPLACE_TEXT_MAX];
static c8 s_ReplaceCondition[REPLACE_TEXT_MAX];

static u32 DecimalAbs(i32 value)
{
	return (value < 0) ? (u32)(-value) : (u32)value;
}

static bool ScaleDecimal(DecimalValue* value, u8 scale)
{
	while (value->Scale < scale)
	{
		if ((value->Mantissa > DECIMAL_MAX_VALUE / 10)
			|| (value->Mantissa < -DECIMAL_MAX_VALUE / 10))
			return FALSE;
		value->Mantissa *= 10;
		value->Scale++;
	}
	return TRUE;
}

static void SkipExpressionSpaces(ExpressionParser* parser)
{
	while ((*parser->Source == ' ') || (*parser->Source == '\t'))
		parser->Source++;
}

static bool ParseDecimalText(const c8* text, u8 length, DecimalValue* value)
{
	u8 index = 0;
	bool negative = FALSE;
	bool decimalPoint = FALSE;
	bool hasDigit = FALSE;
	u32 mantissa = 0;
	u8 scale = 0;
	if ((length > 0) && ((text[0] == '-') || (text[0] == '+')))
	{
		negative = text[0] == '-';
		index++;
	}
	for (; index < length; index++)
	{
		c8 chr = text[index];
		if ((chr >= '0') && (chr <= '9'))
		{
			u8 digit = chr - '0';
			if (mantissa > (0x7FFFFFFFUL - digit) / 10)
				return FALSE;
			mantissa = mantissa * 10 + digit;
			hasDigit = TRUE;
			if (decimalPoint)
			{
				if (scale >= EXPRESSION_MAX_SCALE)
					return FALSE;
				scale++;
			}
		}
		else if ((chr == '.') && !decimalPoint)
			decimalPoint = TRUE;
		else
			return FALSE;
	}
	if (!hasDigit)
		return FALSE;
	value->Mantissa = negative ? -(i32)mantissa : (i32)mantissa;
	value->Scale = scale;
	return TRUE;
}

static bool ReadNumericField(u8 fieldIndex, DecimalValue* value)
{
	const DbfField* field = &s_Db.Fields[fieldIndex];
	if (field->Type != 'N')
		return FALSE;
	const u8* data = Dbf_FieldData(&s_Db, fieldIndex);
	u8 first = 0;
	u8 last = field->Length;
	while ((first < last) && (data[first] == ' '))
		first++;
	while ((last > first) && (data[last - 1] == ' '))
		last--;
	if (first == last)
		return FALSE;
	return ParseDecimalText((const c8*)data + first, last - first, value);
}

static bool ParseExpression(ExpressionParser* parser, DecimalValue* value);

static bool ParseExpressionFactor(ExpressionParser* parser, DecimalValue* value)
{
	SkipExpressionSpaces(parser);
	bool negative = FALSE;
	if ((*parser->Source == '+') || (*parser->Source == '-'))
	{
		negative = *parser->Source == '-';
		parser->Source++;
		SkipExpressionSpaces(parser);
	}

	if (*parser->Source == '(')
	{
		if (parser->Depth >= 8)
			return FALSE;
		parser->Source++;
		parser->Depth++;
		bool parsed = ParseExpression(parser, value);
		parser->Depth--;
		if (!parsed)
			return FALSE;
		SkipExpressionSpaces(parser);
		if (*parser->Source != ')')
			return FALSE;
		parser->Source++;
	}
	else if (((*parser->Source >= '0') && (*parser->Source <= '9')) || (*parser->Source == '.'))
	{
		const c8* start = parser->Source;
		u8 length = 0;
		while (((parser->Source[length] >= '0') && (parser->Source[length] <= '9'))
			|| (parser->Source[length] == '.'))
			length++;
		parser->Source += length;
		if (!ParseDecimalText(start, length, value))
			return FALSE;
	}
	else
	{
		c8 fieldName[DBF_NAME_LEN];
		u8 nameLength = 0;
		while (((parser->Source[nameLength] >= 'A') && (parser->Source[nameLength] <= 'Z'))
			|| ((parser->Source[nameLength] >= 'a') && (parser->Source[nameLength] <= 'z'))
			|| ((parser->Source[nameLength] >= '0') && (parser->Source[nameLength] <= '9'))
			|| (parser->Source[nameLength] == '_'))
		{
			if (nameLength >= DBF_NAME_LEN - 1)
				return FALSE;
			fieldName[nameLength] = parser->Source[nameLength];
			nameLength++;
		}
		if (!nameLength)
			return FALSE;
		fieldName[nameLength] = 0;
		parser->Source += nameLength;
		i16 fieldIndex = FindField(fieldName, nameLength);
		if ((fieldIndex < 0) || !ReadNumericField((u8)fieldIndex, value))
			return FALSE;
	}

	if (negative)
		value->Mantissa = -value->Mantissa;
	return TRUE;
}

static bool AddDecimals(DecimalValue* left, DecimalValue right, bool subtract)
{
	u8 scale = (left->Scale > right.Scale) ? left->Scale : right.Scale;
	if (!ScaleDecimal(left, scale) || !ScaleDecimal(&right, scale))
		return FALSE;
	if (subtract)
	{
		if ((right.Mantissa > 0) && (left->Mantissa < -DECIMAL_MAX_VALUE + right.Mantissa))
			return FALSE;
		if ((right.Mantissa < 0) && (left->Mantissa > DECIMAL_MAX_VALUE + right.Mantissa))
			return FALSE;
		left->Mantissa -= right.Mantissa;
	}
	else
	{
		if ((right.Mantissa > 0) && (left->Mantissa > DECIMAL_MAX_VALUE - right.Mantissa))
			return FALSE;
		if ((right.Mantissa < 0) && (left->Mantissa < -DECIMAL_MAX_VALUE - right.Mantissa))
			return FALSE;
		left->Mantissa += right.Mantissa;
	}
	return TRUE;
}

static bool MultiplyDecimals(DecimalValue* left, DecimalValue right)
{
	u32 leftAbs = DecimalAbs(left->Mantissa);
	u32 rightAbs = DecimalAbs(right.Mantissa);
	if ((leftAbs != 0) && (rightAbs > (u32)DECIMAL_MAX_VALUE / leftAbs))
		return FALSE;
	if ((u16)left->Scale + right.Scale > EXPRESSION_MAX_SCALE)
		return FALSE;
	bool negative = ((left->Mantissa < 0) != (right.Mantissa < 0));
	u32 product = leftAbs * rightAbs;
	left->Mantissa = negative ? -(i32)product : (i32)product;
	left->Scale += right.Scale;
	return TRUE;
}

static bool DivideDecimals(DecimalValue* left, DecimalValue right)
{
	if (right.Mantissa == 0)
		return FALSE;
	u8 outScale = EXPRESSION_MAX_SCALE;
	i16 exponent = (i16)outScale + right.Scale - left->Scale;
	u32 magnitude = DecimalAbs(left->Mantissa);
	while ((exponent > 0) && (magnitude > (u32)DECIMAL_MAX_VALUE / 10))
	{
		outScale--;
		exponent--;
	}
	if (exponent < 0)
		return FALSE;
	while (exponent-- > 0)
		magnitude *= 10;
	u32 divisor = DecimalAbs(right.Mantissa);
	u32 quotient = magnitude / divisor;
	if (quotient > (u32)DECIMAL_MAX_VALUE)
		return FALSE;
	bool negative = ((left->Mantissa < 0) != (right.Mantissa < 0));
	left->Mantissa = negative ? -(i32)quotient : (i32)quotient;
	left->Scale = outScale;
	return TRUE;
}

static bool ParseExpressionTerm(ExpressionParser* parser, DecimalValue* value)
{
	if (!ParseExpressionFactor(parser, value))
		return FALSE;
	for (;;)
	{
		SkipExpressionSpaces(parser);
		c8 operation = *parser->Source;
		if ((operation != '*') && (operation != '/'))
			return TRUE;
		parser->Source++;
		DecimalValue right;
		if (!ParseExpressionFactor(parser, &right))
			return FALSE;
		if ((operation == '*') ? !MultiplyDecimals(value, right) : !DivideDecimals(value, right))
			return FALSE;
	}
}

static bool ParseExpression(ExpressionParser* parser, DecimalValue* value)
{
	if (!ParseExpressionTerm(parser, value))
		return FALSE;
	for (;;)
	{
		SkipExpressionSpaces(parser);
		c8 operation = *parser->Source;
		if ((operation != '+') && (operation != '-'))
			return TRUE;
		parser->Source++;
		DecimalValue right;
		if (!ParseExpressionTerm(parser, &right)
			|| !AddDecimals(value, right, operation == '-'))
			return FALSE;
	}
}

static bool EvaluateExpression(const c8* source, DecimalValue* value)
{
	ExpressionParser parser;
	parser.Source = source;
	parser.Depth = 0;
	if (!ParseExpression(&parser, value))
		return FALSE;
	SkipExpressionSpaces(&parser);
	return *parser.Source == 0;
}

enum ConditionOperator
{
	CONDITION_EQUAL,
	CONDITION_NOT_EQUAL,
	CONDITION_LESS,
	CONDITION_LESS_EQUAL,
	CONDITION_GREATER,
	CONDITION_GREATER_EQUAL
};

static bool EvaluateCondition(const c8* source, bool* result)
{
	ExpressionParser parser;
	DecimalValue left;
	DecimalValue right;
	parser.Source = source;
	parser.Depth = 0;
	if (!ParseExpression(&parser, &left))
		return FALSE;
	SkipExpressionSpaces(&parser);

	u8 operation;
	if (*parser.Source == '=')
	{
		operation = CONDITION_EQUAL;
		parser.Source++;
		if (*parser.Source == '=')
			parser.Source++;
	}
	else if (*parser.Source == '!')
	{
		if (parser.Source[1] != '=')
			return FALSE;
		operation = CONDITION_NOT_EQUAL;
		parser.Source += 2;
	}
	else if (*parser.Source == '<')
	{
		parser.Source++;
		if (*parser.Source == '=')
		{
			operation = CONDITION_LESS_EQUAL;
			parser.Source++;
		}
		else if (*parser.Source == '>')
		{
			operation = CONDITION_NOT_EQUAL;
			parser.Source++;
		}
		else
			operation = CONDITION_LESS;
	}
	else if (*parser.Source == '>')
	{
		parser.Source++;
		if (*parser.Source == '=')
		{
			operation = CONDITION_GREATER_EQUAL;
			parser.Source++;
		}
		else
			operation = CONDITION_GREATER;
	}
	else
		return FALSE;

	if (!ParseExpression(&parser, &right))
		return FALSE;
	SkipExpressionSpaces(&parser);
	if (*parser.Source)
		return FALSE;
	u8 scale = (left.Scale > right.Scale) ? left.Scale : right.Scale;
	if (!ScaleDecimal(&left, scale) || !ScaleDecimal(&right, scale))
		return FALSE;
	switch (operation)
	{
	case CONDITION_EQUAL:         *result = left.Mantissa == right.Mantissa; break;
	case CONDITION_NOT_EQUAL:     *result = left.Mantissa != right.Mantissa; break;
	case CONDITION_LESS:          *result = left.Mantissa < right.Mantissa; break;
	case CONDITION_LESS_EQUAL:    *result = left.Mantissa <= right.Mantissa; break;
	case CONDITION_GREATER:       *result = left.Mantissa > right.Mantissa; break;
	default:                     *result = left.Mantissa >= right.Mantissa; break;
	}
	return TRUE;
}

static bool FormatDecimal(DecimalValue value, const DbfField* field, c8* text, u8* length)
{
	if (field->Decimals > EXPRESSION_MAX_SCALE)
		return FALSE;
	if (value.Scale > field->Decimals)
	{
		u8 drop = value.Scale - field->Decimals;
		u32 divisor = 1;
		while (drop--)
			divisor *= 10;
		u32 magnitude = DecimalAbs(value.Mantissa);
		u32 rounded = (magnitude + divisor / 2) / divisor;
		if (rounded > (u32)DECIMAL_MAX_VALUE)
			return FALSE;
		value.Mantissa = (value.Mantissa < 0) ? -(i32)rounded : (i32)rounded;
		value.Scale = field->Decimals;
	}
	else if (!ScaleDecimal(&value, field->Decimals))
		return FALSE;

	u32 magnitude = DecimalAbs(value.Mantissa);
	c8 reverse[12];
	u8 digits = 0;
	do
	{
		reverse[digits++] = '0' + (u8)(magnitude % 10);
		magnitude /= 10;
	} while (magnitude && (digits < sizeof(reverse)));
	while (digits <= field->Decimals)
		reverse[digits++] = '0';
	u8 out = 0;
	if (value.Mantissa < 0)
		text[out++] = '-';
	for (u8 i = digits; i > 0; i--)
	{
		if ((value.Scale > 0) && (i == value.Scale))
			text[out++] = '.';
		text[out++] = reverse[i - 1];
	}
	text[out] = 0;
	*length = out;
	return out <= field->Length;
}

static bool IsWord(const c8* word, u8 length, const c8* expected)
{
	if (length != Len(expected))
		return FALSE;
	for (u8 i = 0; i < length; i++)
		if (Upper(word[i]) != expected[i])
			return FALSE;
	return TRUE;
}

static bool ApplyReplacement(u8 fieldIndex, const c8* expression, const c8* literal,
	u8 literalLen, bool numericExpression)
{
	const DbfField* field = &s_Db.Fields[fieldIndex];
	if (numericExpression)
	{
		if (field->Type != 'N')
			return FALSE;
		DecimalValue value;
		c8 formatted[DBF_MAX_RECLEN];
		u8 formattedLen;
		if (!EvaluateExpression(expression, &value)
			|| !FormatDecimal(value, field, formatted, &formattedLen))
			return FALSE;
		return Dbf_ReplaceField(&s_Db, s_DbName, fieldIndex,
			(const u8*)formatted, formattedLen) == DBF_OK;
	}
	return Dbf_ReplaceField(&s_Db, s_DbName, fieldIndex,
		(const u8*)literal, literalLen) == DBF_OK;
}

typedef struct ReplaceRequest
{
	u8 FieldIndex;
	bool All;
	bool NumericExpression;
	bool HasFor;
	const c8* Expression;
	const c8* Condition;
	const c8* Literal;
	u8 LiteralLen;
} ReplaceRequest;

static const c8* FindForClause(const c8* source)
{
	u8 depth = 0;
	c8 quote = 0;
	for (u8 i = 0; source[i]; i++)
	{
		c8 chr = source[i];
		if (quote)
		{
			if (chr == quote)
				quote = 0;
			continue;
		}
		if ((chr == '\'') || (chr == '"'))
		{
			quote = chr;
			continue;
		}
		if (chr == '(')
		{
			depth++;
			continue;
		}
		if ((chr == ')') && depth)
		{
			depth--;
			continue;
		}
	if (depth || (Upper(chr) != 'F')
		|| ((i > 0) && (source[i - 1] != ' ') && (source[i - 1] != '\t')))
			continue;
		if ((Upper(source[i + 1]) == 'O') && (Upper(source[i + 2]) == 'R')
			&& ((source[i + 3] == ' ') || (source[i + 3] == '\t')))
			return source + i;
	}
	return 0;
}

static bool CopyCommandPart(c8* target, const c8* source, u8 length)
{
	if (length >= REPLACE_TEXT_MAX)
		return FALSE;
	for (u8 i = 0; i < length; i++)
		target[i] = source[i];
	target[length] = 0;
	return TRUE;
}

static bool ParseReplaceRequest(const c8* args, ReplaceRequest* request)
{
	const c8* p = args;
	const c8* word;
	u8 wordLen = NextWord(&p, &word);
	request->All = IsWord(word, wordLen, "ALL");
	const c8* fieldName = word;
	u8 fieldLen = wordLen;
	if (request->All)
		fieldLen = NextWord(&p, &fieldName);
	const c8* withWord;
	u8 withLen = NextWord(&p, &withWord);
	if ((fieldLen == 0) || (withLen == 0) || !WordIs(withWord, withLen, "WITH"))
		return FALSE;

	i16 fieldIndex = FindField(fieldName, fieldLen);
	if (fieldIndex < 0)
		return FALSE;

	while ((*p == ' ') || (*p == ','))
		p++;
	if (!*p)
		return FALSE;
	const c8* forClause = FindForClause(p);
	request->HasFor = forClause != 0;
	if (request->HasFor && !request->All)
		return FALSE;
	u8 expressionLength = forClause ? (u8)(forClause - p) : Len(p);
	while (expressionLength && ((p[expressionLength - 1] == ' ')
		|| (p[expressionLength - 1] == '\t')))
		expressionLength--;
	if (!CopyCommandPart(s_ReplaceExpression, p, expressionLength))
		return FALSE;
	if (request->HasFor)
	{
		const c8* condition = forClause + 3;
		while ((*condition == ' ') || (*condition == '\t'))
			condition++;
		if (!*condition || !CopyCommandPart(s_ReplaceCondition, condition, Len(condition)))
			return FALSE;
		request->Condition = s_ReplaceCondition;
	}
	else
		request->Condition = 0;
	p = s_ReplaceExpression;

	const DbfField* field = &s_Db.Fields[(u8)fieldIndex];
	request->FieldIndex = (u8)fieldIndex;
	request->NumericExpression = (field->Type == 'N')
		&& ((*p == '(') || (*p == '+') || (*p == '-') || (*p == '.')
			|| ((*p >= '0') && (*p <= '9'))
			|| ((*p >= 'A') && (*p <= 'Z')) || ((*p >= 'a') && (*p <= 'z')));
	request->Expression = p;
	request->Literal = p;
	request->LiteralLen = 0;
	if (!request->NumericExpression)
	{
		if ((*p == '\'') || (*p == '"'))
		{
			c8 quote = *p++;
			request->Literal = p;
			while (p[request->LiteralLen] && (p[request->LiteralLen] != quote)
				&& (request->LiteralLen < 255))
				request->LiteralLen++;
			if ((p[request->LiteralLen] != quote) || p[request->LiteralLen + 1])
				return FALSE;
		}
		else
		{
			while (p[request->LiteralLen] && (p[request->LiteralLen] != ' ')
				&& (p[request->LiteralLen] != ',') && (request->LiteralLen < 255))
				request->LiteralLen++;
			if (p[request->LiteralLen])
				return FALSE;
		}
	}
	return TRUE;
}

static bool ValidateReplaceLiteral(const ReplaceRequest* request)
	{
	const DbfField* field = &s_Db.Fields[request->FieldIndex];
	if ((request->LiteralLen == 0) || (request->LiteralLen > field->Length))
		return FALSE;
	if (field->Type == 'L')
	{
		c8 value = Upper(request->Literal[0]);
		return (request->LiteralLen == 1)
			&& ((value == 'T') || (value == 'Y') || (value == 'F') || (value == 'N'));
	}
	if (field->Type == 'N')
	{
		DecimalValue value;
		return ParseDecimalText(request->Literal, request->LiteralLen, &value)
			&& (value.Scale <= field->Decimals);
	}
	return field->Type == 'C';
}

static bool ValidateReplaceAll(const ReplaceRequest* request)
	{
	const DbfField* field = &s_Db.Fields[request->FieldIndex];
	if (!request->NumericExpression && !ValidateReplaceLiteral(request))
		return FALSE;
	for (u32 recordNo = 1; recordNo <= s_Db.RecCount; recordNo++)
	{
		if (Dbf_Goto(&s_Db, recordNo) != DBF_OK)
			return FALSE;
		if (IsCurrentRecordHidden())
			continue;
		if (request->HasFor)
		{
			bool matches;
			if (!EvaluateCondition(request->Condition, &matches))
				return FALSE;
			if (!matches)
				continue;
		}
		if (request->NumericExpression)
		{
			DecimalValue value;
			c8 formatted[DBF_MAX_RECLEN];
			u8 formattedLen;
			if (!EvaluateExpression(request->Expression, &value)
				|| !FormatDecimal(value, field, formatted, &formattedLen))
				return FALSE;
		}
	}
	return TRUE;
}

static bool ApplyReplaceAll(const ReplaceRequest* request, u32* changed)
	{
	*changed = 0;
	for (u32 recordNo = 1; recordNo <= s_Db.RecCount; recordNo++)
	{
		if (Dbf_Goto(&s_Db, recordNo) != DBF_OK)
			return FALSE;
		if (IsCurrentRecordHidden())
			continue;
		if (request->HasFor)
		{
			bool matches;
			if (!EvaluateCondition(request->Condition, &matches))
				return FALSE;
			if (!matches)
				continue;
		}
		if (!ApplyReplacement(request->FieldIndex, request->Expression,
			request->Literal, request->LiteralLen, request->NumericExpression))
			return FALSE;
		(*changed)++;
	}
	return TRUE;
}

// REPLACE [ALL] campo WITH expressao - opera no registro atual ou em todos.
static void Cmd_Replace(const c8* args)
{
	if (!RequireDb())
		return;
	ReplaceRequest request;
	if (!ParseReplaceRequest(args, &request))
	{
		Console_Print("*** Sintaxe: REPLACE [ALL] campo WITH expressao [FOR condicao]\n");
		return;
	}
	if (!request.All)
	{
		if (!ApplyReplacement(request.FieldIndex, request.Expression, request.Literal,
			request.LiteralLen, request.NumericExpression))
			Console_Print("*** Expressao invalida ou nao suportada para o campo\n");
		else
			Console_Print("Campo atualizado\n");
		return;
	}
	if (s_Db.RecCount == 0)
	{
		Console_Print("*** Banco vazio\n");
		return;
	}
	if (!ValidateReplaceAll(&request))
	{
		Console_Print("*** Valor ou expressao invalida; nenhum registro foi alterado\n");
		return;
	}
	u32 changed;
	if (!ApplyReplaceAll(&request, &changed))
	{
		Console_Print("*** Falha de escrita; alteracoes anteriores podem permanecer\n");
		return;
	}
	Console_PrintNum(changed);
	Console_Print(" registro(s) atualizado(s)\n");
}

static void Cmd_Delete(const c8* args)
{
	const c8* word;
	if (NextWord(&args, &word) > 0)
	{
		Console_Print("*** Uso: DELETE (registro atual)\n");
		return;
	}
	if (!RequireDb())
		return;
	u8 err = Dbf_SetDeleted(&s_Db, s_DbName, TRUE);
	if (err == DBF_OK)
		Console_Print("Registro marcado para exclusao\n");
	else
		PrintError(err);
}

static void Cmd_Recall(const c8* args)
{
	const c8* word;
	if (NextWord(&args, &word) > 0)
	{
		Console_Print("*** Uso: RECALL (registro atual)\n");
		return;
	}
	if (!RequireDb())
		return;
	u8 err = Dbf_SetDeleted(&s_Db, s_DbName, FALSE);
	if (err == DBF_OK)
		Console_Print("Registro recuperado\n");
	else
		PrintError(err);
}

static void Cmd_Pack(const c8* args)
{
	const c8* word;
	if (NextWord(&args, &word) > 0)
	{
		Console_Print("*** Uso: PACK\n");
		return;
	}
	if (!RequireDb())
		return;
	u8 err = Dbf_Pack(&s_Db, s_DbName);
	if (err == DBF_OK)
		Console_Print("Banco compactado\n");
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
	{ "DISPLAY", Cmd_Display, "DISPLAY FILES [LIKE padrao]|STRUCTURE|ALL" },
	{ "GO",      Cmd_Go,      "GO TOP|BOTTOM|n          move o ponteiro" },
	{ "GOTO",    Cmd_Go,      0 },
	{ "SKIP",    Cmd_Skip,    "SKIP [n]                 avanca/volta n registros" },
	{ "APPEND",  Cmd_Append,  "APPEND [BLANK]            abre formulario ou acrescenta vazio" },
	{ "INSERT",  Cmd_Insert,  "INSERT [BEFORE] [BLANK]   insere antes/depois do atual" },
	{ "REPLACE", Cmd_Replace, "REPLACE [ALL] campo WITH expressao" },
	{ "DELETE",  Cmd_Delete,  "DELETE                   marca registro atual" },
	{ "RECALL",  Cmd_Recall,  "RECALL                   recupera registro atual" },
	{ "PACK",    Cmd_Pack,    "PACK                     remove registros excluidos" },
	{ "CLEAR",   Cmd_Clear,   "CLEAR                    limpa a tela" },
	{ "SET",     Cmd_Set,     "SET COLOR|DELETED|COLON|CONFIRM|HELP|MSX ..." },
	{ "COLOR",   Cmd_Color,   0 },
	{ "VER",     Cmd_Ver,     "VER                      versao e memoria" },
	{ "HELP",    Cmd_Help,    "HELP                     esta lista" },
	{ "QUIT",    Cmd_Quit,    "QUIT                     sai do programa" },
};

u8 Command_Complete(const c8* prefix, u8 prefixLen, u8 matchIndex,
	c8* result, u8 resultMax, u8* matchCount)
{
	u8 count = 0;

	for (u8 i = 0; i < COMMAND_COUNT; i++)
	{
		const c8* name = s_Commands[i].Name;
		u8 nameLen = Len(name);
		if (prefixLen > nameLen)
			continue;
		u8 j = 0;
		while ((j < prefixLen) && (Upper(prefix[j]) == name[j]))
			j++;
		if (j != prefixLen)
			continue;

		count++;
	}

	*matchCount = count;
	if (count == 0)
		return 0;

	u8 selected = 0;
	matchIndex %= count;
	for (u8 i = 0; i < COMMAND_COUNT; i++)
	{
		const c8* name = s_Commands[i].Name;
		u8 nameLen = Len(name);
		if (prefixLen > nameLen)
			continue;
		u8 j = 0;
		while ((j < prefixLen) && (Upper(prefix[j]) == name[j]))
			j++;
		if (j != prefixLen)
			continue;
		if (selected++ == matchIndex)
		{
			u8 resultLen = nameLen;
			if (resultLen >= resultMax)
				return 0;
			for (u8 k = 0; k < resultLen; k++)
				result[k] = name[k];
			result[resultLen] = 0;
			return resultLen;
		}
	}
	return 0;
}

static void Cmd_Help(const c8* args)
{
	const c8* p = args;
	const c8* topic;
	u8 topicLen = NextWord(&p, &topic);
	const c8* extra;
	if (NextWord(&p, &extra) > 0)
	{
		Console_Print("*** Uso: HELP [DBASE|COMANDO]\n");
		return;
	}
	u8 summaryCount = 0;
	for (u8 i = 0; i < COMMAND_COUNT; i++)
		if (s_Commands[i].Help)
			summaryCount++;
	Help_Show((topicLen > 0) ? topic : 0, topicLen,
		s_MsxOn && s_HelpScreen, summaryCount, HelpCommandSummary);
}

static const c8* HelpCommandSummary(u8 index)
{
	for (u8 i = 0; i < COMMAND_COUNT; i++)
	{
		if (!s_Commands[i].Help)
			continue;
		if (index-- == 0)
			return s_Commands[i].Help;
	}
	return 0;
}

void Command_Init(void)
{
	Dbf_Init(&s_Db);
	Help_Init();
	s_DeletedOn = FALSE;
	s_ColonOn = TRUE;
	s_ConfirmOn = TRUE;
	s_MsxOn = FALSE;
	s_HelpScreen = FALSE;
	Console_SetColor(15, 0);
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
