#include "msxgl.h"
#include "console.h"
#include "platform.h"
#include "prompt.h"

void main()
{
	Platform_Detect();
	Console_Init();
	Prompt_Run();
	Console_Done();
	BIOS_Exit(0);
}
