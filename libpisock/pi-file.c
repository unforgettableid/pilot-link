/* 
 * Pilot File Interface Library
 * Pace Willisson <pace@blitz.com> December 1996
 * Additions by Kenneth Albanowski
 *
 * This is free software, licensed under the GNU Library Public License V2.
 * See the file COPYING.LIB for details.
 *
 * the following is extracted from the combined wisdom of
 * PDB by Kevin L. Flynn
 * install-prc by Brian J. Swetland, D. Jeff Dionne and Kenneth Albanowski
 * makedoc7 by Pat Beirne, <patb@corel.com>
 * and the include files from the pilot SDK
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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pi-debug.h"
#include "pi-source.h"
#include "pi-socket.h"
#include "pi-file.h"

#define pi_mktag(c1,c2,c3,c4) (((c1)<<24)|((c2)<<16)|((c3)<<8)|(c4))

/*
   header:
   32		name
   2		flags
   2		version
   4		creation time
   4 		modification time
   4		backup time
   4		modification number
   4		app info offset 
   4		sort info offset
   4		type
   4		creator
   4		uniq id seed (I think it is just garbage)
   4		next record list id (normally 0, or ptr to extended hdr)
   2		num records for this header

   Hypothetically plus 2 more bytes if an extended or perhaps secondary
   header (not supported) (In practice, this value is never set, instead it
   usually indicates a damaged file.)
 
   if the low bit of attr is on, then next thing is a list of resource entry
   descriptors:
 
   resource entry header
   4		type
   2		id
   4		offset
 
   otherwise, if the low bit of attr is off, the next thing is a list of
   record entry decriptors:
 
   record entry header
   4		offset
   1		record attributes
   3		unique id
 
   then two bytes of unknown purpose, \0\0 seems safe
 
   next, the app_info, if any, then the sort_info, if any
 
   then the space used the data. Every offset is an offset from the
   beginning of the file, and will point until this area. Each block starts
   at the given offset and ends at the beginning of the next block. The last
   block ends at the end of the file.
 */

#define PI_HDR_SIZE 78
#define PI_RESOURCE_ENT_SIZE 10
#define PI_RECORD_ENT_SIZE 8

struct pi_file_entry {
	int 	offset,
		size,
		id,
		attrs;
	unsigned long type;
	pi_uid_t uid;
};

struct pi_file {
	int 	err,
		for_writing,	
		app_info_size,
		sort_info_size,
		next_record_list_id,
		resource_flag,
		ent_hdr_size,
		nentries,
		nentries_allocated,
		rbuf_size;
	FILE 	*f;
	FILE 	*tmpf;
	char 	*file_name;
	void 	*app_info,
		*sort_info,
		*rbuf;
	unsigned long unique_id_seed;
	struct 	DBInfo info;
	struct 	pi_file_entry *entries;


};

static int pi_file_close_for_write(struct pi_file *pf);
static void pi_file_free(struct pi_file *pf);

int file_size;
unsigned long start_time;
void display_rate(int record, int records, int written, int elapsed);

void display_rate(int record, int records, int written, int elapsed)
{
	int 	est_done;
	float 	k_sec;

	if (elapsed < 1)
		elapsed = 1;
	if (written < 1)
		written = 1;
	k_sec = ((double) (written / 1024) / (double) elapsed);
	
	if (file_size > 0) {
		est_done = (file_size - written) / (written / elapsed);

		fprintf(stderr, "   Record %3d of %3d. Wrote %9d bytes of %9d. Elapsed:"
		       "%2d sec.  %0.2f KB/s  Remaining: %d\r", record, records, 
		       written, file_size, elapsed, k_sec, est_done);
	} else {
		fprintf(stderr, "   Record %3d of %3d. Wrote %9d bytes. Elapsed: %2d "
		       "sec.  %0.2f KB/s.\r", record, records, written, elapsed, k_sec);

	}
}

/* this seems to work, but what about leap years? */
/*#define PILOT_TIME_DELTA (((unsigned)(1970 - 1904) * 365 * 24 * 60 * 60) + 1450800)*/

/* Exact value of "Jan 1, 1970 0:00:00 GMT" - "Jan 1, 1904 0:00:00 GMT" */
#define PILOT_TIME_DELTA (unsigned)(2082844800)


/* FIXME: These conversion functions apply no timezone correction. UNIX uses
   UTC for time_t's, while the Pilot uses local time for database backup
   time and appointments, etc. It is not particularly simple to convert
   between these in UNIX, especially since the Pilot's local time is
   unknown, and if syncing over political boundries, could easily be
   different then the local time on the UNIX box. Since the Pilot does not
   know what timezone it is in, there is no unambiguous way to correct for
   this.
   
   Worse, the creation date for a program is stored in the local time _of
   the computer which did the final linking of that program_. Again, the
   Pilot does not store the timezone information needed to reconstruct
   where/when this was.
   
   A better immediate tack would be to dissect these into struct tm's, and
   return those.
                                                                     --KJA
   */

/***********************************************************************
 *
 * Function:    pilot_time_to_unix_time
 *
 * Summary:     Convert the Palm time to Unix time
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static time_t pilot_time_to_unix_time(unsigned long raw_time)
{
	return (time_t) (raw_time - PILOT_TIME_DELTA);
}

/***********************************************************************
 *
 * Function:    unix_time_to_pilot_time
 *
 * Summary:     Convert Unix time to Palm time
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static unsigned long unix_time_to_pilot_time(time_t t)
{
	return (unsigned long) ((unsigned long) t + PILOT_TIME_DELTA);
}

/***********************************************************************
 *
 * Function:    pi_file
 *
 * Summary:     Open .prc or .pdb file for reading
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
struct pi_file *pi_file_open(char *name)
{
	int 	i;
	
	struct 	pi_file *pf;
	struct 	DBInfo *ip;
	struct 	pi_file_entry *entp;
		
	unsigned char buf[PI_HDR_SIZE];
	unsigned char *p;
	unsigned long offset, app_info_offset, sort_info_offset;

	if ((pf = calloc(1, sizeof *pf)) == NULL)
		return (NULL);

	pf->for_writing = 0;

	if ((pf->f = fopen(name, "rb")) == NULL)
		goto bad;

	fseek(pf->f, 0, SEEK_END);
	file_size = ftell(pf->f);
	start_time = (unsigned long) time(NULL);
	fseek(pf->f, 0, SEEK_SET);

	if (fread(buf, PI_HDR_SIZE, 1, pf->f) != (size_t) 1) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: can't read header\n", name));
		goto bad;
	}

	p 	= buf;
	ip 	= &pf->info;

	memcpy(ip->name, p, 32);
	ip->flags 		= get_short(p + 32);
	ip->version 		= get_short(p + 34);
	ip->createDate 		= pilot_time_to_unix_time(get_long(p + 36));
	ip->modifyDate 		= pilot_time_to_unix_time(get_long(p + 40));
	ip->backupDate 		= pilot_time_to_unix_time(get_long(p + 44));
	ip->modnum 		= get_long(p + 48);
	app_info_offset 	= get_long(p + 52);
	sort_info_offset 	= get_long(p + 56);
	ip->type 		= get_long(p + 60);
	ip->creator 		= get_long(p + 64);
	pf->unique_id_seed 	= get_long(p + 68);

	/* record list header */
	pf->next_record_list_id = get_long(p + 72);
	pf->nentries 		= get_short(p + 76);

	LOG ((PI_DBG_API, PI_DBG_LVL_INFO,
	     "FILE OPEN Name: '%s' Flags: 0x%4.4X Version: %d\n",
	     ip->name, ip->flags, ip->version));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Creation date: %s", ctime(&ip->createDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Modification date: %s", ctime(&ip->modifyDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Backup date: %s", ctime(&ip->backupDate)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Appinfo Size: %d Sortinfo Size: %d\n",
	     pf->app_info_size, pf->sort_info_size));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Type: '%s'", printlong(ip->type)));
	LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
	     "  Creator: '%s' Seed: 0x%8.8lX\n", printlong(ip->creator),
	     pf->unique_id_seed));

	if (pf->next_record_list_id != 0) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: this file is probably damaged\n", name));
		goto bad;
	}

	if (ip->flags & dlpDBFlagResource) {
		pf->resource_flag = 1;
		pf->ent_hdr_size = PI_RESOURCE_ENT_SIZE;
	} else {
		pf->resource_flag = 0;
		pf->ent_hdr_size = PI_RECORD_ENT_SIZE;
	}

	if (pf->nentries < 0) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: bad header\n", name));
		goto bad;
	}

	offset = file_size;

	if (pf->nentries) {
		if ((pf->entries =
		     calloc(pf->nentries, sizeof *pf->entries)) == NULL)
			goto bad;

		for (i = 0, entp = pf->entries; i < pf->nentries;
		     i++, entp++) {
			if (fread(buf, pf->ent_hdr_size, 1, pf->f) !=
			    (size_t) 1)
				goto bad;

			p = buf;
			if (pf->resource_flag) {
				entp->type 	= get_long(p);
				entp->id 	= get_short(p + 4);
				entp->offset 	= get_long(p + 6);

				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				     "FILE OPEN Entry %d '%s' #%d @%X\n", i,
				       printlong(entp->type), entp->id,
				       entp->offset));
			} else {
				entp->offset 	= get_long(p);
				entp->attrs 	= get_byte(p + 4);
				entp->uid 	= get_treble(p + 5);

				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				     "FILE OPEN Entry %d UID: 0x%8.8X Attrs: %2.2X Offset: @%X\n", i,
				     (int) entp->uid, entp->attrs, entp->offset));
			}
		}

		for (i = 0, entp = pf->entries + pf->nentries - 1;
		     i < pf->nentries; i++, entp--) {
			entp->size 	= offset - entp->offset;
			offset 		= entp->offset;

			LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
			     "FILE OPEN Entry: %d Size: %d\n",
			     pf->nentries - i - 1, entp->size));

			if (entp->size < 0) {
				LOG ((PI_DBG_API, PI_DBG_LVL_DEBUG,
				     "FILE OPEN %s: Entry %d corrupt, giving up\n",
					name, pf->nentries - i - 1));
				goto bad;
			}
		}
	}

	if (sort_info_offset) {
		pf->sort_info_size 	= offset - sort_info_offset;
		offset 			= sort_info_offset;
#ifdef DEBUG
		printf("Sort info, size %d\n", pf->sort_info_size);
#endif
	}

	if (app_info_offset) {
		pf->app_info_size 	= offset - app_info_offset;
		offset 			= app_info_offset;
#ifdef DEBUG
		printf("App info, size %d\n", pf->app_info_size);
#endif
	}

	if (pf->app_info_size < 0 || pf->sort_info_size < 0) {
		LOG ((PI_DBG_API, PI_DBG_LVL_ERR,
 		     "FILE OPEN %s: bad header\n", name));
		goto bad;
	}

	if (pf->app_info_size == 0)
		pf->app_info = NULL;
	else {
		if ((pf->app_info = malloc(pf->app_info_size)) == NULL)
			goto bad;
		fseek(pf->f, app_info_offset, SEEK_SET);
		if (fread(pf->app_info, 1, pf->app_info_size, pf->f) !=
		    (size_t) pf->app_info_size)
			goto bad;
	}

	if (pf->sort_info_size == 0)
		pf->sort_info = NULL;
	else {
		if ((pf->sort_info = malloc(pf->sort_info_size)) == NULL)
			goto bad;
		fseek(pf->f, sort_info_offset, SEEK_SET);
		if (fread(pf->sort_info, 1, pf->sort_info_size, pf->f) !=
		    (size_t) pf->sort_info_size)
			goto bad;
	}

	return (pf);

      bad:
	pi_file_close(pf);
	return (NULL);
}

/***********************************************************************
 *
 * Function:    pi_file_close
 *
 * Summary:     Close the open file handle
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_close(struct pi_file *pf)
{
	int 	err;

	if (pf->for_writing) {
		if (pi_file_close_for_write(pf) < 0)
			pf->err = 1;
	}

	err = pf->err;

	pi_file_free(pf);

	if (err)
		return (-1);

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_free
 *
 * Summary:     Flush and clean the file handles used
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static void pi_file_free(struct pi_file *pf)
{
	if (pf->f)
		fclose(pf->f);
	
	if (pf->app_info)
		free(pf->app_info);
	
	if (pf->sort_info)
		free(pf->sort_info);
	
	if (pf->entries)
		free(pf->entries);
	
	if (pf->file_name)
		free(pf->file_name);
	
	if (pf->rbuf)
		free(pf->rbuf);
	
	if (pf->tmpf)
		fclose(pf->tmpf);
	
	free(pf);
}

/***********************************************************************
 *
 * Function:    pi_file_get_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_get_info(struct pi_file *pf, struct DBInfo *infop)
{
	*infop = pf->info;
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_get_app_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_get_app_info(struct pi_file *pf, void **datap, int *sizep)
{
	*datap = pf->app_info;
	*sizep = pf->app_info_size;
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_get_sort_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_get_sort_info(struct pi_file *pf, void **datap, int *sizep)
{
	*datap = pf->sort_info;
	*sizep = pf->sort_info_size;
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_set_rbuf_size
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_file_set_rbuf_size(struct pi_file *pf, int size)
{
	int 	new_size;
	void 	*rbuf;

	if (size > pf->rbuf_size) {
		if (pf->rbuf_size == 0) {
			new_size = size + 2048;
			rbuf = malloc(new_size);
		} else {
			new_size = size + 2048;
			rbuf = realloc(pf->rbuf, new_size);
		}

		if (rbuf == NULL)
			return (-1);

		pf->rbuf_size = new_size;
		pf->rbuf = rbuf;
	}

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_find_resource_by_type_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int
pi_file_find_resource_by_type_id(struct pi_file *pf,
				 unsigned long type, int id, int *idxp)
{
	int 	i;
	struct 	pi_file_entry *entp;

	if (!pf->resource_flag)
		return (-1);

	for (i = 0, entp = pf->entries; i < pf->nentries;
	     i++, entp++) {
		if (entp->type == type && entp->id == id) {
			if (idxp)
				*idxp = i;
			return (0);
		}
	}

	return (-1);
}

/***********************************************************************
 *
 * Function:    pi_file_read_resource_by_type_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_read_resource_by_type_id(struct pi_file *pf, unsigned long type,
				 int id, void **bufp, int *sizep,
				 int *idxp)
{
	int 	i;

	if (pi_file_find_resource_by_type_id(pf, type, id, &i) == 0) {
		if (idxp)
			*idxp = i;
		return (pi_file_read_resource
			(pf, i, bufp, sizep, NULL, NULL));
	}

	return (-1);
}

/***********************************************************************
 *
 * Function:    pi_file_type_id_used
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_type_id_used(struct pi_file *pf, unsigned long type, int id)
{
	return (pi_file_find_resource_by_type_id(pf, type, id, NULL) == 0);
}

/*********************************************************************** 
 * 
 * Function:    pi_file_read_resource
 *
 * Summary:     Returned buffer is valid until next call, or until
 *              pi_file_close
 *
 * Parameters:  None
 *
 * Returns:     Nothing  
 *
 ***********************************************************************/
int
pi_file_read_resource(struct pi_file *pf, int i,
		      void **bufp, int *sizep, unsigned long *type,
		      int *idp)
{
	struct pi_file_entry *entp;

	if (pf->for_writing)
		return (-1);

	if (!pf->resource_flag)
		return (-1);

	if (i < 0 || i >= pf->nentries)
		return (-1);

	entp = &pf->entries[i];

	if (bufp) {
		if (pi_file_set_rbuf_size(pf, entp->size) < 0)
			return (-1);
		fseek(pf->f, pf->entries[i].offset, SEEK_SET);
		if (fread(pf->rbuf, 1, entp->size, pf->f) !=
		    (size_t) entp->size)
			return (-1);
		*bufp = pf->rbuf;
	}

	if (sizep)
		*sizep = entp->size;
	if (type)
		*type = entp->type;
	if (idp)
		*idp = entp->id;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_read_record
 *
 * Summary:     Returned buffer is valid until next call, or until
 *              pi_file_close
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_read_record(struct pi_file *pf, int i,
		    void **bufp, int *sizep, int *attrp, int *catp,
		    pi_uid_t * uidp)
{
	struct pi_file_entry *entp;

	if (pf->for_writing)
		return (-1);

	if (pf->resource_flag)
		return (-1);

	if (i < 0 || i >= pf->nentries)
		return (-1);

	entp = &pf->entries[i];

	if (bufp) {
		if (pi_file_set_rbuf_size(pf, entp->size) < 0) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
			    "FILE READ_RECORD Unable to set buffer size!\n"));
			return (-1);
		}
		
		fseek(pf->f, pf->entries[i].offset, SEEK_SET);

		if (fread(pf->rbuf, 1, entp->size, pf->f) !=
		    (size_t) entp->size) {
			LOG((PI_DBG_API, PI_DBG_LVL_ERR,
			    "FILE READ_RECORD Unable to read record!\n"));
			return (-1);
		}
		
		*bufp = pf->rbuf;
	}

	LOG ((PI_DBG_API, PI_DBG_LVL_INFO,
	     "FILE READ_RECORD Record: %d Bytes: %d\n", i, entp->size));

	if (sizep)
		*sizep = entp->size;
	if (attrp)
		*attrp = entp->attrs & 0xf0;
	if (catp)
		*catp = entp->attrs & 0xf;
	if (uidp)
		*uidp = entp->uid;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_read_record_by_id
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_read_record_by_id(struct pi_file *pf, pi_uid_t uid,
			  void **bufp, int *sizep, int *idxp, int *attrp,
			  int *catp)
{
	int 	i;
	struct 	pi_file_entry *entp;

	for (i = 0, entp = pf->entries; i < pf->nentries;
	     i++, entp++) {
		if (entp->uid == uid) {
			if (idxp)
				*idxp = i;
			return (pi_file_read_record
				(pf, i, bufp, sizep, attrp, catp, &uid));
		}
	}

	return (-1);
}

/***********************************************************************
 *
 * Function:    pi_file_id_used
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_id_used(struct pi_file *pf, pi_uid_t uid)
{
	int 	i;
	struct 	pi_file_entry *entp;

	for (i = 0, entp = pf->entries; i < pf->nentries;
	     i++, entp++) {
		if (entp->uid == uid)
			return (1);
	}
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_create
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
struct pi_file *pi_file_create(char *name, struct DBInfo *info)
{
	struct pi_file *pf;

	if ((pf = calloc(1, sizeof *pf)) == NULL)
		return (NULL);

	if ((pf->file_name = strdup(name)) == NULL)
		goto bad;

	pf->for_writing = 1;
	pf->info = *info;

	if (info->flags & dlpDBFlagResource) {
		pf->resource_flag = 1;
		pf->ent_hdr_size = PI_RESOURCE_ENT_SIZE;
	} else {
		pf->resource_flag = 0;
		pf->ent_hdr_size = PI_RECORD_ENT_SIZE;
	}

	if ((pf->tmpf = tmpfile()) == NULL)
		goto bad;

	return (pf);

      bad:
	pi_file_free(pf);
	return (NULL);
}

/***********************************************************************
 *
 * Function:    pi_file_set_info
 *
 * Summary:     May call these any time before close (even multiple
 *              times)
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_set_info(struct pi_file *pf, struct DBInfo *ip)
{
	if (!pf->for_writing)
		return (-1);

	if ((ip->flags & dlpDBFlagResource) !=
	    (pf->info.flags & dlpDBFlagResource))
		return (-1);

	pf->info = *ip;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_set_app_info
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_set_app_info(struct pi_file *pf, void *data, int size)
{
	void 	*p;

	if (!size) {
		if (pf->app_info)
			free(pf->app_info);
		pf->app_info_size = 0;
		return (0);
	}

	if ((p = malloc(size)) == NULL)
		return (-1);
	memcpy(p, data, size);

	if (pf->app_info)
		free(pf->app_info);
	pf->app_info = p;
	pf->app_info_size = size;
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_set_sort_info
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_set_sort_info(struct pi_file *pf, void *data, int size)
{
	void 	*p;

	if (!size) {
		if (pf->sort_info)
			free(pf->sort_info);
		pf->sort_info_size = 0;
		return (0);
	}

	if ((p = malloc(size)) == NULL)
		return (-1);
	memcpy(p, data, size);

	if (pf->sort_info)
		free(pf->sort_info);
	pf->sort_info = p;
	pf->sort_info_size = size;
	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_append_entry
 *
 * Summary:     Internal function to extend entry list if necessary,
 *              and return a pointer to the next available slot
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static struct pi_file_entry *pi_file_append_entry(struct pi_file *pf)
{
	int 	new_count,
		new_size;
	struct 	pi_file_entry *new_entries;
	struct 	pi_file_entry *entp;

	if (pf->nentries >= pf->nentries_allocated) {
		if (pf->nentries_allocated == 0)
			new_count = 100;
		else
			new_count = pf->nentries_allocated * 3 / 2;
		new_size = new_count * sizeof *pf->entries;

		if (pf->entries == NULL)
			new_entries = malloc(new_size);
		else
			new_entries = realloc(pf->entries, new_size);

		if (new_entries == NULL)
			return (NULL);

		pf->nentries_allocated = new_count;
		pf->entries = new_entries;
	}

	entp = &pf->entries[pf->nentries++];
	memset(entp, 0, sizeof *entp);
	return (entp);
}

/***********************************************************************
 *
 * Function:    pi_file_append_resource
 *
 * Summary:
 *
 * Parameters:  None 
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int
pi_file_append_resource(struct pi_file *pf, void *buf, int size,
			unsigned long type, int id)
{
	struct pi_file_entry *entp;

	if (!pf->for_writing || !pf->resource_flag)
		return (-1);

	entp = pi_file_append_entry(pf);

	if (size && (fwrite(buf, size, 1, pf->tmpf) != 1)) {
		pf->err = 1;
		return (-1);
	}

	entp->size 	= size;
	entp->type 	= type;
	entp->id 	= id;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_append_record  
 *
 * Summary:
 *
 * Parameters:  None 
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_append_record(struct pi_file *pf, void *buf, int size,
			  int attrs, int category, pi_uid_t uid)
{
	struct pi_file_entry *entp;

#ifdef DEBUG
	printf("append: %d\n", pf->nentries);
#endif

	if (!pf->for_writing || pf->resource_flag)
		return (-1);

	entp = pi_file_append_entry(pf);

	if (size && (fwrite(buf, size, 1, pf->tmpf) != 1)) {
		pf->err = 1;
		return (-1);
	}

	entp->size 	= size;
	entp->attrs 	= (attrs & 0xf0) | (category & 0xf);
	entp->uid 	= uid;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_get_entries
 *
 * Summary:     
 *
 * Parameters:  None   
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_get_entries(struct pi_file *pf, int *entries)
{
	*entries = pf->nentries;

	return (0);
}

/***********************************************************************
 *
 * Function:    pi_file_close_for_write 
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
static int pi_file_close_for_write(struct pi_file *pf)
{
	int 	i,
		offset,
		c;
	FILE 	*f;
	
	struct 	DBInfo *ip;
	struct 	pi_file_entry *entp;
		
	unsigned char buf[PI_HDR_SIZE];
	unsigned char *p;

	ip = &pf->info;
	if (pf->nentries >= 64 * 1024) {
		printf
		    ("too many entries for this implentation of pi-file: %d\n",
		     pf->nentries);
		return (-1);
	}

	if ((f = fopen(pf->file_name, "wb")) == NULL)
		return (-1);

	ip = &pf->info;

	offset = PI_HDR_SIZE + pf->nentries * pf->ent_hdr_size + 2;

	p = buf;
	memcpy(p, ip->name, 32);
	set_short(p + 32, ip->flags);
	set_short(p + 34, ip->version);
	set_long(p + 36, unix_time_to_pilot_time(ip->createDate));
	set_long(p + 40, unix_time_to_pilot_time(ip->modifyDate));
	set_long(p + 44, unix_time_to_pilot_time(ip->backupDate));
	set_long(p + 48, ip->modnum);
	set_long(p + 52, pf->app_info_size ? offset : 0);
	offset += pf->app_info_size;
	set_long(p + 56, pf->sort_info_size ? offset : 0);
	offset += pf->sort_info_size;
	set_long(p + 60, ip->type);
	set_long(p + 64, ip->creator);
	set_long(p + 68, pf->unique_id_seed);
	set_long(p + 72, pf->next_record_list_id);
	set_short(p + 76, pf->nentries);

	if (fwrite(buf, PI_HDR_SIZE, 1, f) != 1)
		goto bad;

	for (i = 0, entp = pf->entries; i < pf->nentries; i++, entp++) {
		entp->offset = offset;

		p = buf;
		if (pf->resource_flag) {
			set_long(p, entp->type);
			set_short(p + 4, entp->id);
			set_long(p + 6, entp->offset);
		} else {
			set_long(p, entp->offset);
			set_byte(p + 4, entp->attrs);
			set_treble(p + 5, entp->uid);
		}

		if (fwrite(buf, pf->ent_hdr_size, 1, f) != 1)
			goto bad;

		offset += entp->size;
	}

	/* This may just be packing */
	fwrite("\0\0", 1, 2, f);

	if (pf->app_info
	    && (fwrite(pf->app_info, 1, pf->app_info_size, f) !=
		(size_t) pf->app_info_size))
		goto bad;

	if (pf->sort_info
	    && (fwrite(pf->sort_info, 1, pf->sort_info_size, f) !=
		(size_t) pf->sort_info_size))
		goto bad;


	rewind(pf->tmpf);
	while ((c = getc(pf->tmpf)) != EOF)
		putc(c, f);

	fflush(f);

	if (ferror(f) || feof(f))
		goto bad;

	fclose(f);
	return (0);

      bad:
	fclose(f);
	return (-1);
}

/***********************************************************************
 *
 * Function:    pi_file_retrieve
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_retrieve(struct pi_file *pf, int socket, int cardno)
{
	int 	db,
		l,
		j,
		size		= 0,
		written 	= 0;
	
	unsigned char buffer[0xffff];

	/* printf("\n"); */
	start_time = (unsigned long) time(NULL);
	if (dlp_OpenDB
	    (socket, cardno, dlpOpenRead | dlpOpenSecret, pf->info.name,
	     &db) < 0)
		return -1;

	l = dlp_ReadAppBlock(socket, db, 0, buffer, 0xffff);
	if (l > 0)
		pi_file_set_app_info(pf, buffer, l);

	if (dlp_ReadOpenDBInfo(socket, db, &l) < 0)
		return -1;

	if (pf->info.flags & dlpDBFlagResource)

		for (j = 0; j < l; j++) {
			int 	id;
			unsigned long type;

			written += size;

			/* FIXME - need to add callbacks for this info, not print here -DD
			display_rate(j + 1, l, written,
				     (int) ((unsigned long) time(NULL) -
					    start_time));
			 */

			if ((dlp_ReadResourceByIndex
			     (socket, db, j, buffer, &type, &id,
			      &size) < 0)
			    ||
			    (pi_file_append_resource
			     (pf, buffer, size, type, id) < 0)) {
				dlp_CloseDB(socket, db);
				return -1;
			}
	} else
		for (j = 0; j < l; j++) {
			int 	attr,
				category;
			unsigned long id;

			if ((dlp_ReadRecordByIndex
			     (socket, db, j, buffer, &id, &size, &attr,
			      &category) < 0)) {
				dlp_CloseDB(socket, db);
				return -1;
			}

			written += size;

			/* FIXME - need to add callbacks for this info, not print here -DD
			display_rate(j + 1, l, written,
				     (int) ((unsigned long) time(NULL) -
					    start_time));
			*/
			/* There is no way to restore records with these
			   attributes, so there is no use in backing them up
			 */
			if (attr &
			    (dlpRecAttrArchived | dlpRecAttrDeleted))
				continue;
			if (pi_file_append_record
			    (pf, buffer, size, attr, category, id) < 0) {
				dlp_CloseDB(socket, db);
				return -1;
			}
		}
	/* printf("\n"); */
	return dlp_CloseDB(socket, db);
}

/***********************************************************************
 *
 * Function:    pi_file_install
 *
 * Summary:     
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_install(struct pi_file *pf, int socket, int cardno)
{
	int 	db,
		l,
		j,
		reset 		= 0,
		size		= 0,
		flags,
		version,
		freeai 		= 0;
	
	void 	*buffer;

	version = pi_version(socket);
//	printf("\n");

	/* Delete DB if it already exists */
	dlp_DeleteDB(socket, cardno, pf->info.name);

	 /* Judd - 25Nov99 - Graffiti hack We want to make sure that these 2
	    flags get set for this one */

	if (pf->info.creator == pi_mktag('g', 'r', 'a', 'f')) {
		flags |= dlpDBFlagNewer;
		flags |= dlpDBFlagReset;
	}

	/* Set up DB flags */
	flags = pf->info.flags;

	if (strcmp(pf->info.name, "Graffiti ShortCuts ") == 0) {
		flags |= 0x8000;	/* Rewrite an open DB */
		reset = 1;	/* To be on the safe side */
	}
	LOG((PI_DBG_API, PI_DBG_LVL_INFO,
	    "FILE INSTALL Name: %s Flags: %8.8X\n", pf->info.name, flags));

	/* Create DB */
	if (dlp_CreateDB
	    (socket, pf->info.creator, pf->info.type, cardno, flags,
	     pf->info.version, pf->info.name, &db) < 0) {
		int retry = 0;

		/* Judd - 25Nov99 - Graffiti hack

		   The dlpDBFlagNewer specifies that if a DB is open and
		   cannot be deleted then it can be overwritten by a DB with
		   a different name.  The creator ID of "graf" is what
		   really identifies a DB, not the name.  We could call it
		   JimBob and the palm would still find it and use it. */

		if (strcmp(pf->info.name, "Graffiti ShortCuts ") == 0) {
			strcpy(pf->info.name, "Graffiti ShortCuts");
			retry = 1;
		} else if (strcmp(pf->info.name, "Graffiti ShortCuts") ==
			   0) {
			strcpy(pf->info.name, "Graffiti ShortCuts ");
			retry = 1;
		} else if (pf->info.creator ==
			   pi_mktag('g', 'r', 'a', 'f')) {
			/* Yep, someone has named it JimBob */
			strcpy(pf->info.name, "Graffiti ShortCuts");
			retry = 1;
		}

		if (retry) {
			/* Judd - 25Nov99 - Graffiti hack
			   We changed the name, now we can try to write it
			   again */
			if (dlp_CreateDB
			    (socket, pf->info.creator, pf->info.type,
			     cardno, flags, pf->info.version,
			     pf->info.name, &db) < 0) {
				return -1;
			}
		} else {
			return -1;
		}
	}



	pi_file_get_app_info(pf, &buffer, &l);

	/* Compensate for bug in OS 2.x Memo */
	if ((version > 0x0100) && (strcmp(pf->info.name, "MemoDB") == 0)
	    && (l > 0) && (l < 282)) {
		/* Justification: The appInfo structure was accidentally
		   lengthend in OS 2.0, but the Memo application does not
		   check that it is long enough, hence the shorter block
		   from OS 1.x will cause the 2.0 Memo application to lock
		   up if the sort preferences are modified. This code
		   detects the installation of a short app info block on a
		   2.0 machine, and lengthens it. This transformation will
		   never lose information. */

		void *b2 = calloc(1, 282);
		memcpy(b2, buffer, l);
		    
		buffer 	= b2;
		l 	= 282;
		freeai 	= 1;
	}

	/* All system updates seen to have the 'ptch' type, so trigger a
	   reboot on those */
	if (pf->info.creator == pi_mktag('p', 't', 'c', 'h'))
		reset = 1;

	if (pf->info.flags & dlpDBFlagReset)
		reset = 1;

	if (l > 0)
		dlp_WriteAppBlock(socket, db, buffer, l);

	if (freeai)
		free(buffer);

	/* Resource or record? */
	if (pf->info.flags & dlpDBFlagResource) {
		for (j = 0; j < pf->nentries; j++) {
			if ((pi_file_read_resource(pf, j, 0, &size, 0, 0)
			     == 0) && (size > 65536)) {
				LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				    "FILE INSTALL Database contains resource over 64K!\n"));
				goto fail;
			}
		}
		for (j = 0; j < pf->nentries; j++) {
			int 	id;
			unsigned long type;

			if (pi_file_read_resource
			    (pf, j, &buffer, &size, &type, &id) < 0)
				goto fail;

			/* Skip empty resource, it cannot be installed */
			if (size == 0)
				continue;

			if (dlp_WriteResource
			    (socket, db, type, id, buffer, size) < 0)
				goto fail;

			/* FIXME - need to add callbacks for this info, not print here -DD
			display_rate(j + 1, pf->nentries, ftell(pf->f),
				     (double) ((unsigned long) time(NULL) -
					       start_time));
			*/
			
			/* If we see a 'boot' section, regardless of file
			   type, require reset */
			if (type == pi_mktag('b', 'o', 'o', 't'))
				reset = 1;
		}
	} else {
		for (j = 0; j < pf->nentries; j++) {
			int size;

			if (((pi_file_read_record(pf, j, 0, &size, 0, 0, 0)
			      == 0)) && (size > 65536)) {
				LOG((PI_DBG_API, PI_DBG_LVL_ERR,
				    "FILE INSTALL Database contains resource over 64K!\n"));
				goto fail;
			}
		}
		for (j = 0; j < pf->nentries; j++) {
			int 	attr,
				category;
			unsigned long id;

			if (pi_file_read_record
			    (pf, j, &buffer, &size, &attr, &category,
			     &id) < 0)
				goto fail;

			/* Old OS version cannot install deleted records, so
			   don't even try */
			if ((attr & (dlpRecAttrArchived | dlpRecAttrDeleted))
			    && (version < 0x0101))
				continue;

			if (dlp_WriteRecord
			    (socket, db, attr, id, category, buffer, size,
			     0) < 0)
				goto fail;

			/* FIXME - need to add callbacks for this info, not print here -DD
			display_rate(j + 1, pf->nentries, ftell(pf->f),
				     (double) ((unsigned long) time(NULL) -
					       start_time));
			*/
		}
//		printf("\n");
	}

	if (reset)
		dlp_ResetSystem(socket);

	return dlp_CloseDB(socket, db);

      fail:
	dlp_CloseDB(socket, db);
	dlp_DeleteDB(socket, cardno, pf->info.name);
	return -1;
}

/***********************************************************************
 *
 * Function:    pi_file_merge
 *
 * Summary:
 *
 * Parameters:  None
 *
 * Returns:     Nothing
 *
 ***********************************************************************/
int pi_file_merge(struct pi_file *pf, int socket, int cardno)
{
	int 	db,
		j,
		reset 	= 0,
		version;
	void 	*buffer;
	
	version = pi_version(socket);

	if (dlp_OpenDB
	    (socket, cardno, dlpOpenReadWrite | dlpOpenSecret,
	     pf->info.name, &db) < 0)
		return pi_file_install(pf, socket, cardno);

	/* All system updates seen to have the 'ptch' type, so trigger a
	   reboot on those */
	if (pf->info.creator == pi_mktag('p', 't', 'c', 'h'))
		reset = 1;

	if (pf->info.flags & dlpDBFlagReset)
		reset = 1;

	/* Resource or record? */
	if (pf->info.flags & dlpDBFlagResource) {
		for (j = 0; j < pf->nentries; j++) {
			int 	size;

			if ((pi_file_read_resource(pf, j, 0, &size, 0, 0)
			     == 0) && (size > 65536)) {
				printf("Database contains resource over 64K!\n");
				goto fail;
			}
		}
		for (j = 0; j < pf->nentries; j++) {
			int 	id,
				size;
			unsigned long type;

			if (pi_file_read_resource
			    (pf, j, &buffer, &size, &type, &id) < 0)
				goto fail;
			if (size == 0)
				continue;
			if (dlp_WriteResource
			    (socket, db, type, id, buffer, size) < 0)
				goto fail;

			/* If we see a 'boot' section, regardless of file
			   type, require reset */
			if (type == pi_mktag('b', 'o', 'o', 't'))
				reset = 1;
		}
	} else {
		for (j = 0; j < pf->nentries; j++) {
			int 	size;

			if (((pi_file_read_record(pf, j, 0, &size, 0, 0, 0)
			      == 0)) && (size > 65536)) {
				printf("Database contains record over 64K!\n");
				goto fail;
			}
		}
		for (j = 0; j < pf->nentries; j++) {
			int 	size,
				attr,
				category;
			unsigned long id;

			if (pi_file_read_record
			    (pf, j, &buffer, &size, &attr, &category,
			     &id) < 0)
				goto fail;

			/* Old OS version cannot install deleted records, so
			   don't even try */
			if ((attr & (dlpRecAttrArchived | dlpRecAttrDeleted))
			    && (version < 0x0101))
				continue;
			if (dlp_WriteRecord
			    (socket, db, attr, 0, category, buffer, size,
			     0) < 0)
				goto fail;
		}
	}

	if (reset)
		dlp_ResetSystem(socket);

	return dlp_CloseDB(socket, db);

      fail:
	dlp_CloseDB(socket, db);
	return -1;
}
