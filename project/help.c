#include "help.h"
#include "console.h"
#include "platform.h"

#define HELP_FILE_NAME "DB8SEMSG.TXT"
#define HELP_MAX_LINES 1024
#define HELP_MAX_TOPICS 160
#define HELP_TOPIC_MAX 16
#define HELP_LINE_MAX 80
#define HELP_READ_CHUNK 512
#define HELP_SUMMARY_HEADER_LINES 4
#define HELP_BODY_ROWS 19
#define HELP_TEXT_WIDTH 78
#define HELP_STATUS_ROW 20
#define HELP_KEYS_ROW 21
#define HELP_BOTTOM_ROW 22
#define HELP_SEARCH_MAX 40
#define HELP_GLYPH_FIRST 96
#define HELP_GLYPH_TL 98
#define HELP_GLYPH_TR 99
#define HELP_GLYPH_BL 96
#define HELP_GLYPH_BR 97
#define HELP_GLYPH_H  100
#define HELP_GLYPH_V  101

#define HELP_KEY_CR    0x0D
#define HELP_KEY_BS    0x08
#define HELP_KEY_TAB   0x09
#define HELP_KEY_UP    0x1E
#define HELP_KEY_DOWN  0x1F
#define HELP_KEY_RIGHT 0x1C
#define HELP_KEY_LEFT  0x1D
#define HELP_KEY_ESC   0x1B

typedef struct HelpDocument
{
	bool Summary;
	bool Combined;
	u16 StartLine;
	u16 LineCount;
	u16 FirstLineCount;
	u16 SecondStartLine;
} HelpDocument;

static u16 s_LineOffsets[HELP_MAX_LINES];
static u16 s_TopicLines[HELP_MAX_TOPICS];
static c8 s_TopicNames[HELP_MAX_TOPICS][HELP_TOPIC_MAX];
static u8 s_ReadBuffer[HELP_READ_CHUNK];
static u16 s_LineCount;
static u16 s_TopicCount;
static u16 s_FileSize;
static u16 s_LastReadLine;
static bool s_IndexBuilt;
static bool s_HaveReadLine;

void Help_Init(void)
{
	s_LineCount = 0;
	s_TopicCount = 0;
	s_FileSize = 0;
	s_LastReadLine = 0;
	s_IndexBuilt = FALSE;
	s_HaveReadLine = FALSE;
	for (u8 i = 0; i < HELP_MAX_TOPICS; i++)
		s_TopicNames[i][0] = 0;
}

static const u8 s_HelpFramePatterns[6 * 8] =
{
	0x18, 0x18, 0x18, 0x1F, 0x1F, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0xF8, 0xF8, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x1F, 0x1F, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0xF8, 0xF8, 0x18, 0x18, 0x18,
	0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18
};

static c8 Help_Translate(u8 byte)
{
	if ((byte >= ' ') && (byte <= '~'))
		return (c8)byte;
	switch (byte)
	{
	case 0x80: return 'C';
	case 0x82: case 0x88: return 'e';
	case 0x83: case 0x85: case 0xA0: return 'a';
	case 0x84: case 0xB5: return 'A';
	case 0x87: return 'c';
	case 0xA1: return 'i';
	case 0xA2: return 'o';
	case 0xA3: return 'u';
	case 0xB0: return ' ';
	case 0xB1: return 'a';
	case 0xB4: return 'o';
	case '\t': return ' ';
	default: return '?';
	}
}

static c8 Help_Upper(c8 ch)
{
	return ((ch >= 'a') && (ch <= 'z')) ? (ch - ('a' - 'A')) : ch;
}

static bool Help_BuildIndex(u8 file)
{
	if (s_IndexBuilt)
	{
		s_HaveReadLine = FALSE;
		return TRUE;
	}
	u32 fileSize = PFile_Size(file);
	if (fileSize > 0xFFFFUL)
		return FALSE;
	s_FileSize = (u16)fileSize;
	s_LineCount = 1;
	s_TopicCount = 0;
	s_LineOffsets[0] = 0;
	if (!PFile_Seek(file, 0))
		return FALSE;

	u32 offset = 0;
	bool lineStart = TRUE;
	bool topicActive = FALSE;
	u8 activeTopic = 0;
	u8 topicLength = 0;
	while (offset < fileSize)
	{
		u32 remaining = fileSize - offset;
		u16 request = (remaining > sizeof(s_ReadBuffer)) ? sizeof(s_ReadBuffer) : (u16)remaining;
		u16 count = PFile_Read(file, s_ReadBuffer, request);
		if (count != request)
			return FALSE;
		for (u16 i = 0; i < count; i++)
		{
			u8 byte = s_ReadBuffer[i];
			if (lineStart && (byte == '*'))
			{
				if (s_TopicCount >= HELP_MAX_TOPICS)
					return FALSE;
				activeTopic = s_TopicCount++;
				s_TopicLines[activeTopic] = s_LineCount - 1;
				s_TopicNames[activeTopic][0] = 0;
				topicLength = 0;
				topicActive = TRUE;
			}
			else if (lineStart)
				topicActive = FALSE;
			if (topicActive && ((byte == ' ') || (byte == '\t')))
				topicActive = FALSE;
			else if (topicActive && (byte != '*') && (byte != '\r') && (byte != '\n')
				&& (topicLength + 1 < HELP_TOPIC_MAX))
			{
				s_TopicNames[activeTopic][topicLength++] = Help_Translate(byte);
				s_TopicNames[activeTopic][topicLength] = 0;
			}
			if (byte == '\n')
			{
				if (s_LineCount >= HELP_MAX_LINES)
					return FALSE;
				s_LineOffsets[s_LineCount++] = (u16)(offset + i + 1);
				lineStart = TRUE;
			}
			else if (byte != '\r')
				lineStart = FALSE;
		}
		offset += count;
	}
	if ((s_LineCount > 1) && (s_LineOffsets[s_LineCount - 1] == s_FileSize))
		s_LineCount--;
	s_IndexBuilt = TRUE;
	s_HaveReadLine = FALSE;
	return TRUE;
}

static bool Help_ReadFileLine(u8 file, u16 lineNo, c8* line)
{
	if (lineNo >= s_LineCount)
		return FALSE;
	u32 start = s_LineOffsets[lineNo];
	u32 end = (lineNo + 1 < s_LineCount) ? s_LineOffsets[lineNo + 1] : s_FileSize;
	u16 rawLength = (u16)(end - start);
	u8 raw[HELP_LINE_MAX + 2];
	if (rawLength > sizeof(raw))
		rawLength = sizeof(raw);
	if ((!s_HaveReadLine || (lineNo != s_LastReadLine + 1))
		&& !PFile_Seek(file, start))
		return FALSE;
	if (PFile_Read(file, raw, rawLength) != rawLength)
		return FALSE;
	s_LastReadLine = lineNo;
	s_HaveReadLine = TRUE;
	while (rawLength && ((raw[rawLength - 1] == '\n') || (raw[rawLength - 1] == '\r')))
		rawLength--;
	if (rawLength > HELP_LINE_MAX)
		rawLength = HELP_LINE_MAX;
	for (u16 i = 0; i < rawLength; i++)
		line[i] = Help_Translate(raw[i]);
	line[rawLength] = 0;
	return TRUE;
}

static bool Help_MatchesTopicName(const c8* name, const c8* topic)
{
	u8 j = 0;
	while ((j < HELP_TOPIC_MAX) && name[j] && topic[j])
	{
		if (Help_Upper(name[j]) != Help_Upper(topic[j]))
			return FALSE;
		j++;
	}
	return (j < HELP_TOPIC_MAX) && (name[j] == 0) && (topic[j] == 0);
}

static bool Help_FindTopic(u8 file, const c8* topic, u16* startLine, u16* lineCount)
{
	(void)file;
	for (u8 topicIndex = 0; (topicIndex < s_TopicCount)
		&& (topicIndex < HELP_MAX_TOPICS)
		&& s_TopicNames[topicIndex][0]; topicIndex++)
	{
		u16 markerLine = s_TopicLines[topicIndex];
		if (!Help_MatchesTopicName(s_TopicNames[topicIndex], topic))
			continue;

		*startLine = markerLine + 1;
		for (u8 endIndex = topicIndex + 1; (endIndex < s_TopicCount)
			&& (endIndex < HELP_MAX_TOPICS)
			&& s_TopicNames[endIndex][0]; endIndex++)
		{
			if (Help_MatchesTopicName(s_TopicNames[endIndex], "EXIT"))
			{
				u16 endLine = s_TopicLines[endIndex];
				*lineCount = endLine - *startLine;
				return TRUE;
			}
		}
		return FALSE;
	}
	return FALSE;
}

static u16 Help_DocumentLineCount(const HelpDocument* document, u8 summaryCount)
{
	return document->Summary ? HELP_SUMMARY_HEADER_LINES + summaryCount : document->LineCount;
}

static bool Help_CopyText(c8* target, const c8* source)
{
	u8 i = 0;
	while (source[i] && (i < HELP_LINE_MAX))
	{
		target[i] = source[i];
		i++;
	}
	target[i] = 0;
	return TRUE;
}

static bool Help_GetLine(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt, u16 lineNo, c8* line)
{
	if (!document->Summary && !document->Combined)
		return Help_ReadFileLine(file, document->StartLine + lineNo, line);
	if (document->Combined)
	{
		if (lineNo < document->FirstLineCount)
			return Help_ReadFileLine(file, document->StartLine + lineNo, line);
		return Help_ReadFileLine(file, document->SecondStartLine
			+ lineNo - document->FirstLineCount, line);
	}
	if (lineNo == 0)
		return Help_CopyText(line, "dBASE II PLUS MSX");
	if (lineNo == 1)
		return Help_CopyText(line, "HELP DBASE: detalhes do sistema e arquivos.");
	if (lineNo == 2)
		return Help_CopyText(line, "HELP <COMANDO>: detalhes completos do comando.");
	if (lineNo == 3)
		return Help_CopyText(line, "Comandos:");
	if ((lineNo < HELP_SUMMARY_HEADER_LINES)
		|| ((lineNo - HELP_SUMMARY_HEADER_LINES) >= summaryCount))
		return FALSE;
	const c8* source = summaryAt ? summaryAt(lineNo - HELP_SUMMARY_HEADER_LINES) : 0;
	return source ? Help_CopyText(line, source) : Help_CopyText(line, "");
}

static bool Help_Contains(const c8* line, const c8* query)
{
	u8 queryLength = 0;
	while (query[queryLength])
		queryLength++;
	if (queryLength == 0)
		return FALSE;
	for (u8 start = 0; line[start]; start++)
	{
		u8 i = 0;
		while (i < queryLength && line[start + i]
			&& (Help_Upper(line[start + i]) == Help_Upper(query[i])))
			i++;
		if (i == queryLength)
			return TRUE;
	}
	return FALSE;
}

static bool Help_FindNext(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt, const c8* query, u16 from, u16* found)
{
	c8 line[HELP_LINE_MAX + 1];
	u16 lineCount = Help_DocumentLineCount(document, summaryCount);
	if (lineCount == 0)
		return FALSE;
	for (u16 offset = 0; offset < lineCount; offset++)
	{
		u16 lineNo = (from + offset) % lineCount;
		if (Help_GetLine(file, document, summaryCount, summaryAt, lineNo, line)
			&& Help_Contains(line, query))
		{
			*found = lineNo;
			return TRUE;
		}
	}
	return FALSE;
}

static void Help_ShowText(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt)
{
	c8 line[HELP_LINE_MAX + 1];
	u16 total = Help_DocumentLineCount(document, summaryCount);
	u8 pageLines = 0;
	for (u16 i = 0; i < total; i++)
	{
		if (!Help_GetLine(file, document, summaryCount, summaryAt, i, line))
		{
			Console_Print("*** Erro lendo arquivo HELP\n");
			return;
		}
		Console_Print(line);
		Console_NewLine();
		pageLines++;
		if ((pageLines >= CON_ROWS - 3) && (i + 1 < total))
		{
			Console_SetPos(0, CON_ROWS - 1);
			Console_Print("Esperando - tecla continua, ESC interrompe");
			if ((u8)Console_ReadKey() == HELP_KEY_ESC)
				return;
			Console_Clear();
			pageLines = 0;
		}
	}
}

static void Help_DrawBorder(u8 y, u8 leftShape, u8 rightShape)
{
	u8 shapes[CON_COLS];
	shapes[0] = leftShape;
	for (u8 i = 1; i < CON_COLS - 1; i++)
		shapes[i] = HELP_GLYPH_H;
	shapes[CON_COLS - 1] = rightShape;
	Console_DrawShapesAt(0, y, shapes, CON_COLS);
}

static void Help_DrawFrame(const c8* title, u8 fg, u8 bg)
{
	Console_Clear();
	Console_LoadGlyphs(HELP_GLYPH_FIRST, s_HelpFramePatterns, 6);
	Console_SetColor(fg, bg);
	Help_DrawBorder(0, HELP_GLYPH_TL, HELP_GLYPH_TR);
	c8 titleText[HELP_LINE_MAX];
	u8 titleLength = 0;
	const c8* prefix = " HELP: ";
	while (prefix[titleLength])
	{
		titleText[titleLength] = prefix[titleLength];
		titleLength++;
	}
	for (u8 i = 0; title[i] && (titleLength < HELP_LINE_MAX - 1); i++)
		titleText[titleLength++] = title[i];
	titleText[titleLength] = 0;
	Console_DrawTextAt((CON_COLS - titleLength) / 2, 0, titleText, titleLength);

	for (u8 row = 0; row < HELP_BODY_ROWS; row++)
	{
		u8 sides[1] = { HELP_GLYPH_V };
		Console_DrawShapesAt(0, row + 1, sides, 1);
		Console_DrawShapesAt(CON_COLS - 1, row + 1, sides, 1);
	}

	u8 sides[1] = { HELP_GLYPH_V };
	Console_DrawShapesAt(0, HELP_STATUS_ROW, sides, 1);
	Console_DrawShapesAt(CON_COLS - 1, HELP_STATUS_ROW, sides, 1);
	Console_DrawShapesAt(0, HELP_KEYS_ROW, sides, 1);
	Console_DrawShapesAt(CON_COLS - 1, HELP_KEYS_ROW, sides, 1);
	c8 keysLine[HELP_TEXT_WIDTH];
	for (u8 i = 0; i < HELP_TEXT_WIDTH; i++)
		keysLine[i] = ' ';
	const c8* keyHelp = "Up/Down rolam  Space/Enter pagina  / busca  N proximo  Esc sai";
	for (u8 i = 0; keyHelp[i] && (i < HELP_TEXT_WIDTH); i++)
		keysLine[i] = keyHelp[i];
	Console_DrawTextAt(1, HELP_KEYS_ROW, keysLine, HELP_TEXT_WIDTH);
	Help_DrawBorder(HELP_BOTTOM_ROW, HELP_GLYPH_BL, HELP_GLYPH_BR);
}

static void Help_DrawBodyRow(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt, u16 topLine, u8 horizontal, u8 row)
{
	c8 line[HELP_LINE_MAX + 1];
	c8 visible[HELP_TEXT_WIDTH];
	u8 sides[1] = { HELP_GLYPH_V };
	u8 length = 0;
	Console_DrawShapesAt(0, row + 1, sides, 1);
	Console_DrawShapesAt(CON_COLS - 1, row + 1, sides, 1);
	if (((topLine + row) < Help_DocumentLineCount(document, summaryCount))
		&& Help_GetLine(file, document, summaryCount, summaryAt, topLine + row, line))
	{
		while (line[length] && (length < HELP_LINE_MAX))
			length++;
	}
	for (u8 column = 0; column < HELP_TEXT_WIDTH; column++)
	{
		u16 sourceColumn = horizontal + column;
		visible[column] = (sourceColumn < length) ? line[sourceColumn] : ' ';
	}
	Console_DrawTextAt(1, row + 1, visible, HELP_TEXT_WIDTH);
}

static void Help_DrawBody(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt, u16 topLine, u8 horizontal)
{
	for (u8 row = 0; row < HELP_BODY_ROWS; row++)
		Help_DrawBodyRow(file, document, summaryCount, summaryAt,
			topLine, horizontal, row);
}

static void Help_DrawStatus(const c8* status)
{
	c8 statusLine[HELP_TEXT_WIDTH];
	for (u8 i = 0; i < HELP_TEXT_WIDTH; i++)
		statusLine[i] = ' ';
	for (u8 i = 0; status[i] && (i < HELP_TEXT_WIDTH); i++)
		statusLine[i] = status[i];
	Console_DrawTextAt(1, HELP_STATUS_ROW, statusLine, HELP_TEXT_WIDTH);
}

static u8 Help_ReadQuery(c8* query, u8 max)
{
	c8 blank[HELP_TEXT_WIDTH];
	for (u8 i = 0; i < HELP_TEXT_WIDTH; i++)
		blank[i] = ' ';
	Console_DrawTextAt(1, HELP_STATUS_ROW, blank, HELP_TEXT_WIDTH);
	Console_SetPos(1, HELP_STATUS_ROW);
	Console_Print("Buscar: ");
	return Console_ReadLine(query, max, 0);
}

static void Help_ShowScreen(u8 file, const HelpDocument* document, u8 summaryCount,
	HelpSummaryFn summaryAt, const c8* title)
{
	u8 oldFg, oldBg;
	Console_GetColor(&oldFg, &oldBg);
	u16 total = Help_DocumentLineCount(document, summaryCount);
	u16 topLine = 0;
	u16 lastFound = 0;
	u8 horizontal = 0;
	c8 query[HELP_SEARCH_MAX];
	query[0] = 0;
	const c8* status = "";
	bool done = FALSE;

	Help_DrawFrame(title, oldFg, oldBg);
	Help_DrawBody(file, document, summaryCount, summaryAt, topLine, horizontal);
	Help_DrawStatus(status);
	while (!done)
	{
		u8 key = (u8)Console_ReadKey();
		bool redrawBody = FALSE;
		bool redrawStatus = FALSE;
		bool scrollRows = FALSE;
		bool scrollUp = FALSE;
		u8 exposedRow = 0;
		if (key == HELP_KEY_ESC)
			done = TRUE;
		else if ((key == HELP_KEY_UP) && (topLine > 0))
		{
			topLine--;
			scrollRows = TRUE;
			exposedRow = 0;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == HELP_KEY_DOWN) && (topLine + HELP_BODY_ROWS < total))
		{
			topLine++;
			scrollRows = TRUE;
			scrollUp = TRUE;
			exposedRow = HELP_BODY_ROWS - 1;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == HELP_KEY_LEFT) && (horizontal > 0))
		{
			horizontal--;
			redrawBody = TRUE;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == HELP_KEY_RIGHT) && (horizontal < 8))
		{
			horizontal++;
			redrawBody = TRUE;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == HELP_KEY_BS) || (key == ' '))
		{
			if (key == HELP_KEY_BS)
				topLine = (topLine > HELP_BODY_ROWS) ? topLine - HELP_BODY_ROWS : 0;
			else if (topLine + HELP_BODY_ROWS < total)
				topLine += HELP_BODY_ROWS;
			redrawBody = TRUE;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == HELP_KEY_CR) || (key == HELP_KEY_TAB))
		{
			if (topLine + HELP_BODY_ROWS < total)
				topLine += HELP_BODY_ROWS;
			redrawBody = TRUE;
			status = "";
			redrawStatus = TRUE;
		}
		else if ((key == '/') || (Help_Upper((c8)key) == 'F'))
		{
			if (Help_ReadQuery(query, sizeof(query)))
			{
				u16 found;
				if (Help_FindNext(file, document, summaryCount, summaryAt,
					query, topLine + 1, &found))
				{
					topLine = found;
					lastFound = found;
					status = "Encontrado. N procura a proxima ocorrencia.";
					redrawBody = TRUE;
				}
				else
					status = "Texto nao encontrado.";
				redrawStatus = TRUE;
			}
			else
			{
				status = "";
				redrawStatus = TRUE;
			}
		}
		else if ((Help_Upper((c8)key) == 'N') && query[0])
		{
			u16 found;
			if (Help_FindNext(file, document, summaryCount, summaryAt,
				query, lastFound + 1, &found))
			{
				topLine = found;
				lastFound = found;
				status = "Encontrado. N procura a proxima ocorrencia.";
				redrawBody = TRUE;
			}
			else
				status = "Fim da busca.";
			redrawStatus = TRUE;
		}
		else
			continue;

		if (scrollRows)
		{
			Console_ScrollRows(1, HELP_BODY_ROWS, scrollUp);
			Help_DrawBodyRow(file, document, summaryCount, summaryAt,
				topLine, horizontal, exposedRow);
		}
		else if (redrawBody)
			Help_DrawBody(file, document, summaryCount, summaryAt, topLine, horizontal);
		if (redrawStatus)
			Help_DrawStatus(status);
	}
	Console_SetColor(oldFg, oldBg);
	Console_Clear();
}

void Help_Show(const c8* topic, u8 topicLen, bool screen,
	u8 summaryCount, HelpSummaryFn summaryAt)
{
	HelpDocument document;
	document.Summary = FALSE;
	document.Combined = FALSE;
	document.StartLine = 0;
	document.LineCount = 0;
	document.FirstLineCount = 0;
	document.SecondStartLine = 0;
	const c8* title = "HELP";
	u8 file = PFILE_INVALID;
	if ((topic == 0) || (topicLen == 0))
	{
		document.Summary = TRUE;
	}
	else
	{
		c8 topicName[HELP_LINE_MAX + 1];
		for (u8 i = 0; i < topicLen; i++)
			topicName[i] = topic[i];
		topicName[topicLen] = 0;
		title = topicName;
		file = PFile_OpenRead(HELP_FILE_NAME);
		if (file == PFILE_INVALID)
			file = PFile_OpenRead("db8semsg.txt");
		if (file == PFILE_INVALID)
		{
			Console_Print("*** DB8SEMSG.TXT nao encontrado\n");
			return;
		}
		if (!Help_BuildIndex(file))
		{
			PFile_Close(file);
			Console_Print("*** Indice DB8SEMSG.TXT falhou\n");
			return;
		}
		if (!Help_FindTopic(file, topicName, &document.StartLine, &document.LineCount))
		{
			PFile_Close(file);
			Console_Print("*** Nao ha mensagem HELP para esse topico\n");
			return;
		}
	}

	if (screen)
		Help_ShowScreen(file, &document, summaryCount, summaryAt, title);
	else
		Help_ShowText(file, &document, summaryCount, summaryAt);
	if (file != PFILE_INVALID)
		PFile_Close(file);
}