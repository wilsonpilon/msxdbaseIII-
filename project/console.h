#ifndef CONSOLE_H
#define CONSOLE_H

#include "msxgl.h"

// Console de texto 80x24: video direto no VDP (Text2), teclado pela BIOS. Nao usa o console do MSX-DOS.

#define CON_COLS 80
#define CON_ROWS 24

typedef u8 (*ConsoleCompleteFn)(const c8* prefix, u8 prefixLen, u8 matchIndex,
	 c8* result, u8 resultMax, u8* matchCount);

#define CONSOLE_EDIT_FIELD_FULL 0x80

// Inicializa o VDP, a fonte e limpa a tela
void Console_Init(void);

// Restaura o estado da BIOS alterado por Console_Init (chamar antes de sair)
void Console_Done(void);

// Define cor do texto e do fundo/borda (0 a 15, paleta MSX)
void Console_SetColor(u8 fg, u8 bg);

// Consulta as cores atuais para restaurar a console apos uma janela.
void Console_GetColor(u8* fg, u8* bg);

// Limpa a tela e leva o cursor para (0,0)
void Console_Clear(void);

// Move a posicao de escrita sem alterar o conteudo da tela
void Console_SetPos(u8 x, u8 y);

// Instala glyphs de 8x8 em codigos de padrao livres da fonte atual.
void Console_LoadGlyphs(u8 firstShape, const u8* patterns, u8 count);

// Escreve texto ou codigos de padrao em coordenadas, com um flush VDP por trecho.
void Console_DrawTextAt(u8 x, u8 y, const c8* text, u8 len);
void Console_DrawShapesAt(u8 x, u8 y, const u8* shapes, u8 len);
void Console_ScrollRows(u8 y, u8 height, bool up);

// Escreve um caractere na posicao do cursor ('\n' = nova linha)
void Console_PutChar(c8 chr);

// Escreve uma string terminada em zero
void Console_Print(const c8* str);

// Move o cursor para o inicio da proxima linha (rola a tela se preciso)
void Console_NewLine(void);

// Escreve um numero sem sinal em decimal
void Console_PrintNum(u32 value);

// Escreve len bytes de data (nao precisa de terminador)
void Console_PrintN(const u8* data, u8 len);

// Espera uma tecla (sem eco) e a retorna. Setas: 0x1C a 0x1F; ESC: 0x1B
c8 Console_ReadKey(void);

// Le uma linha com edicao, historico Up/Down e completamento via TAB.
// Retorna o tamanho; buf termina em zero.
u8 Console_ReadLine(c8* buf, u8 max, ConsoleCompleteFn complete);

// Edita um campo com capacidade e largura visivel separadas.
// Retorna a tecla de navegacao/saida ou CONSOLE_EDIT_FIELD_FULL em auto-advance.
u8 Console_EditField(c8* buf, u8* length, u8 maxLen, u8 visibleWidth,
	u8 x, u8 y, bool autoAdvance);

#endif
