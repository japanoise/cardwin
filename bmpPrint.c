#include <stdio.h>
#include <unistd.h>
#include "data.h"

int main(int argc, char *argv[])
{
	if (argc == 0) {
		fprintf(stderr,
			"%s: print bmps in cardfile\n"
			"Please supply a cardfile.\n",
			argv[0]);
	}

	crd_cardfile *data = crd_cardfile_new();
	if (!crd_cardfile_load(data, argv[1])) {
		fprintf(stderr, "%s: error reading cardfile\n", argv[0]);
		crd_cardfile_destroy(data);
		return 1;
	}

	for (int i = 0; i < data->ncards; i++) {
		crd_card *card = data->cards[i];
		printf("Card %i: %s\n", i, card->title);
		uint16_t width, height, xcoord, ycoord, datalen;
		uint8_t *datastart;
		if (!crd_card_parse_bmp(card, &width, &height, &xcoord, &ycoord,
					&datalen, &datastart)) {
			printf("Card has no bmp data or can't parse.\n");
			continue;
		}
		printf("width: %d; height %d; datalen %d\n", width, height,
		       datalen);

		int rowbytes = width / 8;
		if (width % 8) {
			rowbytes++;
		}

		if (datalen >= (width * height)) {
			/* Malformed bmp with extra data -
			   find the start with a heuristic. */
			datalen = (rowbytes+1) * height;
			int j = datalen;
			/* 0 is black which will rarely be used as a background,
			 * but is used for extra data in malformed bmps.
			 */
			while (j != rowbytes*height && datastart[j] == 0) {
				j--;
			}
			if (j == rowbytes*height) {
				datalen = j;
			}
		}

		if ((rowbytes+1) * height == datalen) {
			rowbytes++;
		}

		/* image bits aligned to 8 memory bits */
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < rowbytes; x++) {
				uint8_t byte = datastart[(y * rowbytes) + x];
				for (int i = 0; i < 8; i++) {
					putchar((byte << i) & 0x80 ? '`' : '#');
				}
			}
			putchar('\n');
		}
	}

	crd_cardfile_destroy(data);

	return 0;
}
