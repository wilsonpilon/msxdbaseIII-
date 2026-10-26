#include "msxgl.h"

void main()
{
	if (BIOS_GetMSXVersion() < MSXVER_2)
	{
		VDP_SetMode(VDP_MODE_TEXT1);
		VDP_SetColor(0xF1);
		Print_SetColor(0x0F, 0x01);

		Print_SetPosition(0, 0);
		Print_DrawText("MSX dBASE II+");
		Print_SetPosition(0, 2);
		Print_DrawText("Error: MSX2 or higher is required.");
		Print_SetPosition(0, 4);
		Print_DrawText("Press ESC to exit.");

		while (!Keyboard_IsKeyPressed(KEY_ESC))
			Halt();

		BIOS_Exit(1);
	}

	VDP_SetMode(VDP_MODE_TEXT2);
	VDP_SetColor(0xF0);
	Print_SetColor(0x0F, 0x00);

	Print_SetPosition(0, 0);
	Print_DrawText("MSX dBASE II+");
	Print_SetPosition(0, 1);
	Print_DrawText("Clean-room database environment");

	Print_SetPosition(0, 3);
	Print_DrawText("Target: MSX-DOS 2 | MSX 2 / 2+");
	Print_SetPosition(0, 5);
	Print_DrawText("Phase 1: CLI and dBASE II compatibility");

	Print_SetPosition(0, 23);
	Print_DrawText("ESC: exit");

	while (!Keyboard_IsKeyPressed(KEY_ESC))
		Halt();

	BIOS_Exit(0);
}
