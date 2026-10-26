#ifndef CONSOLE_H
#define CONSOLE_H

#include "msxgl.h"

// Console de texto 80x24: video direto no VDP (Text2), teclado pela BIOS. Nao usa o console do MSX-DOS.

#define CON_COLS 80
#define CON_ROWS 24

// Inicializa o VDP, a fonte e limpa a tela
void Console_Init(void);

// Restaura o estado da BIOS alterado por Console_Init (chamar antes de sair)
void Console_Done(void);

// Define cor do texto e do fundo/borda (0 a 15, paleta MSX)
void Console_SetColor(u8 fg, u8 bg);

// Limpa a tela e leva o cursor para (0,0)
void Console_Clear(void);

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

// Le uma linha com edicao (Backspace). Retorna o tamanho; buf termina em zero
u8 Console_ReadLine(c8* buf, u8 max);

#endif
