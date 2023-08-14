/*
 * This file presents an interface for reading and writing CRD files
 * into a C struct.
 *
 * It laughs in the face of concerns about efficiency or size. The
 * concern here is in presenting a comprehensible interface that's
 * easy to hack on.
 */

#ifndef _CRD_DATA_H
#define _CRD_DATA_H

#include <stdint.h>

/* I will support DKO-format when I have a sample to analyze */
enum tag_signature {
	CRD_SIG_MGC,
	CRD_SIG_RRG,
};

typedef enum tag_signature crd_signature;

/* clang-format off */
typedef struct {
	char title[40];		/* Card title */
	char *data;		/* The text data on the card */
	uint8_t flag;		/* Flag byte, probably unused */
	uint16_t datalen;	/* Size of data */
	int databuf;		/* Buffer size of data array in bytes */
	uint32_t offset;	/* Offset of data in file, used for reading/writing */
	int bmpsize;		/* Size of bitmap data in bytes */
	uint8_t *bmpdata;	/* Bitmap data, currently imported as-is */
	int olesize;		/* Size of OLE data in bytes */
	uint8_t *oledata;	/* OLE data, currently imported as-is */
} crd_card;

typedef struct {
	crd_signature signature; /* Which file format the cardfile uses */
	uint16_t ncards;	 /* Number of cards */
	uint16_t bufsize;	 /* Size of the cards buffer */
	uint32_t lastObjectId;	 /* (RRG only) ID of the last object in the file */
	crd_card **cards;	 /* The card data, array of pointers */
} crd_cardfile;
/* clang-format on */

/* Create a blank CRD structure */
crd_cardfile *crd_cardfile_new();

/* Loads a CRD file into a structure. Returns number of bytes read if
 * successful, 0 otherwise. */
int crd_cardfile_load(crd_cardfile *cardfile, char *filename);

/* Saves a cardfile to the given filename. Returns falsey if it
 * encounters an io error */
int crd_cardfile_save(crd_cardfile *cardfile, char *filename);

/* Frees a previously allocated cardfile */
void crd_cardfile_destroy(crd_cardfile *cardfile);

/* Adds a card to the cardfile. Returns the card created, or null if
 * it cannot (i.e. if there's no space)
 *
 * The card is inserted into the list in alphabetical order, so a
 * memmove may be needed.
 */
crd_card *crd_cardfile_add_new_card(crd_cardfile *cardfile, char *title);

/* Deletes a card from the cardfile, freeing the memory allocated for
 * it. Does a bounds check. */
void crd_cardfile_delete_card(crd_cardfile *cardfile, int at);

#endif
