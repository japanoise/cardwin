#include <stdio.h>
#include <unistd.h>
#include "data.h"

void print_json_string(char *str)
{
	putchar('"');
	for (int idx = 0; str[idx]; idx++) {
		if (str[idx] == '\r') {
			printf("\\r");
		} else if (str[idx] == '\n') {
			printf("\\n");
		} else if (str[idx] == '\t') {
			printf("\\t");
		} else if (str[idx] == '\\') {
			printf("\\\\");
		} else if (str[idx] < 0x20) {
			printf("\\u%04X", 0xFF & str[idx]);
		} else if (str[idx] > '~') {
			/* Very much not ideal (it will create
			 * mojibake), but older data is almost
			 * certainly not in UTF-8; this at least
			 * preserves the data and produces valid (if
			 * nonsensical) JSON. An intelligent operator
			 * should know what encoding the file is in
			 * and use a little creative engineering to
			 * update the data from there. */
			printf("\\u%04X", 0xFF & str[idx]);
		} else if (str[idx] == '"') {
			printf("\\\"");
		} else {
			putchar(str[idx]);
		}
	}
	putchar('"');
}

void print_json_bytes(uint8_t *bytes, int len)
{
	putchar('[');
	for (int i = 0; i < len; i++) {
		if (i != 0) {
			putchar(',');
		}
		printf(" %d", 0xFF & bytes[i]);
	}
	putchar(' ');
	putchar(']');
}

int main(int argc, char *argv[])
{
	if (argc == 0) {
		fprintf(stderr,
			"%s: convert cardfile to json\n"
			"Please supply a cardfile.\n",
			argv[0]);
	}

	crd_cardfile *data = crd_cardfile_new();
	if (!crd_cardfile_load(data, argv[1])) {
		fprintf(stderr, "%s: error reading cardfile\n", argv[0]);
		crd_cardfile_destroy(data);
		return 1;
	}

	printf("{\n");

	switch (data->signature) {
	case CRD_SIG_MGC:
		printf("\t\"magicBytes\": \"MGC\",\n");
		break;
	case CRD_SIG_RRG:
		printf("\t\"magicBytes\": \"RRG\",\n");
		break;
	}
	printf("\t\"lastObjectId\": %d,\n", data->lastObjectId);
	printf("\t\"numberOfCards\": %d,\n", data->ncards);

	printf("\t\"cards\": [\n");
	for (int i = 0; i < data->ncards; i++) {
		printf("\t\t{\n");
		crd_card *card = data->cards[i];

		printf("\t\t\t\"title\": ");
		print_json_string(card->title);
		printf(",\n");

		printf("\t\t\t\"data\": ");
		print_json_string(card->data);
		printf(",\n");

		if (card->bmpsize > 0) {
			printf("\t\t\t\"bmpData\": ");
			print_json_bytes(card->bmpdata, card->bmpsize);
			printf(",\n");
		}

		if (card->olesize > 0) {
			printf("\t\t\t\"oleData\": ");
			print_json_bytes(card->oledata, card->olesize);
			printf(",\n");
		}

		printf("\t\t\t\"flag\": %d,\n", card->flag);
		printf("\t\t\t\"offsetInFile\": %d\n", card->offset);

		if (i == data->ncards - 1) {
			printf("\t\t}\n");
		} else {
			printf("\t\t},\n");
		}
	}
	printf("\t]\n");

	printf("}\n");

	crd_cardfile_destroy(data);

	return 0;
}
