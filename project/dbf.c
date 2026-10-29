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
#define DBF_PATH_MAX     80
#define DBF_COPY_SIZE    32

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

static u8 AppendRecord(Dbf* db, const c8* path, const u8* record)
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
	u8 padding[16];
	for (u8 i = 0; i < sizeof(padding); i++)
		padding[i] = 0xFF;

	if (!PFile_Seek(writeHandle, recordOffset)
		|| (PFile_Write(writeHandle, record, db->RecLen) != db->RecLen))
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
		db->Buffer[i] = record[i];
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

u8 Dbf_AppendRecord(Dbf* db, const c8* path, const u8* record)
{
	return AppendRecord(db, path, record);
}

u8 Dbf_AppendBlank(Dbf* db, const c8* path)
{
	if (!Dbf_IsOpen(db))
		return DBF_ERR_OPEN;
	if (db->RecLen > DBF_MAX_RECLEN)
		return DBF_ERR_LIMITS;
	u8 record[DBF_MAX_RECLEN];
	for (u16 i = 0; i < db->RecLen; i++)
		record[i] = ' ';
	return AppendRecord(db, path, record);
}

static bool IsCurrentRecord(const Dbf* db)
{
	return Dbf_IsOpen(db) && !db->Bof && !db->Eof
		&& (db->RecNo >= 1) && (db->RecNo <= db->RecCount);
}

u8 Dbf_ReplaceField(Dbf* db, const c8* path, u8 fieldIndex, const u8* value, u8 valueLen)
{
	if (!IsCurrentRecord(db))
		return DBF_ERR_RANGE;
	if ((db->Version != DBF_VER_II) || (fieldIndex >= db->FieldCount))
		return DBF_ERR_WRITE;

	DbfField* field = &db->Fields[fieldIndex];
	if ((field->Type != 'C') && (field->Type != 'N') && (field->Type != 'L'))
		return DBF_ERR_WRITE;
	if ((valueLen == 0) || (valueLen > field->Length))
		return DBF_ERR_RANGE;

	u8 replacement[DBF_MAX_RECLEN];
	for (u8 i = 0; i < field->Length; i++)
		replacement[i] = ' ';

	if (field->Type == 'L')
	{
		if (valueLen != 1)
			return DBF_ERR_FORMAT;
		c8 c = (c8)value[0];
		if ((c >= 'a') && (c <= 'z'))
			c -= 'a' - 'A';
		if ((c != 'T') && (c != 'Y') && (c != 'F') && (c != 'N'))
			return DBF_ERR_FORMAT;
		replacement[0] = ((c == 'T') || (c == 'Y')) ? 'T' : 'F';
	}
	else if (field->Type == 'N')
	{
		bool hasDigit = FALSE;
		bool hasDecimal = FALSE;
		u8 decimals = 0;
		for (u8 i = 0; i < valueLen; i++)
		{
			c8 c = (c8)value[i];
			if ((c >= '0') && (c <= '9'))
			{
				hasDigit = TRUE;
				if (hasDecimal)
					decimals++;
			}
			else if ((c == '-') && (i == 0))
				continue;
			else if ((c == '.') && !hasDecimal)
				hasDecimal = TRUE;
			else
				return DBF_ERR_FORMAT;
		}
		if (!hasDigit || (decimals > field->Decimals))
			return DBF_ERR_FORMAT;
		for (u8 i = 0; i < valueLen; i++)
			replacement[field->Length - valueLen + i] = value[i];
	}
	else
	{
		for (u8 i = 0; i < valueLen; i++)
			replacement[i] = value[i];
	}

	u8 oldValue[DBF_MAX_RECLEN];
	for (u8 i = 0; i < field->Length; i++)
	{
		oldValue[i] = db->Buffer[field->Offset + i];
		db->Buffer[field->Offset + i] = replacement[i];
	}

	u8 handle = PFile_OpenReadWrite(path);
	u32 offset = (u32)db->HeaderLen + (db->RecNo - 1) * (u32)db->RecLen + field->Offset;
	bool written = (handle != PFILE_INVALID) && PFile_Seek(handle, offset)
		&& (PFile_Write(handle, replacement, field->Length) == field->Length);
	if (!written)
	{
		if (handle != PFILE_INVALID)
		{
			if (PFile_Seek(handle, offset))
				PFile_Write(handle, oldValue, field->Length);
			PFile_Close(handle);
		}
		for (u8 i = 0; i < field->Length; i++)
			db->Buffer[field->Offset + i] = oldValue[i];
		return DBF_ERR_WRITE;
	}
	PFile_Close(handle);
	return DBF_OK;
}

u8 Dbf_SetDeleted(Dbf* db, const c8* path, bool deleted)
{
	if (!IsCurrentRecord(db))
		return DBF_ERR_RANGE;
	if (db->Version != DBF_VER_II)
		return DBF_ERR_WRITE;

	u8 oldMark = db->Buffer[0];
	u8 newMark = deleted ? '*' : ' ';
	if (oldMark == newMark)
		return DBF_OK;

	u32 offset = (u32)db->HeaderLen + (db->RecNo - 1) * (u32)db->RecLen;
	u8 handle = PFile_OpenReadWrite(path);
	bool written = (handle != PFILE_INVALID) && PFile_Seek(handle, offset)
		&& (PFile_Write(handle, &newMark, 1) == 1);
	if (!written)
	{
		if (handle != PFILE_INVALID)
		{
			if (PFile_Seek(handle, offset))
				PFile_Write(handle, &oldMark, 1);
			PFile_Close(handle);
		}
		return DBF_ERR_WRITE;
	}
	PFile_Close(handle);
	db->Buffer[0] = newMark;
	return DBF_OK;
}

static bool BuildSiblingPath(const c8* path, const c8* extension, c8* out)
{
	u8 length = 0;
	u8 lastSeparator = 0xFF;
	u8 lastDot = 0xFF;
	while (path[length] && (length < DBF_PATH_MAX))
	{
		if ((path[length] == '/') || (path[length] == '\\'))
			lastSeparator = length;
		else if (path[length] == '.')
			lastDot = length;
		out[length] = path[length];
		length++;
	}
	if (path[length] || (length == 0))
		return FALSE;

	u8 baseLength = ((lastDot != 0xFF) && ((lastSeparator == 0xFF) || (lastDot > lastSeparator)))
		? lastDot : length;
	u8 extLength = 0;
	while (extension[extLength])
		extLength++;
	if ((u16)baseLength + extLength >= DBF_PATH_MAX)
		return FALSE;
	for (u8 i = 0; i < extLength; i++)
		out[baseLength + i] = extension[i];
	out[baseLength + extLength] = 0;
	return TRUE;
}

static bool PathExists(const c8* path)
{
	u8 handle = PFile_OpenRead(path);
	if (handle == PFILE_INVALID)
		return FALSE;
	PFile_Close(handle);
	return TRUE;
}

u8 Dbf_InsertRecord(Dbf* db, const c8* path, u32 position, const u8* record)
{
	if (!Dbf_IsOpen(db))
		return DBF_ERR_OPEN;
	if (db->Version != DBF_VER_II)
		return DBF_ERR_WRITE;
	if (db->RecCount >= 0xFFFFUL)
		return DBF_ERR_LIMITS;
	if ((position < 1) || (position > db->RecCount + 1))
		return DBF_ERR_RANGE;
	if (db->RecLen > DBF_MAX_RECLEN)
		return DBF_ERR_LIMITS;
	u32 previousRecNo = db->RecNo;
	bool previousBof = db->Bof;
	bool previousEof = db->Eof;

	c8 tempPath[DBF_PATH_MAX];
	c8 backupPath[DBF_PATH_MAX];
	if (!BuildSiblingPath(path, ".TMP", tempPath)
		|| !BuildSiblingPath(path, ".BAK", backupPath))
		return DBF_ERR_RANGE;
	if (PathExists(tempPath) || PathExists(backupPath))
		return DBF_ERR_WRITE;

	u32 oldEofOffset = (u32)db->HeaderLen + db->RecCount * (u32)db->RecLen;
	u32 oldSize = PFile_Size(db->Handle);
	u8 oldEof;
	if ((oldSize <= oldEofOffset)
		|| !PFile_Seek(db->Handle, oldEofOffset)
		|| (PFile_Read(db->Handle, &oldEof, 1) != 1)
		|| (oldEof != 0x1A))
		return DBF_ERR_FORMAT;

	u16 newCount = (u16)(db->RecCount + 1);
	u32 newEofOffset = (u32)db->HeaderLen + (u32)newCount * db->RecLen;
	u32 newSize = (oldSize > newEofOffset) ? oldSize : newEofOffset + 1;
	u8 tempHandle = PFile_Create(tempPath);
	if (tempHandle == PFILE_INVALID)
		return DBF_ERR_WRITE;

	u8 copyBuffer[DBF_COPY_SIZE];
	u8 recordBuffer[DBF_MAX_RECLEN];
	u8 padding[16];
	for (u8 i = 0; i < sizeof(padding); i++)
		padding[i] = 0xFF;

	bool ok = PFile_Seek(db->Handle, 0);
	u16 headerRemaining = db->HeaderLen;
	while (ok && headerRemaining)
	{
		u16 chunk = (headerRemaining > sizeof(copyBuffer)) ? sizeof(copyBuffer) : headerRemaining;
		ok = (PFile_Read(db->Handle, copyBuffer, chunk) == chunk)
			&& (PFile_Write(tempHandle, copyBuffer, chunk) == chunk);
		headerRemaining -= chunk;
	}

	PlatformDate date;
	Platform_GetDate(&date);
	u8 headerUpdate[5];
	headerUpdate[0] = (u8)newCount;
	headerUpdate[1] = (u8)(newCount >> 8);
	headerUpdate[2] = date.Month;
	headerUpdate[3] = date.Day;
	headerUpdate[4] = date.Year;
	if (ok)
		ok = PFile_Seek(tempHandle, 1)
			&& (PFile_Write(tempHandle, headerUpdate, sizeof(headerUpdate)) == sizeof(headerUpdate))
			&& PFile_Seek(tempHandle, db->HeaderLen);

	for (u32 newRecordNo = 1; ok && (newRecordNo <= newCount); newRecordNo++)
	{
		if (newRecordNo == position)
			ok = PFile_Write(tempHandle, record, db->RecLen) == db->RecLen;
		else
		{
			u32 oldRecordNo = (newRecordNo < position) ? newRecordNo : newRecordNo - 1;
			u32 oldRecordOffset = (u32)db->HeaderLen + (oldRecordNo - 1) * (u32)db->RecLen;
			ok = PFile_Seek(db->Handle, oldRecordOffset)
				&& (PFile_Read(db->Handle, recordBuffer, db->RecLen) == db->RecLen)
				&& (PFile_Write(tempHandle, recordBuffer, db->RecLen) == db->RecLen);
		}
	}

	u8 eof = 0x1A;
	if (ok)
		ok = PFile_Write(tempHandle, &eof, 1) == 1;
	u32 paddingOffset = newEofOffset + 1;
	while (ok && (paddingOffset < newSize))
	{
		u32 remaining = newSize - paddingOffset;
		u16 chunk = (remaining > sizeof(padding)) ? sizeof(padding) : (u16)remaining;
		ok = PFile_Write(tempHandle, padding, chunk) == chunk;
		paddingOffset += chunk;
	}

	PFile_Close(tempHandle);
	if (!ok)
	{
		PFile_Delete(tempPath);
		return DBF_ERR_WRITE;
	}

	PFile_Close(db->Handle);
	db->Handle = PFILE_INVALID;
	if (!PFile_Rename(path, backupPath))
	{
		PFile_Delete(tempPath);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}
	if (!PFile_Rename(tempPath, path))
	{
		PFile_Rename(backupPath, path);
		PFile_Delete(tempPath);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}
	if (!PFile_Delete(backupPath))
	{
		bool restored = PFile_Delete(path) && PFile_Rename(backupPath, path);
		if (restored && (Dbf_Open(db, path) == DBF_OK))
		{
			if (!previousBof && !previousEof && (previousRecNo >= 1)
				&& (previousRecNo <= db->RecCount))
				Dbf_Goto(db, previousRecNo);
			else
			{
				db->RecNo = previousRecNo;
				db->Bof = previousBof;
				db->Eof = previousEof;
			}
		}
		return DBF_ERR_WRITE;
	}
	if (Dbf_Open(db, path) != DBF_OK)
	{
		Dbf_Close(db);
		PFile_Delete(path);
		PFile_Rename(backupPath, path);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}
	if (Dbf_Goto(db, position) != DBF_OK)
	{
		Dbf_Close(db);
		PFile_Delete(path);
		PFile_Rename(backupPath, path);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}

	if (Dbf_Goto(db, position) != DBF_OK)
		return DBF_ERR_WRITE;
	return DBF_OK;
}

u8 Dbf_Pack(Dbf* db, const c8* path)
{
	if (!Dbf_IsOpen(db))
		return DBF_ERR_OPEN;
	if (db->Version != DBF_VER_II)
		return DBF_ERR_WRITE;

	c8 tempPath[DBF_PATH_MAX];
	c8 backupPath[DBF_PATH_MAX];
	if (!BuildSiblingPath(path, ".TMP", tempPath)
		|| !BuildSiblingPath(path, ".BAK", backupPath))
		return DBF_ERR_RANGE;
	if (PathExists(tempPath) || PathExists(backupPath))
		return DBF_ERR_WRITE;

	u8 tempHandle = PFile_Create(tempPath);
	if (tempHandle == PFILE_INVALID)
		return DBF_ERR_WRITE;

	u8 copyBuffer[DBF_COPY_SIZE];
	u8 headerStart[8];
	bool ok = PFile_Seek(db->Handle, 0)
		&& (PFile_Read(db->Handle, headerStart, sizeof(headerStart)) == sizeof(headerStart))
		&& (PFile_Write(tempHandle, headerStart, sizeof(headerStart)) == sizeof(headerStart));

	u16 headerRemaining = DBF_II_HDR_SIZE - sizeof(headerStart);
	while (ok && headerRemaining)
	{
		u16 chunk = (headerRemaining > sizeof(copyBuffer)) ? sizeof(copyBuffer) : headerRemaining;
		ok = (PFile_Read(db->Handle, copyBuffer, chunk) == chunk)
			&& (PFile_Write(tempHandle, copyBuffer, chunk) == chunk);
		headerRemaining -= chunk;
	}

	u16 keptCount = 0;
	for (u32 n = 1; ok && (n <= db->RecCount); n++)
	{
		ok = (Dbf_Goto(db, n) == DBF_OK);
		if (ok && !Dbf_IsDeleted(db))
		{
			ok = PFile_Write(tempHandle, db->Buffer, db->RecLen) == db->RecLen;
			if (ok)
				keptCount++;
		}
	}

	PlatformDate date;
	Platform_GetDate(&date);
	u8 headerUpdate[5];
	headerUpdate[0] = (u8)keptCount;
	headerUpdate[1] = (u8)(keptCount >> 8);
	headerUpdate[2] = date.Month;
	headerUpdate[3] = date.Day;
	headerUpdate[4] = date.Year;
	u8 eof = 0x1A;
	if (ok)
		ok = PFile_Seek(tempHandle, 1)
			&& (PFile_Write(tempHandle, headerUpdate, sizeof(headerUpdate)) == sizeof(headerUpdate))
			&& PFile_Seek(tempHandle, (u32)DBF_II_HDR_SIZE + (u32)keptCount * db->RecLen)
			&& (PFile_Write(tempHandle, &eof, 1) == 1);

	PFile_Close(tempHandle);
	if (!ok)
	{
		PFile_Delete(tempPath);
		return DBF_ERR_WRITE;
	}

	PFile_Close(db->Handle);
	db->Handle = PFILE_INVALID;
	if (!PFile_Rename(path, backupPath))
	{
		PFile_Delete(tempPath);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}
	if (!PFile_Rename(tempPath, path))
	{
		PFile_Rename(backupPath, path);
		PFile_Delete(tempPath);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}

	if (Dbf_Open(db, path) != DBF_OK)
	{
		Dbf_Close(db);
		PFile_Delete(path);
		PFile_Rename(backupPath, path);
		Dbf_Open(db, path);
		return DBF_ERR_WRITE;
	}
	PFile_Delete(backupPath);
	return DBF_OK;
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
