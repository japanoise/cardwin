#include <stdio.h>
#include "data.h"

int main(int argc, char *argv[])
{
	if (argc == 0) {
		fprintf(stderr,
			"%s: open and re-save a cardfile "
			"to scratch/testOut.crd\n"
			"Please supply a cardfile.\n",
			argv[0]);
	}

	crd_cardfile *data = crd_cardfile_new();
	if (!crd_cardfile_load(data, argv[1])) {
		fprintf(stderr,
			"%s: error reading cardfile\n",
			argv[0]);
		crd_cardfile_destroy(data);
		return 1;
	}

	if (!crd_cardfile_save(data, "scratch/testOut.crd")) {
		fprintf(stderr,
			"%s: error writing cardfile\n",
			argv[0]);
		crd_cardfile_destroy(data);
		return 1;
	}

	crd_cardfile_destroy(data);
	return 0;
}
