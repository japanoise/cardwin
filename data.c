#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"

bool fexist(char *path) {
	return access( path, F_OK ) == 0;
}

bool IsBigEndian() {
    int i=1;
    return ! *((char *)&i);
}

void write_u16(FILE *outfile, uint16_t output) {
	fputc(output&0xFF, outfile);
	fputc((output>>8)&0xFF, outfile);
}

void write_u32(FILE *outfile, uint32_t output) {
	fputc(output&0xFF, outfile);
	fputc((output>>8)&0xFF, outfile);
	fputc((output>>16)&0xFF, outfile);
	fputc((output>>24)&0xFF, outfile);
}

int string_write_u16(char *s, int idx, uint16_t output) {
	s[idx] = output&0xFF;
	s[idx+1] = (output>>8)&0xFF;
	return 2;
}

int string_write_u32(char *s, int idx, uint32_t output) {
	s[idx] = output&0xFF;
	s[idx+1] = (output>>8)&0xFF;
	s[idx+2] = (output>>16)&0xFF;
	s[idx+3] = (output>>24)&0xFF;
	return 4;
}

int read_u8(FILE *input, uint8_t *result) {
	int by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	*result = (uint8_t) by;
	return 1;
}

int read_u16(FILE *input, uint16_t *result) {
	/* Not sure if this actually works on big endian systems */
	bool bigEndian = IsBigEndian();
	*result = 0;
	int by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result = (by << 8);
	} else {
		*result = by;
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= by;
	} else {
		*result |= (by<<8);
	}

	return 2;
}

int read_u32(FILE *input, uint32_t *result) {
	bool bigEndian = IsBigEndian();
	*result = 0;
	int by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result = (by << 24);
	} else {
		*result = by;
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= (by<<16);
	} else {
		*result |= (by<<8);
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= (by<<8);
	} else {
		*result |= (by<<16);
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= by;
	} else {
		*result |= (by<<24);
	}

	return 4;
}

/* Create a blank CRD structure */
crd_cardfile *crd_cardfile_new()
{
	return calloc(1, sizeof(crd_cardfile));
}

/* Create a blank card */
crd_card *crd_card_new() {
	return calloc(1, sizeof(crd_card));
}

/* Loads a CRD file into a structure. */
int crd_cardfile_load(crd_cardfile *cardfile, char *filename)
{
	char buf[6];
	FILE *fp;
	int nbytes;
	int nextOffset;
	int res;

	if ((fp = fopen(filename, "rb")) == NULL) {
		/* Couldn't open the file */
		return 0;
	}

	/* Read file signature/magic bytes */
	nbytes = fread(buf, 1, 3, fp);
	if (nbytes != 3) {
		/* EOF before file signature */
		goto cleanup;
	}

	if (!strncmp(buf, "MGC", 3)) {
		/* Simpler/older style */
		cardfile->signature = CRD_SIG_MGC;
	} else if (!strncmp(buf, "RRG", 3)) {
		/* Newer style */
		cardfile->signature = CRD_SIG_RRG;
		/* Extra 4 bytes (u32) ID of last object */
		res = read_u32(fp, &(cardfile->lastObjectId));
		if (!res) {
			goto cleanup;
		}
		nbytes += res;
	} else {
		/* Bad signature */
		/* There's also supposedly a Unicode version with
		 * magic bytes "DKO", but I'll need a documentation of
		 * the file format and a sample cardfile to analyze in
		 * order to support it. */
		goto cleanup;
	}

	/* Next is a u16 describing the number of cards */
	res = read_u16(fp, &(cardfile->ncards));
	if (!res) {
		goto cleanup;
	}
	nbytes += res;

	/* Now we can allocate the memory for the card array */
	cardfile->cards = calloc(cardfile->ncards, sizeof(crd_cardfile*));

	/* Cool, now let's start reading index entries */
	/*
	 *  0 - 5  Null bytes, reserved for future.
	 *  6 - 9  Absolute position of card data in file (32 bits)
	 *   10    Flag byte (00)
	 * 11 - 50 Index line text, null terminated
	 *   51    Null byte, indicates end of entry
	 */
	for (int i = 0; i < cardfile->ncards; i++) {
		/* Reserved bytes */
		res = fread(buf, 1, 6, fp);
		if (res != 6) {
			goto cleanup;
		}
		nbytes += res;

		/* Allocate the memory. It's safe to do here because
		 * the destructor can free it later even if the file
		 * is not successfully loaded. */
		crd_card *newcard = crd_card_new();
		cardfile->cards[i] = newcard;

		/* Not sure we'll use this, but might as well load it. */
		res = read_u32(fp, &(newcard->offset));
		if (!res) {
			goto cleanup;
		}
		nbytes += res;

		/* Ditto. */
		res = read_u8(fp, &(newcard->flag));
		if (!res) {
			goto cleanup;
		}
		nbytes += res;

		/* This we will need! */
		res = fread(newcard->title, 1, 40, fp);
		if (res != 40) {
			goto cleanup;
		}
		nbytes += res;

		/* Just a null */
		res = fread(buf, 1, 1, fp);
		if (res != 1) {
			goto cleanup;
		}
		nbytes += res;
	}

	/* Now we're going to read data entries. */
	for (int i = 0; i < cardfile->ncards; i++) {
		crd_card *card = cardfile->cards[i];

		uint16_t word = 0;
		res = read_u16(fp, &word);
		if (!res) {
			goto cleanup;
		}
		nbytes += res;
		/* What we do with this depends on the format. This is
		 * the length of graphic (bmpsize) in MGC, but the
		 * object flag in RRG. */
		if (word) {
			if (cardfile->signature == CRD_SIG_MGC) {
				card->bmpsize = 8+word;
				/* For now, copy the data and do
				 * nothing with it. In a later
				 * version, convert it to an agnostic
				 * format maybe (something easy to
				 * translate to Cairo or SDL
				 * drawing)*/
				card->bmpdata = malloc(card->bmpsize);
				res = fread(
					card->bmpdata, 1, card->bmpsize, fp);
				if (res != card->bmpsize) {
					goto cleanup;
				}
				nbytes += res;
			} else if (cardfile->signature == CRD_SIG_RRG) {
				/* Not yet implemented. */
				/* Pseudocode follows. */
				/* Read the object id */
				/* Read over the OLE object, copying into oledata */
			}
		}

		/* Size of the data in bytes. I believe this is not
		 * null-terminated, but I could be wrong. */
		word = 0;
		res = read_u16(fp, &word);
		if (!res) {
			goto cleanup;
		}
		nbytes += res;
		card->datalen = word;
		card->databuf = word+1;

		/* Now the actual text data. */
		card->data = calloc(card->databuf, 1);
		res = fread(card->data, 1, card->datalen, fp);
		if (res != card->datalen) {
			goto cleanup;
		}
		nbytes += res;
	}

	fclose(fp);
	return nbytes;

cleanup:
	fclose(fp);
	return 0;
}

/* Saves a cardfile to the given filename. Returns any io errors
 * encountered. */
int crd_cardfile_save(crd_cardfile *cardfile, char *filename)
{
	return 1;
}

void card_destroy(crd_card *card)
{
	free(card->data);
	free(card->bmpdata);
	free(card->oledata);
	free(card);
}

/* Frees a previously allocated cardfile */
void crd_cardfile_destroy(crd_cardfile *cardfile)
{
	for (int i = 0; i < cardfile->ncards; i++) {
		card_destroy(cardfile->cards[i]);
	}
	free(cardfile->cards);
	free(cardfile);
}

/* Adds a card to the cardfile. Returns the card created, or null if
 * it cannot (i.e. if there's no space)
 *
 * The card is inserted into the list in alphabetical order, so a
 * memmove may be needed.
 */
crd_card *crd_cardfile_add_new_card(crd_cardfile *cardfile, char *title)
{
	return NULL;
}

/* Deletes a card from the cardfile, freeing the memory allocated for
 * it. Does a bounds check. */
void crd_cardfile_delete_card(crd_cardfile *cardfile, int at)
{
	if (at < 0 || at >= cardfile->ncards) {
		return;
	}

	card_destroy(cardfile->cards[at]);
	if (at < cardfile->ncards-1) {
		memmove(cardfile->cards[at], cardfile->cards[at+1],
			sizeof(crd_card*)*(cardfile->ncards-1));
	}

	cardfile->ncards--;
	cardfile->cards[cardfile->ncards] = NULL; /* Prevents a double-free */
}
