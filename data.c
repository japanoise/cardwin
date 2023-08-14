#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "data.h"

bool fexist(char *path)
{
	return access(path, F_OK) == 0;
}

bool IsBigEndian()
{
	int i = 1;
	return !*((char *)&i);
}

void write_u16(FILE *outfile, uint16_t output)
{
	fputc(output & 0xFF, outfile);
	fputc((output >> 8) & 0xFF, outfile);
}

void write_u32(FILE *outfile, uint32_t output)
{
	fputc(output & 0xFF, outfile);
	fputc((output >> 8) & 0xFF, outfile);
	fputc((output >> 16) & 0xFF, outfile);
	fputc((output >> 24) & 0xFF, outfile);
}

int string_write_u16(char *s, int idx, uint16_t output)
{
	s[idx] = output & 0xFF;
	s[idx + 1] = (output >> 8) & 0xFF;
	return 2;
}

int string_write_u32(char *s, int idx, uint32_t output)
{
	s[idx] = output & 0xFF;
	s[idx + 1] = (output >> 8) & 0xFF;
	s[idx + 2] = (output >> 16) & 0xFF;
	s[idx + 3] = (output >> 24) & 0xFF;
	return 4;
}

int read_u8(FILE *input, uint8_t *result)
{
	int by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	*result = (uint8_t)by;
	return 1;
}

int read_u16(FILE *input, uint16_t *result)
{
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
		*result |= (by << 8);
	}

	return 2;
}

int read_u32(FILE *input, uint32_t *result)
{
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
		*result |= (by << 16);
	} else {
		*result |= (by << 8);
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= (by << 8);
	} else {
		*result |= (by << 16);
	}

	by = fgetc(input);
	if (by == EOF) {
		return 0;
	}
	if (bigEndian) {
		*result |= by;
	} else {
		*result |= (by << 24);
	}

	return 4;
}

/* Create a blank CRD structure */
crd_cardfile *crd_cardfile_new()
{
	return calloc(1, sizeof(crd_cardfile));
}

/* Create a blank card */
crd_card *crd_card_new()
{
	return calloc(1, sizeof(crd_card));
}

#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8)))
#define LOBYTE(w) ((uint8_t)(w))

/* Reads a 16-bit value and snarfs it into the card's oledata,
 * increasing the buffer size if needed. */
uint16_t ReadWord(FILE *fp, crd_card *card, int *res, int *olebufsize)
{
	bool bigEndian = IsBigEndian();

	uint16_t result;
	uint8_t buf[2];
	(*res) = fread(buf, 1, 2, fp);

	if (card->olesize + 2 >= *olebufsize) {
		(*olebufsize) *= 2;
		card->oledata = realloc(card->oledata, (*olebufsize));
	}
	memcpy(card->oledata + card->olesize, buf, 2);
	card->olesize += 2;

	if (bigEndian) {
		return (buf[0] << 8) | buf[1];
	} else {
		return buf[0] | (buf[1] << 8);
	}
}

/* Reads a 32-bit value and snarfs it into the card's oledata,
 * increasing the buffer size if needed. */
uint32_t ReadLong(FILE *fp, crd_card *card, int *res, int *olebufsize)
{
	bool bigEndian = IsBigEndian();

	uint32_t result;
	uint8_t buf[4];
	(*res) = fread(buf, 1, 4, fp);

	if (card->olesize + 4 >= *olebufsize) {
		(*olebufsize) *= 2;
		card->oledata = realloc(card->oledata, (*olebufsize));
	}
	memcpy(card->oledata + card->olesize, buf, 4);
	card->olesize += 4;

	if (bigEndian) {
		return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
	} else {
		return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
	}
}

int EatBytes(FILE *fp, crd_card *card, int *olebufsize, uint32_t numbytes)
{
	if (card->olesize + numbytes >= *olebufsize) {
		while (card->olesize + numbytes >= *olebufsize) {
			(*olebufsize) *= 2;
		}
		card->oledata = realloc(card->oledata, (*olebufsize));
	}
	int ret = fread(card->oledata + card->olesize, 1, numbytes, fp);
	card->olesize += ret;
	return ret;
}

bool RdCheckVer(FILE *fp, crd_card *card, int *res, int *olebufsize)
{
	uint32_t OLEVer = ReadLong(fp, card, res, olebufsize);
	/* This had imbalanced parens when I kopiped it. Made a guess
	 * as to where the missing paren was supposed to go. */
	OLEVer = (((uint16_t)(LOBYTE(OLEVer))) << 8) | (uint16_t)HIBYTE(OLEVer);

	if (OLEVer == 0x0100) {
		return true;
	} else {
		return false;
	}
}

/* This is the only function here that I'm really unsure about */
int RdChkString(FILE *fp, crd_card *card, int *res, int *olebufsize)
{
	/* This function assumes the string is null-terminated. If
	 * not, you can fairly easily grab how long the string is then
	 * fread that many bytes into the buffer.*/
	char last;
	char buf[16]; /* Assuming METAFILEPICT is the
				 * longest value. 13 is bad luck, so
				 * make it a nice round 16. */
	memset(buf, 0, sizeof(buf));
	*res = 0;

	do {
		/* The string not being null-terminated would be the
		 * expected cause of a segfault here. */
		last = fgetc(fp);
		buf[*res] = last;
		(*res)++;
	} while (last != 0);

	if (card->olesize + (*res) >= *olebufsize) {
		(*olebufsize) *= 2;
		card->oledata = realloc(card->oledata, (*olebufsize));
	}
	memcpy(card->oledata + card->olesize, buf, (*res));
	card->olesize += (*res);

	return (!strcmp("METAFILEPICT", buf)) || (!strcmp("BITMAP", buf)) ||
	       (!strcmp("DIB", buf));
}

void SkipPresentationObj(FILE *fp, crd_card *card, int *res, int *olebufsize)
{
	// Format ID
	ReadLong(fp, card, res, olebufsize);
	if (RdChkString(fp, card, res, olebufsize)) {
		// Width in mmhimetric.
		ReadLong(fp, card, res, olebufsize);
		// Height in mmhimetric.
		ReadLong(fp, card, res, olebufsize);
		// Presentation data size and data itself
		*res = EatBytes(fp, card, olebufsize,
				ReadLong(fp, card, res, olebufsize));
	} else {
		// if Clipboard format value is NULL
		if (!ReadLong(fp, card, res, olebufsize)) {
			// Read Clipboard format name.
			*res = EatBytes(fp, card, olebufsize,
					ReadLong(fp, card, res, olebufsize));
		}
		// Presentation data size and data itself.}
		*res = EatBytes(fp, card, olebufsize,
				ReadLong(fp, card, res, olebufsize));
	}
}

/*
 * Snarfs OLE data into a card.
 *
 * https://jeffpar.github.io/kbarchive/kb/099/Q99340/
 */
int ole_snarf(FILE *fp, crd_card *card, int *olebufsize)
{
	/* Honestly this is going to go untested for now, so no error
	 * checking.  Check the main load function to find out how to
	 * do error checking in this area of the codebase. If you find
	 * yourself staring at this code thinking "who wrote this
	 * crap", it's M$'s fault. */
	/* I am open to a C&D so that I can remove this trash and say
	 * I don't support this particular file format for "legal
	 * reasons" */
	int res;

	if (RdCheckVer(fp, card, &res, olebufsize)) {
		// 1==> Linked, 2==> Embedded, 3==> Static
		uint32_t format = ReadLong(fp, card, &res, olebufsize);
		// Class String
		res = EatBytes(fp, card, olebufsize,
			       ReadLong(fp, card, &res, olebufsize));

		if (format == 3) { /* Static object */
			// Width in mmhimetric.
			ReadLong(fp, card, &res, olebufsize);
			// Height in mmhimetric.
			ReadLong(fp, card, &res, olebufsize);
			// Presentation data size and data itself.
			res = EatBytes(fp, card, olebufsize,
				       ReadLong(fp, card, &res, olebufsize));
		} else { /* Embedded/linked object */
			// Topic string.
			res = EatBytes(fp, card, olebufsize,
				       ReadLong(fp, card, &res, olebufsize));
			// Item string.
			res = EatBytes(fp, card, olebufsize,
				       ReadLong(fp, card, &res, olebufsize));
			if (format == 2) { /* Embedded object */
				// Native data and its size.
				res = EatBytes(fp, card, olebufsize,
					       ReadLong(fp, card, &res,
							olebufsize));
				// Read and eat the presentation object.
				SkipPresentationObj(fp, card, &res, olebufsize);
			} else { /* Linked object */
				// Network name.
				res = EatBytes(fp, card, olebufsize,
					       ReadLong(fp, card, &res,
							olebufsize));
				// Network type and net driver version.
				ReadLong(fp, card, &res, olebufsize);
				// Link update options.
				ReadLong(fp, card, &res, olebufsize);
				// Read and eat the presentation object.}}
				SkipPresentationObj(fp, card, &res, olebufsize);
			}
		}
	}
	return res;
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
	cardfile->cards = calloc(cardfile->ncards, sizeof(crd_cardfile *));

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
				card->bmpsize = 8 + word;
				/* For now, copy the data and do
				 * nothing with it. In a later
				 * version, convert it to an agnostic
				 * format maybe (something easy to
				 * translate to Cairo or SDL
				 * drawing)*/
				card->bmpdata = malloc(card->bmpsize);
				res = fread(card->bmpdata, 1, card->bmpsize,
					    fp);
				if (res != card->bmpsize) {
					goto cleanup;
				}
				nbytes += res;
			} else if (cardfile->signature == CRD_SIG_RRG) {
				/* Feel free to tweak this for perf. I
				 * just chose a round number that felt
				 * right.*/
				int olebufsize = 0x1000;
				card->oledata = malloc(olebufsize);
				/* Object ID */
				ReadLong(fp, card, &res, &olebufsize);
				/* OLE object */
				ole_snarf(fp, card, &olebufsize);
				/* DIB character width */
				ReadWord(fp, card, &res, &olebufsize);
				/* DIB character height */
				ReadWord(fp, card, &res, &olebufsize);
				/* X coord U-L */
				ReadWord(fp, card, &res, &olebufsize);
				/* Y coord U-L */
				ReadWord(fp, card, &res, &olebufsize);
				/* X coord L-R */
				ReadWord(fp, card, &res, &olebufsize);
				/* Y coord L-R */
				ReadWord(fp, card, &res, &olebufsize);
				/* embedded=0, linked=1, static=2 */
				ReadWord(fp, card, &res, &olebufsize);
			}
		} else {
			card->bmpsize = 0;
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
		card->databuf = word + 1;

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

/* Saves a cardfile to the given filename. Returns falsey if it
 * encounters an io error */
int crd_cardfile_save(crd_cardfile *cardfile, char *filename)
{
	char nulls[6];
	memset(nulls, 0, sizeof(nulls));
	uint32_t offset;

	FILE *out = fopen(filename, "wb");
	if (out == NULL) {
		return 0;
	}

	/* Write file sig */
	switch (cardfile->signature) {
	case CRD_SIG_MGC:
		offset = 5;
		fprintf(out, "MGC");
		break;
	case CRD_SIG_RRG:
		offset = 9;
		fprintf(out, "RRG");
		write_u32(out, cardfile->lastObjectId);
		break;
	}
	write_u16(out, cardfile->ncards);
	offset += cardfile->ncards * 52;

	/* Write header data */
	for (int i = 0; i < cardfile->ncards; i++) {
		crd_card *card = cardfile->cards[i];
		fwrite(nulls, 1, 6, out);
		write_u32(out, offset);
		offset += 4 + card->bmpsize + card->olesize + card->datalen;
		fputc(card->flag, out);
		fwrite(card->title, 1, 40, out);
		fputc(0, out);
	}

	/* Write card data */
	for (int i = 0; i < cardfile->ncards; i++) {
		crd_card *card = cardfile->cards[i];
		switch (cardfile->signature) {
		case CRD_SIG_MGC:
			if (card->bmpsize) {
				write_u16(out, card->bmpsize - 8);
				fwrite(card->bmpdata, 1, card->bmpsize, out);
			} else {
				write_u16(out, 0);
			}
			break;
		case CRD_SIG_RRG:
			if (card->olesize > 0) {
				write_u16(out, 1);
				fwrite(card->oledata, 1, card->olesize, out);
			} else {
				write_u16(out, 0);
			}
			break;
		}
		write_u16(out, card->datalen);
		fwrite(card->data, 1, card->datalen, out);
	}

	fclose(out);
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
	if (at < cardfile->ncards - 1) {
		memmove(cardfile->cards[at], cardfile->cards[at + 1],
			sizeof(crd_card *) * (cardfile->ncards - 1));
	}

	cardfile->ncards--;
	cardfile->cards[cardfile->ncards] = NULL; /* Prevents a double-free */
}
