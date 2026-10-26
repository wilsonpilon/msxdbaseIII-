#ifndef PLATFORM_H
#define PLATFORM_H

#include "msxgl.h"

// Camada de plataforma: unico lugar que conhece MSX-DOS e Memory Mapper.
// A engine DBF e o interpretador so devem usar estas funcoes (Marco 0).

//-----------------------------------------------------------------------------
// Informacoes do sistema
//-----------------------------------------------------------------------------

typedef struct PlatformInfo
{
	u8  DosKernel;      // versao maior do kernel MSX-DOS (1 ou 2)
	u8  DosKernelMinor; // versao menor do kernel
	bool HasMapper;     // Memory Mapper detectado
	u16 MapperKB;       // RAM mapeada total, em KB (0 se nao houver)
	u16 MapperFreeKB;   // RAM mapeada livre, em KB
} PlatformInfo;

extern PlatformInfo g_Platform;

// Detecta MSX-DOS e Memory Mapper. Chamar uma vez na inicializacao.
void Platform_Detect(void);

//-----------------------------------------------------------------------------
// Arquivos (por handle do MSX-DOS 2)
//-----------------------------------------------------------------------------

#define PFILE_INVALID 0xFF
#define PFILE_ERROR   0xFFFF

// Abre um arquivo existente somente para leitura. Retorna PFILE_INVALID se falhar.
u8 PFile_OpenRead(const c8* path);

// Fecha o arquivo.
void PFile_Close(u8 file);

// Posiciona o ponteiro a partir do inicio. Retorna FALSE em caso de erro.
bool PFile_Seek(u8 file, u32 offset);

// Le ate size bytes. Retorna os bytes lidos (0 = fim do arquivo) ou PFILE_ERROR.
u16 PFile_Read(u8 file, void* buffer, u16 size);

// Tamanho do arquivo em bytes (preserva a posicao atual).
u32 PFile_Size(u8 file);

#endif
