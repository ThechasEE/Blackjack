#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

// Used for cross-platform sleep. (Not mine)
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// Defines to prevent magic numbers.
#define MAX_HAND_COUNT 15
#define MIN_DECK_COUNT 1
#define MAX_DECK_COUNT 8
#define MIN_DIFFICULTY 1
#define MAX_DIFFICULTY 3
#define MIN_MONEY 15
#define MAX_MONEY 100000
#define DIFFICULTY_MULTIPLIER_1 4
#define DIFFICULTY_MULTIPLIER_2 10
#define DIFFICULTY_MULTIPLIER_3 20
#define CARDS_IN_A_DECK 52
#define INITIAL_CARD_DRAW 4
#define MAX_SYMBOL_LENGTH 3
#define STANDARD_SLEEP_TIME 500
#define MAX_CARDS_IN_A_ROW 5
#define NUMBER_OF_ROWS 3
#define MIN_BET 15
#define DEALER_HOLD_VALUE 17

typedef struct deck
{
	int *deck;
	int total_cards;
} deck;

typedef struct hand
{
	int hand[MAX_HAND_COUNT];
	int ace_count;
	int hand_value;
	int total_cards;
} hand;

// Cross-platform sleep-function. (Not mine)
void slp(int milliseconds)
{
	#ifdef _WIN32
 	Sleep(milliseconds);
 	#else
 	usleep(milliseconds*1000);
 	#endif
}

// Clears the buffer for scanf. (I do not take credit for this code).
void clear_scanf_buffer(void)
{
	int c;
	while ((c = getchar()) != '\n' && c != EOF) {};
}

// Gets any number input from the player.
int get_number_input(char *message, int min_num, int max_num, int is_money)
{
	int input, input_loop = 1, iterations = 0;

	while (input_loop)
	{
		if (iterations == 0)
		{
			if (is_money)
			{
				printf("%s ($%d - $%d)\n", message, min_num, max_num);
			}
			else
			{
				printf("%s (%d - %d)\n", message, min_num, max_num);
			}
		}

		iterations++;

		scanf("%d", &input);

		// If the player enters a number out of bounds, restarts the loop.
		if (input >= (max_num + 1) || input <= (min_num - 1))
		{
			if (is_money)
			{
				printf("Please enter an amount that includes or is between $%d and $%d.\n", min_num, max_num);
			}
			else
			{
				printf("Please enter a number that includes or is between %d and %d.\n", min_num, max_num);
			}

			// If the player enters a character instead of a number, the loop mess up.
			// This function clears the buffer in order to prevent this.
			clear_scanf_buffer();

			continue;
		}

		// Exit the loop.
		input_loop = 0;
	}

	return input;
}

// Inserts the correct values into the play and used deck.
void create_decks(deck *play_deck, deck *used_deck, int total_cards)
{
	int i;

	// Zero out the used deck.
	for (i = 0; i < total_cards; i++)
	{
		used_deck->deck[i] = 0;
	}

	// Populates the deck with numbers in ascending order from 1 to the total card count.
	for (i = 0; i < total_cards; i++)
	{
		play_deck->deck[i] = (i + 1);
	}
}

// Swaps two pointer values.
void swap_pointers(int *a, int *b)
{
	int temp = *a;
	*a = *b;
	*b = temp;
}

// Fisher-Yates shuffle algorithm.
void shuffle_deck(deck *play_deck, int n)
{
	int *array = play_deck->deck, i, j;

	// Seed the time.
	srand(time(NULL));

	// Starts with the highest index and picks a random index in the array to swap with.
	for (i = (n - 1); i > 0; i--)
	{
		j = rand() % (i + 1);
		swap_pointers(&array[i], &array[j]);
	}
}

// Merges the used deck into the play deck.
void recombine_decks(deck *play_deck, deck *used_deck)
{
	int i, total_cards = used_deck->total_cards;

	// Moves items from the used deck into the playing deck, zeroing out the moved cards in the used deck array.
	for (i = 0; i < total_cards; i++)
	{
		play_deck->deck[play_deck->total_cards] = used_deck->deck[i];
		(play_deck->total_cards)++;
		used_deck->deck[i] = 0;
		(used_deck->total_cards)--;
	}
}

void get_hand_value(hand *hand)
{
	int i, c_num, temp;

	// Resets the hand and ace count in the deck to zero.
	(hand->hand_value) = 0;
	(hand->ace_count) = 0;

	// Finds the value of each card in the deck and adds it to the total.
	// Also keeps track of all the aces that appear in the deck.
	for (i = 0; i < hand->total_cards; i++)
	{
		c_num = hand->hand[i];

		c_num = c_num % CARDS_IN_A_DECK;

		if ((c_num == 0) || (c_num >= 11 && c_num <= 13) || (c_num >= 24 && c_num <= 26) || (c_num >= 37 && c_num <= 39) || (c_num >= 50 && c_num <= 51))
		{
			temp = 10;
		}
		else if (c_num > 1 && c_num < 11)
		{
			temp = c_num;
		}
		else if (c_num > 14 && c_num < 24)
		{
			temp = (c_num - 13);
		}
		else if (c_num > 27 && c_num < 37)
		{
			temp = (c_num - 26);
		}
		else if (c_num > 40 && c_num < 50)
		{
			temp = (c_num - 39);
		}
		else
		{
			temp = 11;
			(hand->ace_count)++;
		}

		(hand->hand_value) += temp;
	}

	// Creates a temporary value of the total hand "worth".
	temp = hand->hand_value;

	// Finds any aces within the deck and adjusts the overall value of the hand
	// if a player has aces that can lower its value.
	// The deck's value is only lowered if it needs to. (AKA the card would put the player over 21)
	if ((temp > 21) && (hand->ace_count > 0))
	{
		for (i = 0; i < hand->ace_count; i++);
		{
			if (temp > 21)
			{
				temp -= 10;
			}
		}

		(hand->hand_value) = temp;
	}
}

void disgard_hands(deck *used_deck, hand *dealer, hand *player)
{
	int i;

	// Moves all of the dealers cards over to the used deck.
	for (i = 0; i < dealer->total_cards; i++)
	{
		used_deck->deck[(used_deck->total_cards)] = dealer->hand[i];
		(used_deck->total_cards)++;
		dealer->hand[i] = 0;
	}
	(dealer->total_cards) = 0;

	// Moves all of the players cards over to the used deck.
	for (i = 0; i < player->total_cards; i++)
	{
		used_deck->deck[used_deck->total_cards] = player->hand[i];
		(used_deck->total_cards)++;
		player->hand[i] = 0;
	}
	(player->total_cards) = 0;
}

/*==============================================================================
--------------------------------------------------------------------------------
------------------ START OF GRAPHICS AND ASCII ART FUNCTIONS -------------------
--------------------------------------------------------------------------------
==============================================================================*/

// Clears the screen. May or may not work on all terminal interfaces.
void cls(void)
{
	#ifdef _WIN32
	system("cls");
	#else
	printf("\e[1;1H\e[2J");
	#endif
}

void print_blackjack_ascii_art(void)
{
	#ifdef _WIN32
	printf("-------------------------------------\n");
	printf("          Simple Blackjack\n");
	printf("-------------------------------------\n");
	#else
	printf("  ____  _                 _        ____  _            _     _            _    \n");
	printf(" / ___|(_)_ __ ___  _ __ | | ___  | __ )| | __ _  ___| | __(_) __ _  ___| | __\n");
	printf(" \\___ \\| | '_ ` _ \\| '_ \\| |/ _ \\ |  _ \\| |/ _` |/ __| |/ /| |/ _` |/ __| |/ /\n");
	printf("  ___) | | | | | | | |_) | |  __/ | |_) | | (_| | (__|   < | | (_| | (__|   < \n");
	printf(" |____/|_|_| |_| |_| .__/|_|\\___| |____/|_|\\__,_|\\___|_|\\_\\/ |\\__,_|\\___|_|\\_\\\n");
	printf("                   |_|                                   |__/                 \n\n");
	#endif
}

void print_game_win(void)
{
	#ifdef _WIN32
	printf("-------------------------------------\n");
	printf("              You Win!\n");
	printf("-------------------------------------\n");
	#else
	printf(" __      __                          __       __  __           \n");
	printf("|  \\    /  \\                        |  \\  _  |  \\|  \\          \n");
	printf(" \\$$\\  /  $$______   __    __       | $$ / \\ | $$ \\$$ _______  \n");
	printf("  \\$$\\/  $$/      \\ |  \\  |  \\      | $$/  $\\| $$|  \\|       \\ \n");
	printf("   \\$$  $$|  $$$$$$\\| $$  | $$      | $$  $$$\\ $$| $$| $$$$$$$\\\n");
	printf("    \\$$$$ | $$  | $$| $$  | $$      | $$ $$\\$$\\$$| $$| $$  | $$\n");
	printf("    | $$  | $$__/ $$| $$__/ $$      | $$$$  \\$$$$| $$| $$  | $$\n");
	printf("    | $$   \\$$    $$ \\$$    $$      | $$$    \\$$$| $$| $$  | $$\n");
	printf("     \\$$    \\$$$$$$   \\$$$$$$        \\$$      \\$$ \\$$ \\$$   \\$$\n\n");
	#endif
}

void print_game_lose(void)
{
	#ifdef _WIN32
	printf("-------------------------------------\n");
	printf("            Game Over!\n");
	printf("-------------------------------------\n");
	#else
	printf("   ____                         ___                 \n");
	printf("  / ___| __ _ _ __ ___   ___   / _ \\__   _____ _ __ \n");
	printf(" | |  _ / _` | '_ ` _ \\ / _ \\ | | | \\ \\ / / _ \\ '__|\n");
	printf(" | |_| | (_| | | | | | |  __/ | |_| |\\ V /  __/ |   \n");
	printf("  \\____|\\__,_|_| |_| |_|\\___|  \\___/  \\_/ \\___|_|   \n\n");
	#endif
}

// Gets the symbol shown in the middle of the card for the specific suite.
char *get_suite_symbol(int c_num)
{
	// Bring the range of card values between 0 - 51.
	c_num = c_num % CARDS_IN_A_DECK;

	// Returns the respective suite symbol in the 0 - 51 card value range.
	if ((c_num == 0) || (c_num >= 41 && c_num <= 51))
	{
		#ifdef _WIN32
		return "S";
		#else
		return "♠";
		#endif
	}
	else if (c_num >= 1 && c_num <= 13)
	{
		#ifdef _WIN32
		return "C";
		#else
		return "♣";
		#endif
	}
	else if (c_num >= 14 && c_num <= 26)
	{
		#ifdef _WIN32
		return "D";
		#else
		return "♦";
		#endif
	}
	else if (c_num >= 27 && c_num <= 40)
	{
		#ifdef _WIN32
		return "H";
		#else
		return "♥";
		#endif
	}
	// Should never execute the else part of the block, but its here just in case.
	else
	{
		return "ERR";
	}
}

// Gets the symbol that all cards have in the top left and bottom right of the card.
void get_card_symbol(int c_num, char *symbol)
{
	int i;

	// Bring the range of card values between 0 - 51.
	c_num = c_num % CARDS_IN_A_DECK;

	if (c_num % 13 == 0)
	{
		symbol[0] = 'K';
	}
	else if (c_num % 13 == 12)
	{
		symbol[0] = 'Q';
	}
	else if (c_num % 13 == 11)
	{
		symbol[0] = 'J';
	}
	else if (c_num % 13 == 10)
	{
		symbol[0] = '1';
		symbol[1] = '0';
		symbol[2] = '\0';
		return;
	}
	else if (c_num % 13 == 1)
	{
		symbol[0] = 'A';
	}
	else
	{
		symbol[0] = '0' + (c_num % 13);
	}

	symbol[1] = '\0';
}

// Prints the top segment of a card.
void print_top_segment(int iterations)
{
	int i;

	for (i = 0; i < iterations; i++)
	{
		#ifdef _WIN32
		printf("-----------");
		#else
		printf("┌─────────┐");
		#endif
	}

	printf("\n");
}

// Prints a segment of the card with nothing special in it.
void print_empty_segment(int iterations)
{
	int i;

	for (i = 0; i < iterations; i++)
	{
		#ifdef _WIN32
		printf("|         |");
		#else
		printf("│         │");
		#endif
	}

	printf("\n");
}

// Prints the segment of the card with a symbol in the middle.
void print_suite_segment(hand *hand, int iterations, int start_index)
{
	int i;

	for (i = 0; i < iterations; i++)
	{
		#ifdef _WIN32
		printf("|    %s    |", get_suite_symbol(hand->hand[start_index]));
		#else
		printf("│    %s    │", get_suite_symbol(hand->hand[start_index]));
		#endif
		start_index++;
	}

	printf("\n");
}

// Prints the segment of a card with a symbol in the top left or bottom right.
void print_symbol_segment(hand *hand, int left_bound, int iterations, int start_index, char *symbol)
{
	int i;

	for (i = 0; i < iterations; i++)
	{
		get_card_symbol((hand->hand[start_index]), symbol);

		if (left_bound)
		{
			#ifdef _WIN32
			printf("|%-9s|", symbol);
			#else
			printf("│%-9s│", symbol);
			#endif
		}
		else
		{
			#ifdef _WIN32
			printf("|%9s|", symbol);
			#else
			printf("│%9s│", symbol);
			#endif
		}

		start_index++;
	}

	printf("\n");
}

// Prints the bottom segment of a card.
void print_bottom_segment(int iterations)
{
	int i;

	for (i = 0; i < iterations; i++)
	{
		#ifdef _WIN32
		printf("-----------");
		#else
		printf("└─────────┘");
		#endif
	}

	printf("\n");
}

// Prints an unhidden row of cards to the screen.
void print_unhidden_card_row(hand *hand, int *cards, int start_index, char *symbol, int i)
{
	print_top_segment(cards[i]);
	print_symbol_segment(hand, 1, cards[i], start_index, symbol);
	print_empty_segment(cards[i]);
	print_empty_segment(cards[i]);
	print_suite_segment(hand, cards[i], start_index);
	print_empty_segment(cards[i]);
	print_empty_segment(cards[i]);
	print_symbol_segment(hand, 0, cards[i], start_index, symbol);
	print_bottom_segment(cards[i]);
}

// Main function that handles the printing of cards to the screen.
void print_cards(deck *play_deck, deck *used_deck, hand *hand, int first_card_hidden)
{
	char *symbol;
	int i, row, start_index, iterations, temp_card_total, cards[NUMBER_OF_ROWS];

	// Prints the "no cards" element to the screen.
	if (hand->total_cards == 0)
	{
		#ifdef _WIN32
		printf("- - - - - -\n");
		printf("           \n");
		printf("|         |\n");
		printf("           \n");
		printf("|         |\n");
		printf("           \n");
		printf("|         |\n");
		printf("           \n");
		printf("- - - - - -\n");
		#else
		printf("┌ ─ ─ ─ ─ ┐\n");
		printf("           \n");
		printf("│         │\n");
		printf("           \n");
		printf("│         │\n");
		printf("           \n");
		printf("│         │\n");
		printf("           \n");
		printf("└ ─ ─ ─ ─ ┘\n");
		#endif
		return;
	}
	else
	{
		symbol = malloc(sizeof(char) * (MAX_SYMBOL_LENGTH));

		if (symbol == NULL)
		{
			// Exit the game.
			free(used_deck->deck);
			free(play_deck->deck);
			printf("ERROR: Failed to allocate memory in heap space for variable 'symbol' - 01.\n");
			exit(1);
		}

		// Sets all the characters in the symbol string to the null terminator.
		for (i = 0; i < MAX_SYMBOL_LENGTH; i++)
		{
			symbol[i] = '\0';
		}

		// Initializes all the rows in the cards array to 0.
		for (i = 0; i < NUMBER_OF_ROWS; i++)
		{
			cards[i] = 0;
		}

		// Adds cards to each row until the row surpasses the maximum number of cards allowed in the row.
		// Wraps to another row when one becomes full.
		for (temp_card_total = (hand->total_cards), row = 0; temp_card_total > 0; temp_card_total--)
		{
			if (cards[row] > (MAX_CARDS_IN_A_ROW - 1))
			{
				row++;
			}

			cards[row]++;
		}

		// Add 1 to the row variable to get the total number of rows instead of the highest row index.
		row++;

		// Handles the main printing of cards.
		for (i = 0; i < row; i++)
		{
			// Get the start index for each row of the cards array.
			start_index = i * MAX_CARDS_IN_A_ROW;

			if (first_card_hidden)
			{
				// Row 1 has a hidden card while the rest of the rows don't.
				if (start_index == 0)
				{
					#ifdef _WIN32
					print_top_segment(cards[i]);
					printf("|XXXXXXXXX|");
					print_symbol_segment(hand, 1, (cards[i] - 1), start_index + 1, symbol);
					printf("|XXXXXXXXX|");
					print_empty_segment((cards[i]) - 1);
					printf("|XXXXXXXXX|");
					print_empty_segment((cards[i]) - 1);
					printf("|XXXXXXXXX|");
					print_suite_segment(hand, (cards[i] - 1), start_index + 1);
					printf("|XXXXXXXXX|");
					print_empty_segment((cards[i]) - 1);
					printf("|XXXXXXXXX|");
					print_empty_segment((cards[i]) - 1);
					printf("|XXXXXXXXX|");
					print_symbol_segment(hand, 0, (cards[i] - 1), start_index + 1, symbol);
					print_bottom_segment(cards[i]);
					#else
					print_top_segment(cards[i]);
					printf("│░░░░░░░░░│");
					print_symbol_segment(hand, 1, (cards[i] - 1), start_index + 1, symbol);
					printf("│░░░░░░░░░│");
					print_empty_segment((cards[i]) - 1);
					printf("│░░░░░░░░░│");
					print_empty_segment((cards[i]) - 1);
					printf("│░░░░░░░░░│");
					print_suite_segment(hand, (cards[i] - 1), start_index + 1);
					printf("│░░░░░░░░░│");
					print_empty_segment((cards[i]) - 1);
					printf("│░░░░░░░░░│");
					print_empty_segment((cards[i]) - 1);
					printf("│░░░░░░░░░│");
					print_symbol_segment(hand, 0, (cards[i] - 1), start_index + 1, symbol);
					print_bottom_segment(cards[i]);
					#endif
				}
				else
				{
					print_unhidden_card_row(hand, cards, start_index, symbol, i);
				}
			}
			else
			{
				print_unhidden_card_row(hand, cards, start_index, symbol, i);
			}
		}

		// Free the memory.
		free(symbol);
	}
}

void blackjack_ui(deck *play_deck, deck *used_deck, hand *player, hand *dealer, int money, int win_amount, int hidden, int bet)
{
	cls();

	printf("Money: $%d\n", money);
	printf("Minimum bet: $%d\n", MIN_BET);
	printf("Current bet: $%d\n", bet);
	printf("Required money to win: $%d\n", win_amount);
	printf("\n");
	printf("Note: You will lose if your money drops below the minimum bet.\n\n");
	#ifdef _WIN32
	printf("==============================================================\n\n");
	#else
	printf("♠============================================================♣\n\n");
	#endif
	printf("Dealer's cards:\n");

	// Dealer's cards.
	print_cards(play_deck, used_deck, dealer, hidden);

	// To prevent the first player card from being hidden.
	hidden = 0;

	printf("\n");
	printf("Your cards:\n");

	// Player's cards.
	print_cards(play_deck, used_deck, player, hidden);

	printf("\n");

	#ifdef _WIN32
	printf("==============================================================\n\n");
	#else
	printf("♦============================================================♥\n\n");
	#endif
}

/*==============================================================================
--------------------------------------------------------------------------------
------------------ END OF GRAPHICS AND ASCII ART FUNCTIONS ---------------------
--------------------------------------------------------------------------------
==============================================================================*/

// Main game function.
int blackjack(deck *play_deck, deck *used_deck, hand *player, hand *dealer, int *money, int win_amount, int num_decks)
{
	char input;
	int bet, lost_bet, current_index, i, j, dealer_hidden, player_active, blackjack, reshuffled, game_active = 1;

	while (game_active)
	{
		// Set up variables for this turn.
		dealer_hidden = 1;
		player_active = 1;
		bet = 0;
		lost_bet = 0;
		blackjack = 0;
		reshuffled = 0;

		// Player loses if he has no more money to bet.
		if (*money < MIN_BET)
		{
			return 0;
		}

		// Any call to this function will update the UI.
		blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

		// Gets the player's bet amount.
		while (player_active)
		{
			bet = get_number_input("How much money do you want to bet? There is no need for a dollar sign.", MIN_BET, (MAX_MONEY * DIFFICULTY_MULTIPLIER_3), 1);

			if (bet > *money)
			{
				printf("You don't have that much money!\n");
			}
			else
			{
				player_active = 0;
			}

			clear_scanf_buffer();
		}

		*money = *money - bet;

		// Used for keeping track of where to draw from.
		current_index = play_deck->total_cards - 1;

		// Handles the initial drawing of the cards.
		// Alternates between drawing a card for the player and dealer.
		for (i = INITIAL_CARD_DRAW, j = 0; i > 0; i--)
		{
			// Handles the increasing of indexes in the dealer and player hand.
			if (i == INITIAL_CARD_DRAW / 2)
			{
				j++;
			}

			blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

			// Recombine and shuffle the used deck into the playing deck if no cards are in it.
			if (play_deck->total_cards == 0)
			{
				recombine_decks(play_deck, used_deck);
				shuffle_deck(play_deck, play_deck->total_cards);
				current_index = (play_deck->total_cards - 1);
				reshuffled = 1;
			}

			// Alternating if-else statement for adding cards to the hands.
			if (i % 2 == 0)
			{
				player->hand[j] = play_deck->deck[current_index];
				(player->total_cards)++;
				play_deck->deck[current_index] = 0;
				(play_deck->total_cards)--;
				current_index--;
			}
			else
			{
				dealer->hand[j] = play_deck->deck[current_index];
				(dealer->total_cards)++;
				play_deck->deck[current_index] = 0;
				(play_deck->total_cards)--;
				current_index--;
			}

			// Any call to this function will sleep for the standard sleep time defined at the top.
			// Only reason for this is to add some effect of card drawing.
			// Otherwise, the cards would appear instantly.
			slp(STANDARD_SLEEP_TIME);
		}

		blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

		if (reshuffled)
		{
			printf("The deck was reshuffled during the initial draw.\n\n");
			reshuffled = 0;
		}

		// Recombine and shuffle the used deck into the playing deck if no cards are in it.
		if (play_deck->total_cards == 0)
		{
			recombine_decks(play_deck, used_deck);
			shuffle_deck(play_deck, play_deck->total_cards);
			current_index = (play_deck->total_cards - 1);
		}

		slp(STANDARD_SLEEP_TIME);

		// Gets the hand value of the player and dealer.
		get_hand_value(dealer);
		get_hand_value(player);

		player_active = 1;

		// Both the dealer and player have blackjacks.
		if ((dealer->hand_value == 21) && (player->hand_value == 21))
		{
			dealer_hidden = 0;

			blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

			printf("Both the dealer and player got blackjacks! You get your $%d back!\n", bet);

			// Disgard the cards of the player and dealer into the used deck.
			disgard_hands(used_deck, dealer, player);

			// Return the money back to the player.
			*money = *money + bet;

			slp(STANDARD_SLEEP_TIME * 8);

			// Starts a new round.
			continue;
		}
		// The dealer got 21 and player did not, automatic win for him.
		else if (dealer->hand_value == 21)
		{
			dealer_hidden = 0;

			blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

			printf("Dealer got a blackjack! You lost the bet of $%d!\n", bet);

			// Disgard the cards of the player and dealer into the used deck.
			disgard_hands(used_deck, dealer, player);

			slp(STANDARD_SLEEP_TIME * 8);

			// Starts a new round.
			continue;
		}
		// Player got a blackjack. No need for him to do anything else.
		else if (player->hand_value == 21)
		{
			blackjack = 1;
			player_active = 0;
		}

		// Player gets to choose what to do.
		while (player_active)
		{
			printf("Commands: (h)it (s)tand (e)xit\n\n");
			printf("What would you like to do?\n");

			scanf(" %c", &input);

			if (!isalpha(input))
			{
				printf("Please input an alphabetical letter.\n");
			}
			else
			{
				input = tolower(input);

				if (input == 'e')
				{
					printf("Exiting the game...\n");
					return -1;
				}
				else if (input == 'h')
				{
					// Grabs a card from the deck.
					player->hand[player->total_cards] = play_deck->deck[current_index];
					(player->total_cards)++;
					play_deck->deck[current_index] = 0;
					(play_deck->total_cards)--;
					current_index--;

					// Updates the player's total card values.
					get_hand_value(player);

					blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

					// Recombine and shuffle the used deck into the playing deck if no cards are in it.
					if (play_deck->total_cards == 0)
					{
						recombine_decks(play_deck, used_deck);
						shuffle_deck(play_deck, play_deck->total_cards);
						current_index = (play_deck->total_cards - 1);
						reshuffled = 1;
					}


					// Busts the player if he goes above 21.
					if (player->hand_value > 21)
					{
						player_active = 0;
						dealer_hidden = 0;
						lost_bet = 1;

						blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

						if (reshuffled)
						{
							printf("The deck has been reshuffled.\n\n");
							reshuffled = 0;
						}

						printf("Dealer's total cards value: %d\n", dealer->hand_value);
						printf("Your total cards value: %d\n\n", player->hand_value);
						printf("You busted and lost the bet of $%d!\n", bet);

						// Disgard the cards of the player and dealer into the used deck.
						disgard_hands(used_deck, dealer, player);

						slp(STANDARD_SLEEP_TIME * 8);
					}

					if (reshuffled)
					{
						printf("The deck has been reshuffled.\n\n");
						reshuffled = 0;
					}

					// Player got a blackjack, no more need to draw.
					if ((player->hand_value) == 21)
					{
						player_active = 0;
					}
				}
				else if (input == 's')
				{
					player_active = 0;
				}
				else
				{
					printf("Letter '%c' is not a recognized command.\n", input);
				}
			}

			clear_scanf_buffer();
		}

		// Starts a new turn is the player busted while drawing.
		if (lost_bet)
		{
			continue;
		}

		dealer_hidden = 0;

		blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

		slp(STANDARD_SLEEP_TIME);

		// Dealer draws until he reaches or is above the hold threshold.
		while (dealer->hand_value < DEALER_HOLD_VALUE)
		{
			// Dealer draws a card.
			dealer->hand[dealer->total_cards] = play_deck->deck[current_index];
			(dealer->total_cards)++;
			play_deck->deck[current_index] = 0;
			(play_deck->total_cards)--;
			current_index--;

			// Updates the dealer's total card values.
			get_hand_value(dealer);

			blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

			// Recombine and shuffle the used deck into the playing deck if no cards are in it.
			if (play_deck->total_cards == 0)
			{
				recombine_decks(play_deck, used_deck);
				shuffle_deck(play_deck, play_deck->total_cards);
				current_index = (play_deck->total_cards - 1);
				reshuffled = 1;
			}

			slp(STANDARD_SLEEP_TIME);
		}

		blackjack_ui(play_deck, used_deck, player, dealer, *money, win_amount, dealer_hidden, bet);

		if (reshuffled)
		{
			printf("The deck was reshuffled during the dealers draw.\n\n");
			reshuffled = 0;
		}

		slp(STANDARD_SLEEP_TIME);

		// Dealer drew over 21. He busts.
		if (dealer->hand_value > 21)
		{
			printf("Dealer hand value: %d\n", dealer->hand_value);
			printf("Player hand value: %d\n\n", player->hand_value);
			printf("The dealer busted! You won $%d!\n", (bet * 2));
			*money = *money + (bet * 2);
		}
		// Push, both hand values were the same. Player gets origional bet money back.
		else if (dealer->hand_value == player->hand_value)
		{
			printf("Push! You get your $%d back!\n", bet);
			*money = *money + bet;
		}
		// Player lost because the dealer had a higher hand value.
		else if (dealer->hand_value > player->hand_value)
		{
			printf("Dealer hand value: %d\n", dealer->hand_value);
			printf("Player hand value: %d\n\n", player->hand_value);
			printf("You lost $%d!\n", bet);
		}
		// Player had the higher hand value.
		else
		{
			printf("Dealer hand value: %d\n", dealer->hand_value);
			printf("Player hand value: %d\n\n", player->hand_value);

			// If the player gets a blackjack, he gets 1.5x his bet money. Otherwise he gets 2x.
			if (blackjack)
			{
				*money = (int)(*money + ((double)bet * 1.5));
				printf("You won $%d!\n", (int)((double)bet * 1.5));
			}
			else
			{
				*money = *money + (bet * 2);
				printf("You won $%d!\n", (bet * 2));
			}
		}

		slp(STANDARD_SLEEP_TIME * 8);

		// Disgard the cards of the player and dealer into the used deck.
		disgard_hands(used_deck, dealer, player);

		// Player achieved the amount of money required to win.
		if (*money >= win_amount)
		{
			return 1;
		}
	}
}

int main(void)
{
	char input, f_money[15];
	int i, j, money, difficulty, win_amount, num_decks, menu_loop, win, settings_loop = 1, iteration = 0, *dynamic_memory_location;
	deck play_deck, used_deck, duplicate_deck;
	hand dealer, player;

	while (settings_loop)
	{
		// Makes sure the menu loop will activate.
		menu_loop = 1;

		cls();
		print_blackjack_ascii_art();

		// Set up deck information.
		num_decks = get_number_input("Enter the number of decks to use.", MIN_DECK_COUNT, MAX_DECK_COUNT, 0);
		play_deck.total_cards = num_decks * CARDS_IN_A_DECK;
		used_deck.total_cards = 0;

		// Set aside memory for the used and play deck.
		if (iteration == 0)
		{
			// Create memory for the play deck.
			dynamic_memory_location = malloc(sizeof(int) * (num_decks * CARDS_IN_A_DECK));
			if (dynamic_memory_location == NULL)
			{
				printf("ERROR: Failed to allocate memory in heap space for variable 'memory_alloc' - 01.\n");
				return 1;
			}
			play_deck.deck = dynamic_memory_location;

			// Create memory for the used deck.
			dynamic_memory_location = malloc(sizeof(int) * (num_decks * CARDS_IN_A_DECK));
			if (dynamic_memory_location == NULL)
			{
				printf("ERROR: Failed to allocate memory in heap space for variable 'memory_alloc' - 02.\n");
				free(play_deck.deck);
				return 1;
			}
			used_deck.deck = dynamic_memory_location;
		}
		else
		{
			// Free the previously allocated memory to be reallocated.
			free(play_deck.deck);
			free(used_deck.deck);

			// Create memory for the play deck.
			dynamic_memory_location = malloc(sizeof(int) * (num_decks * CARDS_IN_A_DECK));
			if (dynamic_memory_location == NULL)
			{
				printf("ERROR: Failed to allocate memory in heap space for variable 'memory_alloc' - 11.\n");
				return 1;
			}
			play_deck.deck = dynamic_memory_location;

			// Create memory for the used deck.
			dynamic_memory_location = malloc(sizeof(int) * (num_decks * CARDS_IN_A_DECK));
			if (dynamic_memory_location == NULL)
			{
				printf("ERROR: Failed to allocate memory in heap space for variable 'memory_alloc' - 12.\n");
				free(play_deck.deck);
				return 1;
			}
			used_deck.deck = dynamic_memory_location;
		}

		iteration++;

		// Creates the cards within the deck and shuffles them.
		create_decks(&play_deck, &used_deck, play_deck.total_cards);
		shuffle_deck(&play_deck, play_deck.total_cards);

		// Set up the player and dealer hands.
		player.total_cards = 0;
		dealer.total_cards = 0;

		// Set the player's hand to zeros.
		for (i = 0; i < MAX_HAND_COUNT; i++)
		{
			player.hand[i] = 0;
		}

		// Set the dealer's hand to zeros.
		for (i = 0; i < MAX_HAND_COUNT; i++)
		{
			dealer.hand[i] = 0;
		}

		cls();
		print_blackjack_ascii_art();

		// Get starting money from the player.
		money = get_number_input("Select your starting money. There is no need for a dollar sign.", MIN_MONEY, MAX_MONEY, 1);

		cls();
		print_blackjack_ascii_art();

		// Get difficulty from the player.
		printf("The difficulty dictates the money amount you must reach to win the game.\n");
		printf("   (1) - %dx multiplier to starting money.\n", DIFFICULTY_MULTIPLIER_1);
		printf("   (2) - %dx multiplier to starting money.\n", DIFFICULTY_MULTIPLIER_2);
		printf("   (3) - %dx multiplier to starting money.\n", DIFFICULTY_MULTIPLIER_3);
		difficulty = get_number_input("Select a difficulty.", MIN_DIFFICULTY, MAX_DIFFICULTY, 0);

		// There shouldn't be any case in which the "else" would be executed.
		// Translates the difficulty setting to the win amount needed. (in terms of money)
		if (difficulty == 1)
		{
			win_amount = DIFFICULTY_MULTIPLIER_1 * money;
		}
		else if (difficulty == 2)
		{
			win_amount = DIFFICULTY_MULTIPLIER_2 * money;
		}
		else if (difficulty == 3)
		{
			win_amount = DIFFICULTY_MULTIPLIER_3 * money;
		}
		else
		{
			win_amount = DIFFICULTY_MULTIPLIER_1 * money;
		}

		cls();
		print_blackjack_ascii_art();

		// Development information.
		printf("Developed by: Chase Chambliss\n");
		printf("ASCII word art from: http://patorjk.com\n\n");

		// Note for the player.
		printf("Notes:\n");
		printf(" - This is simple blackjack. There is no insurance, splitting, or doubling down.\n");
		printf(" - The dealer will draw until he has 17 or higher.\n");
		printf(" - This game utilizes a 'virtual' deck. Every card that appears on the screen is not randomly generated upon draw.\n\n");

		// Settings verification display.
		#ifdef _WIN32
		printf("=================================================\n");
		#else
		printf("♠===♦===♥===♣===♠===♦===♥===♣===♠===♦===♥===♣===♠\n");
		#endif
		printf("------------------ SETTINGS ---------------------\n");
		printf("Number of decks: %d\n", (play_deck.total_cards) / 52);
		printf("Total number of cards: %d\n", play_deck.total_cards);
		printf("Difficulty: %d\n", difficulty);
		printf("Starting money: $%d\n", money);
		printf("Required money to win: $%d\n", win_amount);
		printf("Minimum bet (Can't be changed): $%d\n", MIN_BET);
		#ifdef _WIN32
		printf("=================================================\n");
		#else
		printf("♠===♦===♥===♣===♠===♦===♥===♣===♠===♦===♥===♣===♠\n");
		#endif
		printf("Commands: (e)xit (y)es (n)o (r)eshuffle\n\n");
		printf("Note: '(n)o' will restart the settings creation process.\n");
		printf("Are you fine with the settings and ready to begin?\n");

		// Handles the player inputs in reponse to the menu.
		while (menu_loop)
		{
			scanf(" %c", &input);

			// Checks to see if the input character is alphabetical or not.
			if (!isalpha(input))
			{
				printf("Please input an alphabetical letter.\n");
			}
			else
			{
				input = tolower(input);

				// User wants to exit the game.
				if (input == 'e')
				{
					printf("Exiting the game...\n");

					// Closes the game.
					free(used_deck.deck);
					free(play_deck.deck);
					return 0;
				}
				// User wants to reshuffle the deck.
				else if (input == 'r')
				{
					// Create memory for the duplicate deck.
					dynamic_memory_location = malloc(sizeof(int) * (num_decks * CARDS_IN_A_DECK));
					if (dynamic_memory_location == NULL)
					{
						printf("ERROR: Failed to allocate memory in heap space for variable 'memory_alloc' - 13.\n");
						return 1;
					}
					duplicate_deck.deck = dynamic_memory_location;

					// Copy the play deck cards into the duplicate deck.
					for (i = 0; i < play_deck.total_cards; i++)
					{
						duplicate_deck.deck[i] = play_deck.deck[i];
					}

					shuffle_deck(&play_deck, play_deck.total_cards);

					// Check for changes between indexes.
					for (i = 0, j = 0; i < play_deck.total_cards; i++)
					{
						if (duplicate_deck.deck[i] != play_deck.deck[i])
						{
							j++;
						}
					}

					// Report of changes.
					printf("The deck has been reshuffled.\n");
					printf("%d cards have changed position in the deck.\n", j);

					free(duplicate_deck.deck);
				}
				else if (input == 'n')
				{
					menu_loop = 0;
				}
				else if (input == 'y')
				{
					settings_loop = 0;
					menu_loop = 0;
				}
				else
				{
					printf("Letter '%c' is not a recognized command.\n", input);
				}
			}

			clear_scanf_buffer();
		}
	}

	cls();

	win = blackjack(&play_deck, &used_deck, &player, &dealer, &money, win_amount, num_decks);

	cls();

	if (win != -1)
	{
		if (win)
		{
			print_game_win();
			printf("Congratulations! You walk out of the casino with $%d.\n", money);
		}
		else
		{
			print_game_lose();
			printf("You fell below the minimum bet requirement!\n");
			printf("Your money: $%d\n", money);
			printf("Minimum bet: $%d\n\n", MIN_BET);
			printf("Better luck next time!\n");
		}
	}

	// Free the allocated memory.
	free(used_deck.deck);
	free(play_deck.deck);

	return 0;
}
