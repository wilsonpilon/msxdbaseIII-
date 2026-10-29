#ifndef COMMANDS_H
#define COMMANDS_H

#include "msxgl.h"

// Interpretador de comandos do prompt (Marco 1: comandos de navegacao e listagem).
// Prepara o estado do interpretador (nenhum banco aberto)
void Command_Init(void);

// Fecha os arquivos abertos
void Command_Done(void);

// Executa uma linha digitada. Retorna FALSE quando o usuario pediu para sair (QUIT).
bool Command_Execute(const c8* line);

// Completa ciclicamente o primeiro token com nomes de comandos.
u8 Command_Complete(const c8* prefix, u8 prefixLen, u8 matchIndex,
	 c8* result, u8 resultMax, u8* matchCount);

#endif
