#include "console.h"
#include "bios.h"
#include "font/font_mgl_std0.h"

// Video: acesso direto ao VDP, sem BIOS de tela nem MSX-DOS.
// Teclado: BIOS (CHSNS/CHGET via CALSLT), sem funcoes de console do MSX-DOS.

#define KEY_CR   0x0D
#define KEY_BS   0x08
#define KEY_DEL  0x7F

#define CELLS    (CON_COLS * CON_ROWS)

// Posicao do cursor, em colunas/linhas de texto
static u8 s_CurX;
static u8 s_CurY;

// Copia em RAM da tabela de nomes do VDP (codigo do padrao de cada celula).
// Permite rolar a tela na RAM e enviar tudo ao VDP numa unica escrita.
static u8 s_Shadow[CELLS];

// Faixa ASCII da fonte embutida do MSXgl.
static u8 s_FontFirst;
static u8 s_FontLast;

// Cursor da BIOS: desligado enquanto o programa roda
static u8 s_OldCursorSw;

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
}

void Console_Done(void)
{
	*(volatile u8*)M_CSRSW = s_OldCursorSw;
}

void Console_SetColor(u8 fg, u8 bg)
{
	VDP_SetColor2(bg & 0x0F, fg & 0x0F);      // registrador 7: texto | fundo/borda
}

void Console_Clear(void)
{
	Mem_Set(0, s_Shadow, CELLS);
	VDP_FillVRAM_16K(0, g_ScreenLayoutLow, CELLS);
	s_CurX = 0;
	s_CurY = 0;
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

u8 Console_ReadLine(c8* buf, u8 max)
{
	u8 len = 0;
	u8 startX = s_CurX;
	// A edicao nao quebra linha: limita ao que cabe ate o fim da tela
	u8 limit = CON_COLS - 1 - startX;
	if (limit > max - 1)
		limit = max - 1;

	for (;;)
	{
		PutShape(s_CurX, s_CurY, ShapeOf('_'));   // cursor proprio
		c8 key = Console_ReadKey();
		PutShape(s_CurX, s_CurY, 0);              // apaga o cursor

		if (key == KEY_CR)
			break;
		if ((key == KEY_BS) || (key == KEY_DEL))
		{
			if (len > 0)
			{
				len--;
				s_CurX--;
			}
		}
		else if (((u8)key >= ' ') && ((u8)key < KEY_DEL) && (len < limit))
		{
			buf[len++] = key;
			PutShape(s_CurX, s_CurY, ShapeOf(key));
			s_CurX++;
		}
	}
	buf[len] = 0;
	Console_NewLine();
	return len;
}
