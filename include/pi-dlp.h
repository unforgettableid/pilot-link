/* 
 * pi-dlp.h: Desktop Link Protocol implementation (ala SLP)
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Library General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA. *
 */

#ifndef _PILOT_DLP_H_
#define _PILOT_DLP_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <unistd.h>

#include "pi-macros.h"		/* For recordid_t */

#define PI_DLP_OFFSET_CMD  0
#define PI_DLP_OFFSET_ARGC 1
#define PI_DLP_OFFSET_ARGV 2
#define PI_DLP_OFFSET_ARGV 2

#define PI_DLP_ARG_TINY_LEN  0x000000FFL
#define PI_DLP_ARG_SHORT_LEN 0x0000FFFFL
#define PI_DLP_ARG_LONG_LEN  0xFFFFFFFFL

#define PI_DLP_ARG_FLAG_TINY  0x00
#define PI_DLP_ARG_FLAG_SHORT 0x80
#define PI_DLP_ARG_FLAG_LONG  0xC0

#define PI_DLP_ARG_FIRST_ID 0x20

#define DLP_BUF_SIZE 0xffff

#define vfsFileAttrReadOnly     (0x00000001UL)
#define vfsFileAttrHidden       (0x00000002UL)
#define vfsFileAttrSystem       (0x00000004UL)
#define vfsFileAttrVolumeLabel  (0x00000008UL)
#define vfsFileAttrDirectory    (0x00000010UL)
#define vfsFileAttrArchive      (0x00000020UL)
#define vfsFileAttrLink         (0x00000040UL)

/* Mount/Format the volume with the filesystem specified */
#define vfsMountFlagsUseThisFileSystem	0x01

/* file type for slot driver libraries */
#define sysFileTSlotDriver	'libs'

/* The date the file was created. */
#define vfsFileDateCreated 	1

/* The date the file was last modified. */
#define vfsFileDateModified 	2

/* The date the file was last accessed. */
#define vfsFileDateAccessed 	3       	

#define vfsMAXFILENAME 256

#define vfsIteratorStart	0L
#define vfsIteratorStop		0xffffffffL

/* constant for an invalid volume reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidVolRef 0	

/* constant for an invalid file reference, guaranteed not to represent a
   valid one.  Use it like you would use NULL for a FILE*. */
#define vfsInvalidFileRef	0L

/* from the beginning (first data byte of file) */
#define vfsOriginBeginning	0

/* from the current position */
#define vfsOriginCurrent	1

/* From the end of file (one position beyond last data byte, only negative
   offsets are legally allowed) */
#define vfsOriginEnd		2

typedef unsigned long FileRef;

/* Don't let anyone else open it */
#define vfsModeExclusive	(0x0001U)

/* Open for read access */
#define vfsModeRead		(0x0002U)

/* Open for write access, implies exclusive */
#define vfsModeWrite		(0x0004U | vfsModeExclusive)

/* create the file if it doesn't already exist.  Implemented in VFS layer,
   no FS lib call will ever have to handle this. */
#define vfsModeCreate		(0x0008U)

/* Truncate file to 0 bytes after opening, removing all existing data. 
   Implemented in VFS layer, no FS lib call will ever have to handle this.
   */
#define vfsModeTruncate		(0x0010U)

/* open for read/write access */
#define vfsModeReadWrite	(vfsModeWrite | vfsModeRead)

/* Leave the file open even if when the foreground task closes */
#define vfsModeLeaveOpen	(0x0020U)

	struct VFSDirInfo {
		unsigned long attr;
		char name[vfsMAXFILENAME];
	};
	
	typedef struct VFSAnyMountParamTag {
		unsigned short volRefNum;
		unsigned short reserved;
		unsigned long  mountClass;
	} VFSAnyMountParamType;
	
	struct VFSSlotMountParamTag {
		VFSAnyMountParamType vfsMountParam;
		unsigned short slotLibRefNum;
		unsigned short slotRefNum;
	};
	
	struct VFSInfo {
		/* 0: read-only etc. */
		unsigned long   attributes;

		/* 4: Filesystem type for this volume (defined below) */
		unsigned long   fsType;

		/* 8: Creator code of filesystem driver for this volume. */
		unsigned long   fsCreator;
							
		/* For slot based filesystems: (mountClass = VFSMountClass_SlotDriver)
		   12: mount class that mounted this volume */
		unsigned long   mountClass;
		
		/* 16: Library on which the volume is mounted */
		int slotLibRefNum;

		/* 18: ExpMgr slot number of card containing volume */
		int slotRefNum;

		/* 20: Type of card media (mediaMemoryStick, mediaCompactFlash, etc.) */
		unsigned long   mediaType;

		/* 24: reserved for future use (other mountclasses may need more space) */
		unsigned long   reserved;
	};

	/* Note: All of these functions return an integer that if greater
	   then zero is the number of bytes in the result, zero if there was
	   no result, or less then zero if an error occured. Any return
	   fields will be set to zero if an error occurs. All calls to dlp_*
	   functions should check for a return value less then zero.
	 */
	struct PilotUser {
		size_t 	passwordLength;
		char 	username[128],
			password[128];
		unsigned long userID, viewerID, lastSyncPC;
		time_t successfulSyncDate, lastSyncDate;
	};

	struct SysInfo {
		unsigned long romVersion;
		unsigned long locale;
		unsigned char prodIDLength;
		char 	prodID[128];
		unsigned short dlpMajorVersion;
		unsigned short dlpMinorVersion;
		unsigned short compatMajorVersion;
		unsigned short compatMinorVersion;
		unsigned long  maxRecSize;		
	};

	struct DBInfo {
		int 	more;
		char name[34];
		unsigned int flags;
		unsigned int miscFlags;
		unsigned int version;
		unsigned long type, creator;
		unsigned long modnum;
		unsigned int index;
		time_t createDate, modifyDate, backupDate;
	};

	struct DBSizeInfo {
		unsigned long numRecords;
		unsigned long totalBytes;
		unsigned long dataBytes;
		unsigned long appBlockSize;
		unsigned long sortBlockSize;
		unsigned long maxRecSize;
	};

	struct CardInfo {
		int 	card,
			version;
			int more;

		time_t 	creation;

		unsigned long romSize;
		unsigned long ramSize;
		unsigned long ramFree;

		char 	name[128],
			manufacturer[128];


	};

	struct NetSyncInfo {
		int 	lanSync;
		char 	hostName[256];			/* Null terminated string */
		char 	hostAddress[40];		/* Null terminated string */
		char 	hostSubnetMask[40];		/* Null terminated string */
	};

	enum dlpFunctions {
		/* range reserved for internal use */
		dlpReservedFunc = 0x0F,

		/* DLP 1.0 FUNCTIONS START HERE (PalmOS v1.0) */
		dlpFuncReadUserInfo,
	
		dlpFuncWriteUserInfo,
	
		dlpFuncReadSysInfo,
	
		dlpFuncGetSysDateTime,
	
		dlpFuncSetSysDateTime,
	
		dlpFuncReadStorageInfo,
	
		dlpFuncReadDBList,
	
		dlpFuncOpenDB,
	
		dlpFuncCreateDB,
	
		dlpFuncCloseDB,
	
		dlpFuncDeleteDB,
	
		dlpFuncReadAppBlock,
	
		dlpFuncWriteAppBlock,

		dlpFuncReadSortBlock,

		dlpFuncWriteSortBlock,

		dlpFuncReadNextModifiedRec,

		dlpFuncReadRecord,

		dlpFuncWriteRecord,

		dlpFuncDeleteRecord,

		dlpFuncReadResource,

		dlpFuncWriteResource,

		dlpFuncDeleteResource,

		dlpFuncCleanUpDatabase,

		dlpFuncResetSyncFlags,

		dlpFuncCallApplication,

		dlpFuncResetSystem,
	
		dlpFuncAddSyncLogEntry,
	
		dlpFuncReadOpenDBInfo,
	
		dlpFuncMoveCategory,
	
		dlpProcessRPC,
	
		dlpFuncOpenConduit,
	
		dlpFuncEndOfSync,
	
		dlpFuncResetRecordIndex,
	
		dlpFuncReadRecordIDList,
	
		/* DLP 1.1 FUNCTIONS ADDED HERE (PalmOS v2.0 Personal, and Professional) */
		dlpFuncReadNextRecInCategory,
	
		dlpFuncReadNextModifiedRecInCategory,
	
		dlpFuncReadAppPreference,
	
		dlpFuncWriteAppPreference,
	
		dlpFuncReadNetSyncInfo,
	
		dlpFuncWriteNetSyncInfo,

		dlpFuncReadFeature,
	
		/* DLP 1.2 FUNCTIONS ADDED HERE (PalmOS v3.0) */
		dlpFuncFindDB,

		dlpFuncSetDBInfo,
	
		/* DLP 1.3 FUNCTIONS ADDED HERE (PalmOS v4.0) */
		dlpLoopBackTest,

		dlpFuncExpSlotEnumerate,

		dlpFuncExpCardPresent,

		dlpFuncExpCardInfo,

		dlpFuncVFSCustomControl,

		dlpFuncVFSGetDefaultDir,

		dlpFuncVFSImportDatabaseFromFile,

		dlpFuncVFSExportDatabaseToFile,

		dlpFuncVFSFileCreate,

		dlpFuncVFSFileOpen,

		dlpFuncVFSFileClose,

		dlpFuncVFSFileWrite,

		dlpFuncVFSFileRead,

		dlpFuncVFSFileDelete,

		dlpFuncVFSFileRename,

		dlpFuncVFSFileEOF,

		dlpFuncVFSFileTell,

		dlpFuncVFSFileGetAttributes,

		dlpFuncVFSFileSetAttributes,

		dlpFuncVFSFileGetDate,

		dlpFuncVFSFileSetDate,

		dlpFuncVFSDirCreate,

		dlpFuncVFSDirEntryEnumerate,

		dlpFuncVFSGetFile,

		dlpFuncVFSPutFile,

		dlpFuncVFSVolumeFormat,

		dlpFuncVFSVolumeEnumerate,

		dlpFuncVFSVolumeInfo,

		dlpFuncVFSVolumeGetLabel,

		dlpFuncVFSVolumeSetLabel,

		dlpFuncVFSVolumeSize,

		dlpFuncVFSFileSeek,

		dlpFuncVFSFileResize,

		dlpFuncVFSFileSize,

		dlpLastFunc
	};
	
	enum dlpDBFlags {
		dlpDBFlagResource 	= 0x0001,	/* Resource DB, instead of record DB            */
		dlpDBFlagReadOnly 	= 0x0002,	/* DB is read only                              */
		dlpDBFlagAppInfoDirty 	= 0x0004,	/* AppInfo data has been modified               */
		dlpDBFlagBackup 	= 0x0008,	/* DB is tagged for generic backup              */
		dlpDBFlagClipping 	= 0x0200,	/* DB is a Palm Query Application (PQA)         */
		dlpDBFlagOpen 		= 0x8000,	/* DB is currently open                         */

		/* v2.0 specific */
		dlpDBFlagNewer 		= 0x0010,	/* Newer version may be installed over open DB  */
		dlpDBFlagReset 		= 0x0020,	/* Reset after installation                     */

		/* v3.0 specific */
		dlpDBFlagCopyPrevention = 0x0040,	/* DB should not be beamed                      */
		dlpDBFlagStream 	= 0x0080	/* DB implements a file stream                  */
	};

	enum dlpDBMiscFlags {
		dlpDBMiscFlagExcludeFromSync = 0x80
	};

	enum dlpRecAttributes {
		dlpRecAttrDeleted 	= 0x80,		/* tagged for deletion during next sync         */
		dlpRecAttrDirty 	= 0x40,		/* record modified                              */
		dlpRecAttrBusy 		= 0x20,		/* record locked                                */
		dlpRecAttrSecret 	= 0x10,		/* record is secret                             */
		dlpRecAttrArchived 	= 0x08		/* tagged for archival during next sync         */
	};

	enum dlpOpenFlags {
		dlpOpenRead = 0x80,
		dlpOpenWrite = 0x40,
		dlpOpenExclusive = 0x20,
		dlpOpenSecret = 0x10,
		dlpOpenReadWrite = 0xC0
	};

	enum dlpEndStatus {
		dlpEndCodeNormal 	= 0,		/* Normal					 */
		dlpEndCodeOutOfMemory,			/* End due to low memory on Palm		 */
		dlpEndCodeUserCan,			/* Cancelled by user				 */
		dlpEndCodeOther				/* dlpEndCodeOther and higher == "Anything else" */
	};

	enum dlpDBList {
		dlpDBListRAM 		= 0x80,
		dlpDBListROM 		= 0x40
	};

	enum dlpFindDBOptFlags {
		dlpFindDBOptFlagGetAttributes = 0x80,
		dlpFindDBOptFlagGetSize       = 0x40,
		dlpFindDBOptFlagMaxRecSize    = 0x20
	};

	enum dlpFindDBSrchFlags {
		dlpFindDBSrchFlagNewSearch  = 0x80,
		dlpFindDBSrchFlagOnlyLatest = 0x40
	};

	enum dlpErrors {
		dlpErrNoError 		= 0,
		dlpErrSystem 		= -1,
		dlpErrIllegalReq        = -2,
		dlpErrMemory 		= -3,
		dlpErrParam 		= -4,
		dlpErrNotFound 		= -5,
		dlpErrNoneOpen 		= -6,
		dlpErrAlreadyOpen 	= -7,
		dlpErrTooManyOpen 	= -8,
		dlpErrExists 		= -9,
		dlpErrOpen 		= -10,
		dlpErrDeleted 		= -11,
		dlpErrBusy 		= -12,
		dlpErrNotSupp 		= -13,
		dlpErrUnused1 		= -14,
		dlpErrReadOnly 		= -15,
		dlpErrSpace 		= -16,
		dlpErrLimit 		= -17,
		dlpErrSync 		= -18,
		dlpErrWrapper 		= -19,
		dlpErrArgument 		= -20,
		dlpErrSize 		= -21,
		dlpErrUnknown 		= -128
	};

	struct dlpArg {
		int 	id;
		size_t	len;		
		char *data;
	};

	struct dlpRequest {
		enum dlpFunctions cmd;
		
		int argc;
		struct dlpArg **argv;
	};

	struct dlpResponse {
		enum dlpFunctions cmd;
		enum dlpErrors err;
		
		int argc;
		struct dlpArg **argv;
	};	

	extern struct dlpArg * dlp_arg_new PI_ARGS((int id, size_t len));
	extern void dlp_arg_free PI_ARGS((struct dlpArg *arg));
	extern int dlp_arg_len PI_ARGS((int argc, struct dlpArg **argv));
	extern struct dlpRequest *dlp_request_new 
	        PI_ARGS((enum dlpFunctions cmd, int argc, ...));
	extern struct dlpRequest * dlp_request_new_with_argid 
	        PI_ARGS((enum dlpFunctions cmd, int argid, int argc, ...));
	extern struct dlpResponse *dlp_response_new
	        PI_ARGS((enum dlpFunctions cmd, int argc));
	extern ssize_t dlp_response_read PI_ARGS((struct dlpResponse **res,
		int sd));
	extern ssize_t dlp_request_write PI_ARGS((struct dlpRequest *req,
		int sd));
	extern void dlp_request_free PI_ARGS((struct dlpRequest *req));
	extern void dlp_response_free PI_ARGS((struct dlpResponse *req));

	extern int dlp_exec PI_ARGS((int sd, struct dlpRequest *req,
		struct dlpResponse **res));

	extern char *dlp_errorlist[];
	extern char *dlp_strerror(int error);

	/* Get the time on the Palm and return it as a local time_t value. */ 
	extern int dlp_GetSysDateTime PI_ARGS((int sd, time_t *t));

	/* Set the time on the Palm using a local time_t value. */
	extern int dlp_SetSysDateTime PI_ARGS((int sd, time_t t));

	extern int dlp_ReadStorageInfo
		PI_ARGS((int sd, int cardno, struct CardInfo * c));

	/* Read the system information block. */
	extern int dlp_ReadSysInfo PI_ARGS((int sd, struct SysInfo * s));

	/* flags must contain dlpDBListRAM and/or dlpDBListROM */
	extern int dlp_ReadDBList
		PI_ARGS((int sd, int cardno, int flags, int start, 
			struct DBInfo * info));

	extern int dlp_FindDBInfo
		PI_ARGS((int sd, int cardno, int start, char *dbname,
			unsigned long type, unsigned long creator,
			struct DBInfo * info));

	/* Open a database on the Palm. cardno is the target memory card
	   (always use zero for now), mode is the access mode, and name is
	   the ASCII name of the database.

	   Mode can contain any and all of these values:
	   Read 	= 0x80
	   Write 	= 0x40
	   Exclusive 	= 0x20
	   ShowSecret 	= 0x10
	 */
	extern int dlp_OpenDB
		PI_ARGS((int sd, int cardno, int mode, char *name,
			int *dbhandle));

	/* Close an opened database using the handle returned by OpenDB. */
	extern int dlp_CloseDB PI_ARGS((int sd, int dbhandle));

	/* Variant of CloseDB that closes all opened databases. */
	extern int dlp_CloseDB_All PI_ARGS((int sd));

	/* Delete a database. cardno: zero for now name: ascii name of DB. */
	extern int dlp_DeleteDB
		PI_ARGS((int sd, int cardno, PI_CONST char *name));

	/* Create database */
	extern int dlp_CreateDB
		PI_ARGS((int sd, unsigned long creator, unsigned long type,
			int cardno, int flags, unsigned int version,
			PI_CONST char *name, int *dbhandle));

	/* Require reboot of Palm after HotSync terminates. */
	extern int dlp_ResetSystem PI_ARGS((int sd));

	/* Add an entry into the HotSync log on the Palm.  Move to the next
	   line with \n, as usual. You may invoke this command once or more
	   before calling EndOfSync, but it is not required.
	 */
	extern int dlp_AddSyncLogEntry PI_ARGS((int sd, char *entry));

	/* State that the conduit has been succesfully opened -- puts up a status
	   message on the Palm, no other effect as far as I know. Not required.
	 */
	extern int dlp_OpenConduit PI_ARGS((int sd));

	/* Terminate HotSync. Required at the end of a session. The pi_socket layer
	   will call this for you if you don't.

	   Status: dlpEndCodeNormal, dlpEndCodeOutOfMemory, dlpEndCodeUserCan, or
	   dlpEndCodeOther
	 */
	extern int dlp_EndOfSync PI_ARGS((int sd, int status));


	/* Terminate HotSync _without_ notifying Palm. This will cause the
	   Palm to time out, and should (if I remember right) lose any
	   changes to unclosed databases. _Never_ use under ordinary
	   circumstances. If the sync needs to be aborted in a reasonable
	   manner, use EndOfSync with a non-zero status.
	 */
	extern int dlp_AbortSync PI_ARGS((int sd));

	/* Return info about an opened database. Currently the only information
	   returned is the number of records in the database. 
	 */
	extern int dlp_ReadOpenDBInfo
		PI_ARGS((int sd, int dbhandle, int *records));

	extern int dlp_MoveCategory
		PI_ARGS((int sd, int handle, int fromcat, int tocat));

	/* Tell the pilot who it is. */
	extern int dlp_WriteUserInfo
		PI_ARGS((int sd, struct PilotUser * User));

	/* Ask the pilot who it is. */
	extern int dlp_ReadUserInfo
		PI_ARGS((int sd, struct PilotUser * User));

	/* Convenience function to reset lastSyncPC in the UserInfo to 0 */
	extern int dlp_ResetLastSyncPC PI_ARGS((int sd));

	extern int dlp_ReadAppBlock
		PI_ARGS((int sd, int fHandle, int offset, void *dbuf,
			int dlen));

	extern int dlp_WriteAppBlock
		PI_ARGS((int sd,int fHandle,PI_CONST void *dbuf,size_t dlen));

	extern int dlp_ReadSortBlock
		PI_ARGS((int sd, int fHandle, int offset, void *dbuf,
			int dlen));

	extern int dlp_WriteSortBlock
		PI_ARGS((int sd, int fHandle, PI_CONST void *dbuf,
			size_t dlen));

	/* Reset NextModified position to beginning */
	extern int dlp_ResetDBIndex PI_ARGS((int sd, int dbhandle));

	extern int dlp_ReadRecordIDList
		PI_ARGS((int sd, int dbhandle, int sort, int start, int max,
			recordid_t * IDs, int *count));

	/* Write a new record to an open database.  
	   Flags: 0 or dlpRecAttrSecret 
	   RecID: a UniqueID to use for the new record, or 0 to have the
	          Palm create an ID for you.
	   CatID: the category of the record data: the record contents
	          length: length of record.
	   If -1, then strlen will be used on data 
	   NewID: storage for returned ID, or null.  
	 */

	extern int dlp_WriteRecord
		PI_ARGS((int sd, int dbhandle, int flags, recordid_t recID,
			int catID, void *data, size_t length,
			recordid_t * NewID));

	extern int dlp_DeleteRecord
		PI_ARGS((int sd, int dbhandle, int all, recordid_t recID));

	extern int dlp_DeleteCategory
		PI_ARGS((int sd, int dbhandle, int category));

	extern int dlp_ReadResourceByType
		PI_ARGS((int sd, int fHandle, unsigned long type, int id,
			void *buffer, int *index, size_t *size));

	extern int dlp_ReadResourceByIndex
		PI_ARGS((int sd, int fHandle, int index, void *buffer,
			unsigned long *type, int *id, size_t *size));

	extern int dlp_WriteResource
		PI_ARGS((int sd, int dbhandle, unsigned long type, int id, 
			PI_CONST void *data, size_t length));

	extern int dlp_DeleteResource
		PI_ARGS((int sd, int dbhandle, int all, unsigned long restype,
			int resID));

	extern int dlp_ReadNextModifiedRec
		PI_ARGS((int sd, int fHandle, void *buffer, recordid_t * id,
			int *index, size_t *size, int *attr, int *category));

	extern int dlp_ReadNextModifiedRecInCategory
		PI_ARGS((int sd, int fHandle, int incategory, void *buffer,
			recordid_t * id, int *index, size_t *size, int *attr));

	extern int dlp_ReadNextRecInCategory
		PI_ARGS((int sd, int fHandle, int incategory, void *buffer,
			recordid_t * id, int *index, size_t *size, int *attr));

	extern int dlp_ReadRecordById
		PI_ARGS((int sd, int fHandle, recordid_t id, void *buffer,
			int *index, size_t *size, int *attr, int *category));

	extern int dlp_ReadRecordByIndex
		PI_ARGS((int sd, int fHandle, int ind, void *buffer,
			recordid_t * id, size_t *size, int *attr,
			int *category));

	/* Deletes all records in the opened database which are marked as
	   archived or deleted.
	 */
	extern int dlp_CleanUpDatabase PI_ARGS((int sd, int fHandle));

	/* For record databases, reset all dirty flags. For both record and
	   resource databases, set the last sync time to now.
	 */
	extern int dlp_ResetSyncFlags PI_ARGS((int sd, int fHandle));


	/* 32-bit retcode and data over 64K only supported on v2.0 Palms */
	extern int dlp_CallApplication
		PI_ARGS((int sd, unsigned long creator, unsigned long type,
			int action, size_t length, void *data,
			unsigned long *retcode, size_t maxretlen, int *retlen,
			void *retdata));

	extern int dlp_ReadFeature
		PI_ARGS((int sd, unsigned long creator, unsigned int num,
			unsigned long *feature));

	extern int dlp_GetROMToken
		PI_ARGS((int sd, unsigned long token, char *buffer, size_t *size));

	/* PalmOS 2.0 only */
	extern int dlp_ReadNetSyncInfo
		PI_ARGS((int sd, struct NetSyncInfo * i));

	/* PalmOS 2.0 only */
	extern int dlp_WriteNetSyncInfo
		PI_ARGS((int sd, struct NetSyncInfo * i));

	extern int dlp_ReadAppPreference
		PI_ARGS((int sd, unsigned long creator, int id, int backup,
			int maxsize, void *buffer, size_t *size, int *version));

	extern int dlp_WriteAppPreference
		PI_ARGS((int sd, unsigned long creator, int id, int backup,
			int version, void *buffer, size_t size));

	/* PalmOS 3.0 only */
	extern int dlp_SetDBInfo
	        PI_ARGS((int sd, int dbhandle, int flags, int clearFlags, unsigned int version,
			 time_t createDate, time_t modifyDate, time_t backupDate, 
			 unsigned long type, unsigned long creator));

	extern int dlp_FindDBByName
	        PI_ARGS((int sd, int cardno, char *name, unsigned long *localid, int *dbhandle,
			 struct DBInfo *info, struct DBSizeInfo *size));

	extern int dlp_FindDBByOpenHandle 
	        PI_ARGS((int sd, int dbhandle, int *cardno, unsigned long *localid, 
			 struct DBInfo *info, struct DBSizeInfo *size));

	extern int dlp_FindDBByTypeCreator
	        PI_ARGS((int sd, unsigned long type, unsigned long creator, int start, 
			 int latest, int *cardno, unsigned long *localid, int *dbhandle,
			 struct DBInfo *info, struct DBSizeInfo *size));

	struct RPC_params;

	extern int dlp_RPC
		PI_ARGS((int sd, struct RPC_params * p,
			unsigned long *result));

	/* Palm OS 4.0 only */
	extern int dlp_ExpSlotEnumerate
		PI_ARGS((int sd, int *numSlots, int *slotRefs));

	extern int dlp_ExpCardPresent
		PI_ARGS((int sd, int SlotRef));

	extern int dlp_ExpCardInfo
		PI_ARGS((int sd, int SlotRef, unsigned long *flags,
			 int *numStrings, char **strings));

	extern int dlp_VFSGetDefaultDir
		PI_ARGS((int sd, int volRefNum, const char *name,char *dir, int *len));

	extern int dlp_VFSImportDatabaseFromFile
		PI_ARGS((int sd, int volRefNum, const char *pathNameP, 
			 int *cardno, unsigned long *localid));

	extern int dlp_VFSExportDatabaseToFile
		PI_ARGS((int sd, int volRefNum, const char *pathNameP,
			int cardno, unsigned int localid));

	extern int dlp_VFSFileCreate
		PI_ARGS((int sd, int volRefNum,const char *name));

	extern int dlp_VFSFileOpen
		PI_ARGS((int sd, int volRefNum, const char *path, int openMode,
			FileRef *outFileRef));

	extern int dlp_VFSFileClose
		PI_ARGS((int sd, FileRef afile));

	extern int dlp_VFSFileWrite
		PI_ARGS((int sd, FileRef afile, unsigned char *data, size_t len));

	extern int dlp_VFSFileRead
		PI_ARGS((int sd, FileRef afile, unsigned char *data, size_t 
			*numBytes));

	extern int dlp_VFSFileDelete
		PI_ARGS((int sd, int volRefNum, const char *name));

	extern int dlp_VFSFileRename
		PI_ARGS((int sd, int volRefNum, const char *name,
			const char *newname));

	extern int dlp_VFSFileEOF
		PI_ARGS((int sd, FileRef afile));

	extern int dlp_VFSFileTell
		PI_ARGS((int sd, FileRef afile,int *position));

	extern int dlp_VFSFileGetAttributes
		PI_ARGS((int sd, FileRef afile, unsigned long *attributes));

	extern int dlp_VFSFileSetAttributes
		PI_ARGS((int sd, FileRef afile, unsigned long attributes));

	extern int dlp_VFSFileGetDate
		PI_ARGS((int sd, FileRef afile, int which, time_t *date));

	extern int dlp_VFSFileSetDate
		PI_ARGS((int sd, FileRef afile, int which, time_t date));

	extern int dlp_VFSDirCreate
		PI_ARGS((int sd, int volRefNum, const char *path));

	extern int dlp_VFSDirEntryEnumerate
		PI_ARGS((int sd, FileRef dirRefNum, unsigned long *dirIterator, 
			int *maxDirItems, struct VFSDirInfo *dirItems));

	extern int dlp_VFSVolumeFormat
		PI_ARGS((int sd, unsigned char flags, int fsLibRef, 
			struct VFSSlotMountParamTag *param));

	extern int dlp_VFSVolumeEnumerate
		PI_ARGS((int sd, int *numVols, int *volRefs));

	extern int dlp_VFSVolumeInfo
		PI_ARGS((int sd, int volRefNum, struct VFSInfo *volInfo));

	extern int dlp_VFSVolumeGetLabel
		PI_ARGS((int sd, int volRefNum, int *len, char *name));

	extern int dlp_VFSVolumeSetLabel
		PI_ARGS((int sd, int volRefNum, const char *name));

	extern int dlp_VFSVolumeSize
		PI_ARGS((int sd, int volRefNum, long *volSizeUsed, 
			long *volSizeTotal));

	extern int dlp_VFSFileSeek
		PI_ARGS((int sd, FileRef afile, int origin, int offset));

	extern int dlp_VFSFileResize
		PI_ARGS((int sd, FileRef afile, int newSize));

	extern int dlp_VFSFileSize
		PI_ARGS((int sd, FileRef afile,int *size));

#ifdef __cplusplus
}
#endif
#endif				/*_PILOT_DLP_H_*/
