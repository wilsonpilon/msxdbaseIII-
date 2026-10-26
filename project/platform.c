#include "platform.h"
#include "dos.h"
#include "dos_mapper.h"

PlatformInfo g_Platform;

void Platform_Detect(void)
{
	DOS_Version ver;
	g_Platform.DosKernel = DOS_GetVersion(&ver);
	g_Platform.DosKernelMinor = (u8)(ver.Kernel & 0xFF);

	g_Platform.HasMapper = FALSE;
	g_Platform.MapperKB = 0;
	g_Platform.MapperFreeKB = 0;

	// Cada segmento do mapper tem 16 KB
	if (DOSMapper_Init())
	{
		const DOS_VarTable* vt = DOSMapper_GetVarTable();
		g_Platform.HasMapper = TRUE;
		g_Platform.MapperKB = (u16)vt->NumSeg * 16;
		g_Platform.MapperFreeKB = (u16)vt->FreeSeg * 16;
	}
}

u8 PFile_OpenRead(const c8* path)
{
	return DOS_OpenHandle(path, O_RDONLY);
}

void PFile_Close(u8 file)
{
	DOS_CloseHandle(file);
}

bool PFile_Seek(u8 file, u32 offset)
{
	DOS_ResetLastError();
	DOS_SeekHandle(file, (i32)offset, SEEK_SET);
	return g_DOS_LastError == DOS_ERR_NONE;
}

u16 PFile_Read(u8 file, void* buffer, u16 size)
{
	return DOS_ReadHandle(file, buffer, size);
}

u32 PFile_Size(u8 file)
{
	u32 cur = DOS_SeekHandle(file, 0, SEEK_CUR);
	u32 size = DOS_SeekHandle(file, 0, SEEK_END);
	DOS_SeekHandle(file, (i32)cur, SEEK_SET);
	return size;
}
