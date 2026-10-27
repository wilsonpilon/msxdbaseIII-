#include "dbf.h"
#include "platform.h"

// Layout do cabecalho dBASE III (32 bytes):
//   0      versao (0x03 = sem memo, 0x83 = com memo)
//   1..3   ultima atualizacao (AA MM DD)
//   4..7   numero de registros (u32, little-endian)
//   8..9   tamanho do cabecalho (u16)
//   10..11 tamanho do registro (u16)
// Seguem descritores de campo de 32 bytes, terminados por 0x0D:
//   0..10 nome, 11 tipo, 16 tamanho, 17 decimais
#define DBF_HDR_SIZE     32
#define DBF_FIELD_SIZE   32
#define DBF_II_HDR_SIZE  521
#define DBF_II_FIELD_SIZE 16
#define DBF_VER_II       0x02
#define DBF_VER_III      0x03
#define DBF_VER_III_MEMO 0x83
#define DBF_HDR_END      0x0D

static u16 Rd16(const u8* p)
{
	return (u16)p[0] | ((u16)p[1] << 8);
}

static u32 Rd32(const u8* p)
{
	return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

void Dbf_Init(Dbf* db)
{
	db->Handle = PFILE_INVALID;
	db->Version = 0;
	db->RecCount = 0;
	db->FieldCount = 0;
	db->RecNo = 0;
	db->Bof = TRUE;
	db->Eof = TRUE;
}

bool Dbf_IsOpen(const Dbf* db)
{
	return db->Handle != PFILE_INVALID;
}

void Dbf_Close(Dbf* db)
{
	if (Dbf_IsOpen(db))
		PFile_Close(db->Handle);
	Dbf_Init(db);
}

// Le o cabecalho e os descritores. Em erro, o chamador fecha o arquivo.
static u8 ReadHeader(Dbf* db)
{
	u8 hdr[DBF_HDR_SIZE];
	u8 desc[DBF_FIELD_SIZE];

	if (PFile_Read(db->Handle, hdr, 1) != 1)
		return DBF_ERR_READ;
	db->Version = hdr[0];

	if (hdr[0] == DBF_VER_II)
	{
		if (PFile_Read(db->Handle, hdr + 1, 7) != 7)
			return DBF_ERR_READ;

		db->RecCount = Rd16(hdr + 1);
		db->LastUpdate[0] = hdr[5];
		db->LastUpdate[1] = hdr[3];
		db->LastUpdate[2] = hdr[4];
		db->RecLen = Rd16(hdr + 6);
		db->HeaderLen = DBF_II_HDR_SIZE;

		if ((db->RecLen < 2) || (db->RecLen > DBF_MAX_RECLEN))
			return DBF_ERR_LIMITS;

		u16 offset = 1;
		db->FieldCount = 0;
		for (u8 i = 0; i < DBF_MAX_FIELDS; i++)
		{
			if (PFile_Read(db->Handle, desc, DBF_II_FIELD_SIZE) != DBF_II_FIELD_SIZE)
				return DBF_ERR_READ;
			if (desc[0] == DBF_HDR_END)
				break;

			DbfField* f = &db->Fields[db->FieldCount++];
			for (u8 j = 0; j < DBF_NAME_LEN - 1; j++)
				f->Name[j] = desc[j];
			f->Name[DBF_NAME_LEN - 1] = 0;
			f->Type = desc[11];
			f->Length = desc[12];
			f->Decimals = desc[15];
			f->Offset = offset;
			offset += f->Length;
		}

		if (!PFile_Seek(db->Handle, DBF_II_HDR_SIZE - 1))
			return DBF_ERR_READ;
		if (PFile_Read(db->Handle, desc, 1) != 1)
			return DBF_ERR_READ;
		if (((db->FieldCount == DBF_MAX_FIELDS) && (desc[0] != DBF_HDR_END))
			|| ((db->FieldCount < DBF_MAX_FIELDS) && (desc[0] != 0)))
			return DBF_ERR_FORMAT;
		if (offset != db->RecLen)
			return DBF_ERR_FORMAT;
		return DBF_OK;
	}

	if ((hdr[0] != DBF_VER_III) && (hdr[0] != DBF_VER_III_MEMO))
		return DBF_ERR_FORMAT;
	if (PFile_Read(db->Handle, hdr + 1, DBF_HDR_SIZE - 1) != DBF_HDR_SIZE - 1)
		return DBF_ERR_READ;

	db->LastUpdate[0] = hdr[1];
	db->LastUpdate[1] = hdr[2];
	db->LastUpdate[2] = hdr[3];
	db->RecCount = Rd32(hdr + 4);
	db->HeaderLen = Rd16(hdr + 8);
	db->RecLen = Rd16(hdr + 10);

	if ((db->RecLen < 2) || (db->RecLen > DBF_MAX_RECLEN))
		return DBF_ERR_LIMITS;

	// Descritores de campo ate o byte 0x0D
	u16 offset = 1; // byte 0 do registro e a marca de exclusao
	db->FieldCount = 0;
	for (;;)
	{
		if (PFile_Read(db->Handle, desc, 1) != 1)
			return DBF_ERR_READ;
		if (desc[0] == DBF_HDR_END)
			break;
		if (PFile_Read(db->Handle, desc + 1, DBF_FIELD_SIZE - 1) != DBF_FIELD_SIZE - 1)
			return DBF_ERR_READ;
		if (db->FieldCount >= DBF_MAX_FIELDS)
			return DBF_ERR_LIMITS;

		DbfField* f = &db->Fields[db->FieldCount++];
		for (u8 i = 0; i < DBF_NAME_LEN - 1; i++)
			f->Name[i] = desc[i];
		f->Name[DBF_NAME_LEN - 1] = 0;
		f->Type = desc[11];
		f->Length = desc[16];
		f->Decimals = desc[17];
		f->Offset = offset;
		offset += f->Length;
	}

	// Os campos devem preencher exatamente o registro
	if (offset != db->RecLen)
		return DBF_ERR_FORMAT;
	return DBF_OK;
}

u8 Dbf_Open(Dbf* db, const c8* path)
{
	Dbf_Close(db);

	db->Handle = PFile_OpenRead(path);
	if (db->Handle == PFILE_INVALID)
		return DBF_ERR_OPEN;

	u8 err = ReadHeader(db);
	if (err != DBF_OK)
	{
		Dbf_Close(db);
		return err;
	}

	if (db->RecCount == 0)
	{
		db->RecNo = 0;
		db->Bof = TRUE;
		db->Eof = TRUE;
		return DBF_OK;
	}
	return Dbf_Goto(db, 1);
}

u8 Dbf_Goto(Dbf* db, u32 n)
{
	if ((n < 1) || (n > db->RecCount))
		return DBF_ERR_RANGE;

	u32 pos = (u32)db->HeaderLen + (n - 1) * (u32)db->RecLen;
	if (!PFile_Seek(db->Handle, pos))
		return DBF_ERR_READ;
	if (PFile_Read(db->Handle, db->Buffer, db->RecLen) != db->RecLen)
		return DBF_ERR_READ;

	db->RecNo = n;
	db->Bof = FALSE;
	db->Eof = FALSE;
	return DBF_OK;
}

u8 Dbf_AppendBlank(Dbf* db, const c8* path)
{
	if (!Dbf_IsOpen(db))
		return DBF_ERR_OPEN;
	if (db->Version != DBF_VER_II)
		return DBF_ERR_WRITE;
	if (db->RecCount >= 0xFFFFUL)
		return DBF_ERR_LIMITS;

	u32 recordOffset = (u32)db->HeaderLen + db->RecCount * (u32)db->RecLen;
	u8 writeHandle = PFile_OpenReadWrite(path);
	if (writeHandle == PFILE_INVALID)
		return DBF_ERR_WRITE;

	u8 oldEof;
	bool headerWritten;
	if (!PFile_Seek(writeHandle, recordOffset)
		|| (PFile_Read(writeHandle, &oldEof, 1) != 1)
		|| (oldEof != 0x1A))
	{
		PFile_Close(writeHandle);
		return DBF_ERR_FORMAT;
	}

	PlatformDate date;
	Platform_GetDate(&date);
	u8 blankRecord[DBF_MAX_RECLEN];
	u8 padding[16];
	for (u16 i = 0; i < db->RecLen; i++)
		blankRecord[i] = ' ';
	for (u8 i = 0; i < sizeof(padding); i++)
		padding[i] = 0xFF;

	if (!PFile_Seek(writeHandle, recordOffset)
		|| (PFile_Write(writeHandle, blankRecord, db->RecLen) != db->RecLen))
		goto restore_eof;
	if (PFile_Write(writeHandle, "\x1A", 1) != 1)
		goto restore_eof;

	u32 physicalSize = PFile_Size(writeHandle);
	u32 paddingOffset = recordOffset + db->RecLen + 1;
	while (paddingOffset < physicalSize)
	{
		u32 remaining = physicalSize - paddingOffset;
		u16 chunkSize = (remaining > sizeof(padding)) ? sizeof(padding) : (u16)remaining;
		if (!PFile_Seek(writeHandle, paddingOffset)
			|| (PFile_Write(writeHandle, padding, chunkSize) != chunkSize))
			goto restore_eof;
		paddingOffset += chunkSize;
	}

	u16 newCount = (u16)(db->RecCount + 1);
	u8 headerUpdate[5];
	u8 oldHeader[5];
	headerUpdate[0] = (u8)newCount;
	headerUpdate[1] = (u8)(newCount >> 8);
	headerUpdate[2] = date.Month;
	headerUpdate[3] = date.Day;
	headerUpdate[4] = date.Year;
	oldHeader[0] = (u8)db->RecCount;
	oldHeader[1] = (u8)(db->RecCount >> 8);
	oldHeader[2] = db->LastUpdate[1];
	oldHeader[3] = db->LastUpdate[2];
	oldHeader[4] = db->LastUpdate[0];
	headerWritten = PFile_Seek(writeHandle, 1);
	if (headerWritten)
		headerWritten = PFile_Write(writeHandle, headerUpdate, sizeof(headerUpdate)) == sizeof(headerUpdate);
	if (!headerWritten)
	{
		if (PFile_Seek(writeHandle, 1))
			PFile_Write(writeHandle, oldHeader, sizeof(oldHeader));
		goto restore_eof;
	}

	PFile_Close(writeHandle);
	db->RecCount = newCount;
	db->LastUpdate[0] = date.Year;
	db->LastUpdate[1] = date.Month;
	db->LastUpdate[2] = date.Day;
	for (u16 i = 0; i < db->RecLen; i++)
		db->Buffer[i] = blankRecord[i];
	db->RecNo = newCount;
	db->Bof = FALSE;
	db->Eof = FALSE;
	return DBF_OK;

restore_eof:
	if (PFile_Seek(writeHandle, recordOffset))
		PFile_Write(writeHandle, &oldEof, 1);
	PFile_Close(writeHandle);
	return DBF_ERR_WRITE;
}

u8 Dbf_Skip(Dbf* db, i32 n)
{
	i32 target = (i32)db->RecNo + n;

	if (target > (i32)db->RecCount)
	{
		db->RecNo = db->RecCount + 1;
		db->Eof = TRUE;
		return DBF_OK;
	}
	if (target < 1)
	{
		db->RecNo = 0;
		db->Bof = TRUE;
		return DBF_OK;
	}
	return Dbf_Goto(db, (u32)target);
}

bool Dbf_IsDeleted(const Dbf* db)
{
	return db->Buffer[0] == '*';
}

const u8* Dbf_FieldData(const Dbf* db, u8 i)
{
	return db->Buffer + db->Fields[i].Offset;
}
