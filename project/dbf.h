#ifndef DBF_H
#define DBF_H

#include "msxgl.h"

// Leitor DBF sem dependencias de UI ou MSX-DOS; usa apenas platform_*.
// Le dBASE II MSX e mantem dBASE III para fixtures de desenvolvimento.

#define DBF_MAX_FIELDS  32   // campos por arquivo (dBASE III permite 128)
#define DBF_MAX_RECLEN  256  // tamanho maximo do registro, em bytes (buffer estatico)
#define DBF_NAME_LEN    11   // 10 caracteres + terminador

// Codigos de erro
enum DbfError
{
	DBF_OK = 0,
	DBF_ERR_OPEN,        // arquivo nao encontrado / nao abre
	DBF_ERR_READ,        // falha ou fim inesperado na leitura
	DBF_ERR_FORMAT,      // cabecalho invalido ou versao nao suportada
	DBF_ERR_LIMITS,      // excede DBF_MAX_FIELDS ou DBF_MAX_RECLEN
	DBF_ERR_RANGE,       // numero de registro fora do arquivo
	DBF_ERR_WRITE,       // falha ao gravar ou formato sem suporte a escrita
};

typedef struct DbfField
{
	c8  Name[DBF_NAME_LEN];
	c8  Type;       // C, N, L, D, M
	u8  Length;
	u8  Decimals;
	u16 Offset;     // posicao dentro do registro (1 = apos a marca de exclusao)
} DbfField;

typedef struct Dbf
{
	u8  Handle;                    // PFILE_INVALID quando fechado
	u8  Version;
	u32 RecCount;
	u16 HeaderLen;
	u16 RecLen;
	u8  FieldCount;
	u8  LastUpdate[3];             // ano (desde 1900), mes, dia
	DbfField Fields[DBF_MAX_FIELDS];

	u32 RecNo;                     // registro atual (1..RecCount); 0 = nenhum
	bool Bof;
	bool Eof;
	u8  Buffer[DBF_MAX_RECLEN];    // registro atual; Buffer[0] = marca de exclusao
} Dbf;

// Inicializa a estrutura como "fechada"
void Dbf_Init(Dbf* db);

// Abre o arquivo, le o cabecalho e posiciona no registro 1 (ou EOF se vazio)
u8 Dbf_Open(Dbf* db, const c8* path);

// Fecha o arquivo (seguro chamar em arquivo ja fechado)
void Dbf_Close(Dbf* db);

// Indica se ha um arquivo aberto
bool Dbf_IsOpen(const Dbf* db);

// Vai para o registro n (1..RecCount) e le seus dados
u8 Dbf_Goto(Dbf* db, u32 n);

// Acrescenta um registro em branco em DBF II MSX.
u8 Dbf_AppendBlank(Dbf* db, const c8* path);

// Acrescenta um registro preparado em DBF II MSX.
u8 Dbf_AppendRecord(Dbf* db, const c8* path, const u8* record);

// Insere um registro preparado em uma posicao absoluta (base 1) em DBF II MSX.
u8 Dbf_InsertRecord(Dbf* db, const c8* path, u32 position, const u8* record);

// Atualiza um campo do registro atual em DBF II MSX.
u8 Dbf_ReplaceField(Dbf* db, const c8* path, u8 fieldIndex, const u8* value, u8 valueLen);

// Marca ou recupera o registro atual em DBF II MSX.
u8 Dbf_SetDeleted(Dbf* db, const c8* path, bool deleted);

// Remove fisicamente registros excluidos usando um arquivo temporario.
u8 Dbf_Pack(Dbf* db, const c8* path);

// Move n registros (pode ser negativo). Passar do fim deixa Eof, antes do inicio Bof
u8 Dbf_Skip(Dbf* db, i32 n);

// Registro atual marcado como excluido?
bool Dbf_IsDeleted(const Dbf* db);

// Dados do campo i no registro atual (nao terminado em zero; tamanho = Fields[i].Length)
const u8* Dbf_FieldData(const Dbf* db, u8 i);

#endif
