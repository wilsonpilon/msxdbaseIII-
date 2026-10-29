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

void Platform_GetDate(PlatformDate* date)
{
	const DOS_Time* now = DOS_GetTime();
	date->Year = (u8)(now->Year % 100);
	date->Month = now->Month;
	date->Day = now->Date;
}

u8 PFile_OpenRead(const c8* path)
{
	return DOS_OpenHandle(path, O_RDONLY);
}

u8 PFile_OpenReadWrite(const c8* path)
{
	return DOS_OpenHandle(path, O_RDWR);
}

u8 PFile_Create(const c8* path)
{
	return DOS_CreateHandle(path, O_RDWR, 0);
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
	u16 bytesRead = DOS_ReadHandle(file, buffer, size);
#if DOS_USE_VALIDATOR
	if ((bytesRead == PFILE_ERROR) && (DOS_GetLastError() == DOS_ERR_EOF))
		return 0;
#endif
	return bytesRead;
}

u16 PFile_Write(u8 file, const void* buffer, u16 size)
{
	return DOS_WriteHandle(file, buffer, size);
}

u32 PFile_Size(u8 file)
{
	u32 cur = DOS_SeekHandle(file, 0, SEEK_CUR);
	u32 size = DOS_SeekHandle(file, 0, SEEK_END);
	DOS_SeekHandle(file, (i32)cur, SEEK_SET);
	return size;
}

bool PFile_Rename(const c8* path, const c8* newPath)
{
	return DOS_Rename(path, newPath) == DOS_ERR_NONE;
}

bool PFile_Delete(const c8* path)
{
	return DOS_Delete(path) == DOS_ERR_NONE;
}

static bool CopyFindEntry(const DOS_FIB* fib, PlatformFileEntry* entry)
{
	if (!fib)
		return FALSE;
	u8 i = 0;
	while (fib->Filename[i] && (i < sizeof(entry->Name) - 1))
	{
		entry->Name[i] = fib->Filename[i];
		i++;
	}
	entry->Name[i] = 0;
	entry->Size = fib->Size;
	entry->Year = DOS_GetFileYear(fib);
	entry->Month = DOS_GetFileMonth(fib);
	entry->Day = DOS_GetFileDay(fib);
	return TRUE;
}

bool PFile_FindFirst(const c8* pattern, PlatformFileEntry* entry)
{
	return CopyFindEntry(DOS_FindFirstEntry(pattern, 0), entry);
}

bool PFile_FindNext(PlatformFileEntry* entry)
{
	return CopyFindEntry(DOS_FindNextEntry(), entry);
}
