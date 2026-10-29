#include "console.h"
#include "bios.h"
#include "font/font_mgl_std0.h"

// Video: acesso direto ao VDP, sem BIOS de tela nem MSX-DOS.
// Teclado: BIOS (CHSNS/CHGET via CALSLT), sem funcoes de console do MSX-DOS.

#define KEY_CR   0x0D
#define KEY_BS   0x08
#define KEY_DEL  0x7F
#define KEY_TAB  0x09
#define KEY_UP   0x1E
#define KEY_DOWN 0x1F
#define KEY_RIGHT 0x1C
#define KEY_LEFT  0x1D

#define CELLS    (CON_COLS * CON_ROWS)
#define HISTORY_SIZE 8

// Posicao do cursor, em colunas/linhas de texto
static u8 s_CurX;
static u8 s_CurY;
static u8 s_TextFg;
static u8 s_TextBg;

// Copia em RAM da tabela de nomes do VDP (codigo do padrao de cada celula).
// Permite rolar a tela na RAM e enviar tudo ao VDP numa unica escrita.
static u8 s_Shadow[CELLS];

// Faixa ASCII da fonte embutida do MSXgl.
static u8 s_FontFirst;
static u8 s_FontLast;

// Cursor da BIOS: desligado enquanto o programa roda
static u8 s_OldCursorSw;
static c8 s_History[HISTORY_SIZE][CON_COLS];
static u8 s_HistoryCount;

//=============================================================================
// TECLADO (BIOS)
//=============================================================================

// Pega uma tecla do buffer da BIOS sem esperar. Retorna 0 se nao houver.
// Chama CHSNS e CHGET pelo slot da ROM principal (CALSLT), valido tambem sob MSX-DOS.
// CHGET so e chamado com tecla no buffer, entao nao chega a desenhar o cursor da BIOS.
static c8 PollKey(void) __NAKED
{
__asm
	push	ix
	ld		a, (#M_EXPTBL)			// slot da ROM principal
	ld		h, a
	ld		l, #0
	push	hl
	pop		iy
	ld		ix, #R_CHSNS
	call	R_CALSLT
	ei								// CALSLT desliga as interrupcoes
	jr		z, pollkey_empty		// Z = buffer vazio
	ld		ix, #R_CHGET
	call	R_CALSLT
	ei
	pop		ix
	ret								// tecla em A
pollkey_empty:
	xor		a
	pop		ix
	ret
__endasm;
}

c8 Console_ReadKey(void)
{
	c8 key;
	while ((key = PollKey()) == 0)
		Halt();						// espera a proxima interrupcao (1/60 s)
	return key;
}

//=============================================================================
// VIDEO (VDP direto)
//=============================================================================

// Padrao da fonte para um caractere. Espaco e fora da faixa = em branco.
static u8 ShapeOf(u8 chr)
{
	if (chr <= ' ')
		return 0;
	if ((chr < s_FontFirst) || (chr > s_FontLast))
		return 0;
	return chr - s_FontFirst + 1;
}

// Envia ao VDP um trecho da copia em RAM
static void Flush(u16 first, u16 count)
{
	if (count)
		VDP_WriteVRAM_16K(s_Shadow + first, g_ScreenLayoutLow + first, count);
}

// Escreve uma celula (RAM + VDP)
static void PutShape(u8 x, u8 y, u8 shape)
{
	u16 i = (u16)y * CON_COLS + x;
	s_Shadow[i] = shape;
	VDP_Poke_16K(shape, g_ScreenLayoutLow + i);
}

// Sobe todas as linhas na RAM e reenvia a tela inteira ao VDP
static void Scroll(void)
{
	Mem_Copy(s_Shadow + CON_COLS, s_Shadow, CELLS - CON_COLS);
	Mem_Set(0, s_Shadow + CELLS - CON_COLS, CON_COLS);
	Flush(0, CELLS);
}

// Interruptor do cursor da BIOS (0xFCA9): a rotina de teclado da BIOS o desenha numa posicao propria
void Console_Init(void)
{
	s_OldCursorSw = *(volatile u8*)M_CSRSW;
	*(volatile u8*)M_CSRSW = 0;

	VDP_SetMode(VDP_MODE_TEXT2);
	Console_SetColor(15, 1);
	VDP_FillVRAM_16K(0, 0x0000, 0x4000);      // limpa a VRAM

	Print_SetTextFont(g_Font_MGL_Std0, 1);
	s_FontFirst = g_Font_MGL_Std0[2];
	s_FontLast = g_Font_MGL_Std0[3];

	Mem_Set(0, s_Shadow, CELLS);
	s_CurX = 0;
	s_CurY = 0;
	s_HistoryCount = 0;
}

void Console_Done(void)
{
	*(volatile u8*)M_CSRSW = s_OldCursorSw;
}

void Console_SetColor(u8 fg, u8 bg)
{
	s_TextFg = fg & 0x0F;
	s_TextBg = bg & 0x0F;
	VDP_SetColor2(bg & 0x0F, fg & 0x0F);      // registrador 7: texto | fundo/borda
}

void Console_GetColor(u8* fg, u8* bg)
{
	*fg = s_TextFg;
	*bg = s_TextBg;
}

void Console_Clear(void)
{
	Mem_Set(0, s_Shadow, CELLS);
	VDP_FillVRAM_16K(0, g_ScreenLayoutLow, CELLS);
	s_CurX = 0;
	s_CurY = 0;
}

void Console_SetPos(u8 x, u8 y)
{
	if (x >= CON_COLS)
		x = CON_COLS - 1;
	if (y >= CON_ROWS)
		y = CON_ROWS - 1;
	s_CurX = x;
	s_CurY = y;
}

void Console_LoadGlyphs(u8 firstShape, const u8* patterns, u8 count)
{
	if ((u16)firstShape + count > 256)
		return;
	VDP_WriteVRAM_16K(patterns,
		g_ScreenPatternLow + ((u16)firstShape * 8), (u16)count * 8);
}

void Console_DrawShapesAt(u8 x, u8 y, const u8* shapes, u8 len)
{
	if ((x >= CON_COLS) || (y >= CON_ROWS))
		return;
	if (len > CON_COLS - x)
		len = CON_COLS - x;
	u16 start = (u16)y * CON_COLS + x;
	for (u8 i = 0; i < len; i++)
		s_Shadow[start + i] = shapes[i];
	Flush(start, len);
}

void Console_DrawTextAt(u8 x, u8 y, const c8* text, u8 len)
{
	if ((x >= CON_COLS) || (y >= CON_ROWS))
		return;
	if (len > CON_COLS - x)
		len = CON_COLS - x;
	u8 shapes[CON_COLS];
	for (u8 i = 0; i < len; i++)
		shapes[i] = ShapeOf((u8)text[i]);
	Console_DrawShapesAt(x, y, shapes, len);
}

void Console_ScrollRows(u8 y, u8 height, bool up)
{
	if ((height < 2) || (y >= CON_ROWS) || (height > CON_ROWS - y))
		return;
	u16 start = (u16)y * CON_COLS;
	u16 cells = (u16)height * CON_COLS;
	if (up)
	{
		for (u16 i = 0; i < cells - CON_COLS; i++)
			s_Shadow[start + i] = s_Shadow[start + i + CON_COLS];
		Mem_Set(0, s_Shadow + start + cells - CON_COLS, CON_COLS);
	}
	else
	{
		for (u16 i = cells; i > CON_COLS; i--)
			s_Shadow[start + i - 1] = s_Shadow[start + i - 1 - CON_COLS];
		Mem_Set(0, s_Shadow + start, CON_COLS);
	}
	Flush(start, cells);
}

void Console_NewLine(void)
{
	s_CurX = 0;
	if (s_CurY < CON_ROWS - 1)
		s_CurY++;
	else
		Scroll();
}

// Nucleo de escrita: grava os caracteres na copia e envia ao VDP um trecho por linha de tela
static void WriteText(const u8* s, u8 len)
{
	u16 start = (u16)s_CurY * CON_COLS + s_CurX;
	u8 run = 0;

	while (len--)
	{
		u8 chr = *s++;
		if ((chr == '\n') || (s_CurX >= CON_COLS))
		{
			Flush(start, run);
			Console_NewLine();
			start = (u16)s_CurY * CON_COLS;
			run = 0;
			if (chr == '\n')
				continue;
		}
		s_Shadow[start + run] = ShapeOf(chr);
		run++;
		s_CurX++;
	}
	Flush(start, run);
}

void Console_PutChar(c8 chr)
{
	WriteText((const u8*)&chr, 1);
}

void Console_Print(const c8* str)
{
	u8 len = 0;
	while (str[len])
		len++;
	WriteText((const u8*)str, len);
}

void Console_PrintN(const u8* data, u8 len)
{
	WriteText(data, len);
}

void Console_PrintNum(u32 value)
{
	c8 digits[10];
	u8 n = 0;
	do
	{
		digits[n++] = '0' + (u8)(value % 10);
		value /= 10;
	} while (value > 0);

	// Inverte para a ordem de leitura e escreve de uma vez
	c8 text[10];
	for (u8 i = 0; i < n; i++)
		text[i] = digits[n - 1 - i];
	WriteText((const u8*)text, n);
}

static u8 TextLength(const c8* text)
{
	u8 length = 0;
	while (text[length])
		length++;
	return length;
}

static void CopyText(c8* target, const c8* source, u8 max)
{
	u8 i = 0;
	while (source[i] && (i + 1 < max))
	{
		target[i] = source[i];
		i++;
	}
	target[i] = 0;
}

static void StoreHistory(const c8* line)
{
	if (!line[0])
		return;
	if (s_HistoryCount && (TextLength(line) == TextLength(s_History[s_HistoryCount - 1])))
	{
		u8 i = 0;
		while (line[i] && (line[i] == s_History[s_HistoryCount - 1][i]))
			i++;
		if (!line[i])
			return;
	}
	if (s_HistoryCount == HISTORY_SIZE)
	{
		for (u8 i = 1; i < HISTORY_SIZE; i++)
			CopyText(s_History[i - 1], s_History[i], CON_COLS);
		s_HistoryCount--;
	}
	CopyText(s_History[s_HistoryCount++], line, CON_COLS);
}

static void RedrawLine(const c8* buf, u8 len, u8 oldLen, u8 startX, u8 cursorPos)
{
	u8 drawLen = (len > oldLen) ? len : oldLen;
	for (u8 i = 0; i < drawLen; i++)
		PutShape(startX + i, s_CurY, (i < len) ? ShapeOf(buf[i]) : 0);
	s_CurX = startX + cursorPos;
}

static void RedrawField(const c8* buf, u8 len, u8 width, u8 x, u8 y, u8 cursorPos)
{
	u8 viewStart = (cursorPos > width) ? cursorPos - width : 0;
	for (u8 i = 0; i < width; i++)
		PutShape(x + i, y, (viewStart + i < len) ? ShapeOf(buf[viewStart + i]) : 0);
	u8 cursorOffset = cursorPos - viewStart;
	if (cursorOffset >= width)
		cursorOffset = width - 1;
	s_CurX = x + cursorOffset;
	s_CurY = y;
}

u8 Console_EditField(c8* buf, u8* length, u8 maxLen, u8 visibleWidth,
	u8 x, u8 y, bool autoAdvance)
{
	u8 len = *length;
	if (len > maxLen)
		len = maxLen;
	if ((maxLen == 0) || (visibleWidth == 0))
		return KEY_CR;
	u8 cursorPos = len;
	buf[len] = 0;

	for (;;)
	{
		RedrawField(buf, len, visibleWidth, x, y, cursorPos);
		u16 cursorCell = (u16)y * CON_COLS + s_CurX;
		u8 savedShape = s_Shadow[cursorCell];
		PutShape(s_CurX, y, ShapeOf('_'));
		u8 key = (u8)Console_ReadKey();
		PutShape(s_CurX, y, savedShape);

		if ((key == KEY_CR) || (key == KEY_TAB) || (key == KEY_UP)
			|| (key == KEY_DOWN) || (key == 0x1B))
		{
			buf[len] = 0;
			*length = len;
			return key;
		}

		if (key == KEY_LEFT)
		{
			if (cursorPos > 0)
				cursorPos--;
		}
		else if (key == KEY_RIGHT)
		{
			if (cursorPos < len)
				cursorPos++;
		}
		else if (key == KEY_BS)
		{
			if (cursorPos > 0)
			{
				for (u8 i = cursorPos - 1; i < len - 1; i++)
					buf[i] = buf[i + 1];
				len--;
				cursorPos--;
				buf[len] = 0;
			}
		}
		else if (key == KEY_DEL)
		{
			if (cursorPos < len)
			{
				for (u8 i = cursorPos; i < len - 1; i++)
					buf[i] = buf[i + 1];
				len--;
				buf[len] = 0;
			}
		}
		else if (((u8)key >= ' ') && ((u8)key < KEY_DEL) && (len < maxLen))
		{
			for (u8 i = len; i > cursorPos; i--)
				buf[i] = buf[i - 1];
			buf[cursorPos++] = key;
			len++;
			buf[len] = 0;
		}
		*length = len;
		if (autoAdvance && (len == maxLen))
		{
			RedrawField(buf, len, visibleWidth, x, y, cursorPos);
			return CONSOLE_EDIT_FIELD_FULL;
		}
	}
}

static u8 FirstTokenLength(const c8* buf)
{
	u8 len = 0;
	while (buf[len] && (buf[len] != ' ') && (buf[len] != ','))
		len++;
	return len;
}

u8 Console_ReadLine(c8* buf, u8 max, ConsoleCompleteFn complete)
{
	u8 len = 0;
	u8 cursorPos = 0;
	u8 startX = s_CurX;
	u8 limit = CON_COLS - 1 - startX;
	u8 historyPos = s_HistoryCount;
	bool browsingHistory = FALSE;
	bool tabActive = FALSE;
	u8 tabIndex = 0;
	u8 tabPrefixLen = 0;
	c8 tabPrefix[CON_COLS];
	c8 completionTail[CON_COLS];
	if (limit > max - 1)
		limit = max - 1;
	buf[0] = 0;

	for (;;)
	{
		u16 cursorCell = (u16)s_CurY * CON_COLS + s_CurX;
		u8 savedShape = s_Shadow[cursorCell];
		PutShape(s_CurX, s_CurY, ShapeOf('_'));
		c8 key = Console_ReadKey();
		PutShape(s_CurX, s_CurY, savedShape);

		if (key == KEY_CR)
			break;

		if ((key == KEY_UP) || (key == KEY_DOWN))
		{
			if (s_HistoryCount)
			{
				u8 oldLen = len;
				if ((key == KEY_UP) && (historyPos > 0))
				{
					browsingHistory = TRUE;
					historyPos--;
					CopyText(buf, s_History[historyPos], max);
				}
				else if ((key == KEY_DOWN) && browsingHistory)
				{
					if ((u8)(historyPos + 1) < s_HistoryCount)
					{
						historyPos++;
						CopyText(buf, s_History[historyPos], max);
					}
					else
					{
						historyPos = s_HistoryCount;
						browsingHistory = FALSE;
						buf[0] = 0;
					}
				}
				len = TextLength(buf);
				if (len > limit)
				{
					buf[limit] = 0;
					len = limit;
				}
				cursorPos = len;
				RedrawLine(buf, len, oldLen, startX, cursorPos);
			}
			continue;
		}

		if ((key == KEY_TAB) && complete)
		{
			if (!tabActive)
			{
				u8 commandLen = FirstTokenLength(buf);
				if (cursorPos > commandLen)
					continue;
				tabPrefixLen = cursorPos;
				for (u8 i = 0; i < tabPrefixLen; i++)
					tabPrefix[i] = buf[i];
				tabPrefix[tabPrefixLen] = 0;
				tabIndex = 0;
				tabActive = TRUE;
			}

			u8 matchCount = 0;
			c8 candidate[CON_COLS];
			u8 candidateLen = complete(tabPrefix, tabPrefixLen, tabIndex,
				candidate, CON_COLS, &matchCount);
			if (!candidateLen)
			{
				Console_NewLine();
				Console_Print("*** Nenhum comando correspondente\n.");
				startX = s_CurX;
				limit = CON_COLS - 1 - startX;
				if (limit > max - 1)
					limit = max - 1;
				RedrawLine(buf, len, len, startX, cursorPos);
				tabActive = FALSE;
				continue;
			}

			u8 tokenEnd = FirstTokenLength(buf);
			u8 tailLen = len - tokenEnd;
			bool addSpace = (matchCount == 1) && (tailLen == 0);
			u8 newLen = candidateLen + tailLen + (addSpace ? 1 : 0);
			if (newLen > limit)
			{
				Console_NewLine();
				Console_Print("*** Linha cheia\n.");
				startX = s_CurX;
				limit = CON_COLS - 1 - startX;
				if (limit > max - 1)
					limit = max - 1;
				RedrawLine(buf, len, len, startX, cursorPos);
				tabActive = FALSE;
				continue;
			}

			for (u8 i = 0; i < tailLen; i++)
				completionTail[i] = buf[tokenEnd + i];
			for (u8 i = 0; i < candidateLen; i++)
				buf[i] = candidate[i];
			if (addSpace)
				buf[candidateLen] = ' ';
			for (u8 i = 0; i < tailLen; i++)
				buf[candidateLen + (addSpace ? 1 : 0) + i] = completionTail[i];
			buf[newLen] = 0;
			{
				u8 oldLen = len;
				len = newLen;
				cursorPos = candidateLen + (addSpace ? 1 : 0);
				RedrawLine(buf, len, oldLen, startX, cursorPos);
			}
			tabIndex++;
			continue;
		}

		tabActive = FALSE;
		browsingHistory = FALSE;
		historyPos = s_HistoryCount;
		u8 oldLen = len;
		if (key == KEY_LEFT)
		{
			if (cursorPos > 0)
				cursorPos--;
		}
		else if (key == KEY_RIGHT)
		{
			if (cursorPos < len)
				cursorPos++;
		}
		else if (key == KEY_BS)
		{
			if (cursorPos > 0)
			{
				for (u8 i = cursorPos - 1; i < len - 1; i++)
					buf[i] = buf[i + 1];
				len--;
				cursorPos--;
				buf[len] = 0;
			}
		}
		else if (key == KEY_DEL)
		{
			if (cursorPos < len)
			{
				for (u8 i = cursorPos; i < len - 1; i++)
					buf[i] = buf[i + 1];
				len--;
				buf[len] = 0;
			}
		}
		else if (((u8)key >= ' ') && ((u8)key < KEY_DEL) && (len < limit))
		{
			for (u8 i = len; i > cursorPos; i--)
				buf[i] = buf[i - 1];
			buf[cursorPos++] = key;
			len++;
			buf[len] = 0;
		}
		RedrawLine(buf, len, oldLen, startX, cursorPos);
	}

	buf[len] = 0;
	StoreHistory(buf);
	Console_NewLine();
	return len;
}
