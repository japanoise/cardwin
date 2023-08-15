#include "data.h"

int main(int argc, char  *argv[])
{
	crd_cardfile *deck = crd_cardfile_new();

	crd_card *card = crd_cardfile_add_new_card(deck, "Alice");
	crd_card_set_data(card, "Alice lives at:\r\n21 Fake Street\n"
			  "Dover, Delaware");
	card = crd_cardfile_add_new_card(deck, "Delia");
	crd_card_set_data(card, "Delia owns a pet ferret");
	card = crd_cardfile_add_new_card(deck, "Charlie");
	crd_card_set_data(card, "Charlie likes jazz music");
	card = crd_cardfile_add_new_card(deck, "Bob");
	crd_card_set_data(card, "Bob collects old IBM manuals");

	crd_cardfile_delete_card(deck, 3);

	crd_cardfile_save(deck, "scratch/edittest.crd");

	crd_cardfile_destroy(deck);
	return 0;
}
