/*
 * money2qif.c:  Translate Palm MoneyManager database into QIF format
 *
 * Copyright (c) 1998, Rui Oliveira
 * Copyright (c) 1996, Kenneth Albanowski (original read-todos.c)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include "getopt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pi-source.h"
#include "pi-socket.h"
#include "pi-money.h"
#include "pi-dlp.h"
#include "pi-header.h"

int pilot_connect(const char *port);

struct option options[] = {
	{"help",        no_argument,       NULL, 'h'},
	{"version",     no_argument,       NULL, 'v'},
	{"port",        required_argument, NULL, 'p'},
	{"account",     required_argument, NULL, 'a'},
	{NULL,          0,                 NULL,  0}
};

static const char *optstring = "hvp:a:";

static void Help(char *progname)
{
	printf("   Convert and sync your MicroMoney account data Quicken QIF format\n\n"
	       "   Usage: %s -p <port> -a Account\n\n"
	       "   Options:\n"
	       "     -p <port>           Use device file <port> to communicate with Palm\n"
	       "     -a Account          The name of the Account category in MicroMoney\n"
	       "     -h, --help        Display this information\n"
	       "     -v, --version     Display version information\n\n"
	       "   Examples: %s -p /dev/pilot -a BancGlobal\n\n"
	       "   Please see http://www.techstop.com.my/MicroMoney.htm for more information\n"
	       "   on MicroMoney.\n\n"	       
	       "   NOTE: MicroMoney is no longer supported or supplied by Landware, and has\n"
	       "   been superceded by PocketQuicken. There is no PocketQuicken conduit in\n"
	       "   pilot-link yet.\n\n", progname, progname);
	return;
}

int main(int argc, char *argv[])
{
	int 	count,
		db,
		index,
		sd = -1;

	char 	*noteln,
		*progname = argv[0],
		*port = NULL,
		*account = NULL;

	struct 	MoneyAppInfo mai;

	
	unsigned char buffer[0xffff];

	while ((count = getopt_long(argc, argv, optstring, options, NULL)) != -1) {
		switch (count) {

		case 'h':
			Help(progname);
			return 0;
		case 'v':
			PalmHeader(progname);
			return 0;
		case 'p':
			port = optarg;
			break;
		case 'a':
			account = optarg;
			break;
		default:
		}
	}

	if (account == NULL) {
		Help(progname);
		fprintf(stderr, "ERROR: You must specify an Account Category as found in MicroMoney \n");
		return -1;
	}
	
	sd = pilot_connect(port);
	if (sd < 0)
		goto error;

	/* Open the Money database, store access handle in db */
	if (dlp_OpenDB(sd, 0, 0x80 | 0x40, "MoneyDB", &db) < 0) {
		printf("Unable to open MoneyDB");
		dlp_AddSyncLogEntry(sd, "Unable to open MoneyDB.\n");
		exit(1);
	}
	
	dlp_ReadAppBlock(sd, db, 0, buffer, 0xffff);
	unpack_MoneyAppInfo(&mai, buffer, 0xffff);
	
	for (index = 0; index < 16; index++)
		if (!strcmp(mai.category.name[index], account))
			break;
	
	if (index < 16) {
	
		printf("!Type:Bank\n");
	
		for (index = 0;; index++) {
			int 	attr,
				category;

			struct 	Transaction t;
	
			int len =
				dlp_ReadRecordByIndex(sd, db, index, buffer, 0, 0,
						      &attr,
						      &category);
	
			if (len < 0)
				break;
	
				/* Skip deleted records */
			if ((attr & dlpRecAttrDeleted)
			    || (attr & dlpRecAttrArchived))
				continue;
	
			if (strcmp(mai.category.name[category], account))
				continue;
	
			unpack_Transaction(&t, buffer, len);
	
			printf("D%02d/%02d/%2d\n", t.month, t.day,
			       t.year - 1900);
			if (t.checknum)
				printf("N%d\n", t.checknum);
			printf("T%ld.%02d\n", t.amount, t.amountc);
			printf("P%s\n", t.description);
			if (t.xfer == category)
				printf("L%s\n",
				       mai.typeLabels[(int) t.type]);
			else
				printf("L[%s]\n",
				       mai.category.name[(int) t.xfer]);
			if (strcmp(t.note, "")) {
				while ((noteln = strchr(t.note, '\n')))
					*noteln = ' ';
				printf("M%s\n", t.note);
			}
			if (t.flags & 1)
				printf("CX\n");
			printf("\n^\n");
	
		}
	}

	/* Close the database */
	dlp_CloseDB(sd, db);
	dlp_AddSyncLogEntry(sd, "money2qif, successfully read MoneyDB from Palm.\n"
			    "Thank you for using pilot-link.\n");
	pi_close(sd);

	return 0;

 error:
	perror("\tERROR:");
	fprintf(stderr, "\n");

	return -1;
}