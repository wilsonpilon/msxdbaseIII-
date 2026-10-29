#include "msxgl.h"
#include "console.h"
#include "commands.h"
#include "prompt.h"

#define LINE_MAX 80

static c8 s_Line[LINE_MAX];

void Prompt_Run(void)
{
	Command_Init();

	Console_Print("MSX dBASE II+  (clean-room)\n");
	Console_Print("Digite HELP para ajuda, QUIT para sair.\n\n");

	for (;;)
	{
		Console_PutChar('.');                 // prompt classico do dBASE
		Console_ReadLine(s_Line, LINE_MAX, Command_Complete);
		if (!Command_Execute(s_Line))
			break;
	}

	Command_Done();
}
