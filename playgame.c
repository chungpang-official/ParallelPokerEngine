#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

// Maximum number of cards in the deck
#define MAX_CARDS 52
// Maximum number of players
#define MAX_PLAYERS 10

// Structure to represent a card
typedef struct {
    char suit;  // 'S', 'H', 'C', 'D'
    char rank;  // '2', 'A', 'K', ..., '3'
} Card;

// Convert rank character to numerical value for comparison
int rank_to_value(char rank) {
    switch (rank) {
        case '2': return 13;
        case 'A': return 12;
        case 'K': return 11;
        case 'Q': return 10;
        case 'J': return 9;
        case 'T': return 8;
        case '9': return 7;
        case '8': return 6;
        case '7': return 5;
        case '6': return 4;
        case '5': return 3;
        case '4': return 2;
        case '3': return 1;
        default: return 0;  // Should not happen
    }
}

// Convert suit to numerical value for sorting (D, C, H, S)
int suit_to_value(char suit) {
    switch (suit) {
        case 'D': return 0;
        case 'C': return 1;
        case 'H': return 2;
        case 'S': return 3;
        default: return 4;  // Should not happen
    }
}

// Compare two cards for sorting (by suit, then by rank)
int compare_cards_for_sort(Card card1, Card card2) {
    int suit1 = suit_to_value(card1.suit);
    int suit2 = suit_to_value(card2.suit);
    if (suit1 != suit2) return suit1 - suit2;
    int val1 = rank_to_value(card1.rank);
    int val2 = rank_to_value(card2.rank);
    return val1 - val2;  // Ascending order within suit
}

// Compare two cards for gameplay (by rank only, returns 1 if card1 > card2)
int compare_cards(Card card1, Card card2) {
    int val1 = rank_to_value(card1.rank);
    int val2 = rank_to_value(card2.rank);
    return val1 > val2;
}

// Convert card to string for printing
void card_to_string(Card card, char *str) {
    str[0] = card.suit;
    str[1] = card.rank;
    str[2] = '\0';
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <number_of_players>\n", argv[0]);
        exit(1);
    }

    // Number of players
    int n = atoi(argv[1]);
    if (n < 1 || n > MAX_PLAYERS) {
        printf("Number of players must be between 1 and %d\n", MAX_PLAYERS);
        exit(1);
    }

    // Read cards from standard input
    char card_str[3];
    Card deck[MAX_CARDS];
    int deck_size = 0;
    int seen[4][13] = {0};  // To detect duplicates (suit, rank)

    while (scanf("%s", card_str) != EOF) {
        if (deck_size >= MAX_CARDS) {
            printf("Too many cards in input\n");
            exit(1);
        }

        Card card;
        card.suit = card_str[0];
        card.rank = card_str[1];

        // Map suit to index for duplicate checking
        int suit_idx;
        switch (card.suit) {
            case 'S': suit_idx = 0; break;
            case 'H': suit_idx = 1; break;
            case 'C': suit_idx = 2; break;
            case 'D': suit_idx = 3; break;
            default: continue;  // Invalid suit, skip
        }

        int rank_idx = rank_to_value(card.rank) - 1;
        if (rank_idx < 0 || rank_idx >= 13) continue;  // Invalid rank, skip

        // Check for duplicates
        if (seen[suit_idx][rank_idx]) {
            printf("Parent: duplicated card %s discarded\n", card_str);
            continue;
        }

        seen[suit_idx][rank_idx] = 1;
        deck[deck_size++] = card;
    }

    // Create pipes for each child
    int parent_to_child[MAX_PLAYERS][2];  // Pipe from parent to child
    int child_to_parent[MAX_PLAYERS][2];  // Pipe from child to parent
    pid_t pids[MAX_PLAYERS];

    for (int i = 0; i < n; i++) {
        if (pipe(parent_to_child[i]) < 0 || pipe(child_to_parent[i]) < 0) {
            printf("Pipe creation error\n");
            exit(1);
        }
    }

    // Fork children and store PIDs
    for (int i = 0; i < n; i++) {
        pids[i] = fork();
        if (pids[i] < 0) {
            printf("Fork failed\n");
            exit(1);
        }
        if (pids[i] == 0) break;  // Child exits the loop
    }

    // Print child PIDs (only in parent, once)
    if (pids[0] != 0) {  // Ensure only the parent prints this
        printf("Parent: the child players are");
        for (int i = 0; i < n; i++) {
            printf(" %d", pids[i]);
        }
        printf("\n");
    }

    // Child processes
    for (int i = 0; i < n; i++) {
        if (pids[i] == 0) {
            // Child process
            int child_id = i + 1;

            // Close unused pipe ends
            close(parent_to_child[i][1]);  // Close write end of parent-to-child
            close(child_to_parent[i][0]);  // Close read end of child-to-parent
            for (int j = 0; j < n; j++) {
                if (j != i) {
                    close(parent_to_child[j][0]);
                    close(parent_to_child[j][1]);
                    close(child_to_parent[j][0]);
                    close(child_to_parent[j][1]);
                }
            }

            // Collect cards for this child
            Card hand[MAX_CARDS];
            int hand_size = 0;
            for (int j = i; j < deck_size; j += n) {
                hand[hand_size++] = deck[j];
            }

            // Sort hand by suit, then by rank (to match sample output)
            for (int j = 0; j < hand_size - 1; j++) {
                for (int k = 0; k < hand_size - j - 1; k++) {
                    if (compare_cards_for_sort(hand[k], hand[k + 1]) > 0) {
                        Card temp = hand[k];
                        hand[k] = hand[k + 1];
                        hand[k + 1] = temp;
                    }
                }
            }

            // Print hand (twice as per sample output)
            printf("Child %d, pid %d: I have %d cards\n", child_id, getpid(), hand_size);
            char hand_str[100] = "";
            for (int j = 0; j < hand_size; j++) {
                char card_str[3];
                card_to_string(hand[j], card_str);
                strcat(hand_str, card_str);
                if (j < hand_size - 1) strcat(hand_str, " ");
            }
            printf("Child %d, pid %d: %s\n", child_id, getpid(), hand_str);
            printf("Child %d, pid %d: %s\n", child_id, getpid(), hand_str);

            // Game loop
            char message[10];
            Card last_card = {'D', '2'};  // Dummy initial card (smallest possible)
            int active = 1;
            int played_d3 = 0;  // Flag to track if D3 has been played

            while (active) {
                // Read the last played card from parent
                int n = read(parent_to_child[i][0], message, sizeof(message));
                if (n <= 0) break;  // Parent closed pipe
                message[n] = '\0';

                if (strcmp(message, "done") == 0) {
                    active = 0;
                    break;
                }

                // Parse the last played card
                if (strcmp(message, "any") != 0) {
                    last_card.suit = message[0];
                    last_card.rank = message[1];
                }

                // Find the smallest card larger than last_card
                int play_idx = -1;
                if (!played_d3) {
                    for (int j = 0; j < hand_size; j++) {
                        if (hand[j].suit == 'D' && hand[j].rank == '3') {
                            play_idx = j;
                            played_d3 = 1;
                            break;
                        }
                    }
                }
                if (play_idx == -1) {
                    if (strcmp(message, "any") == 0) {
                        // Play the smallest card in hand
                        play_idx = 0;
                        for (int j = 1; j < hand_size; j++) {
                            if (compare_cards_for_sort(hand[j], hand[play_idx]) < 0) {
                                play_idx = j;
                            }
                        }
                    } else {
                        // Find the smallest card larger than last_card
                        for (int j = 0; j < hand_size; j++) {
                            if (compare_cards(hand[j], last_card)) {
                                if (play_idx == -1 || compare_cards(hand[play_idx], hand[j])) {
                                    play_idx = j;
                                }
                            }
                        }
                    }
                }

                if (play_idx >= 0) {
                    // Play the card
                    Card played = hand[play_idx];
                    char play_msg[10];
                    card_to_string(played, play_msg);
                    write(child_to_parent[i][1], play_msg, strlen(play_msg) + 1);

                    // Remove the card from hand
                    for (int j = play_idx; j < hand_size - 1; j++) {
                        hand[j] = hand[j + 1];
                    }
                    hand_size--;

                    printf("Child %d: play %s\n", child_id, play_msg);

                    if (hand_size == 0) {
                        write(child_to_parent[i][1], "completion", 11);
                        printf("Child %d: I complete\n", child_id);
                        active = 0;
                    }
                } else {
                    // Pass
                    write(child_to_parent[i][1], "pass", 5);
                    printf("Child %d: pass\n", child_id);
                }
            }

            // Close pipes and exit
            close(parent_to_child[i][0]);
            close(child_to_parent[i][1]);
            exit(0);
        }
    }

    // Parent process
    // Close unused pipe ends
    for (int i = 0; i < n; i++) {
        close(parent_to_child[i][0]);  // Close read end of parent-to-child
        close(child_to_parent[i][1]);  // Close write end of child-to-parent
    }

    // Game loop
    int active_players = n;
    int current_player = -1;  // Will be set to the player with D3
    Card last_card = {'D', '2'};  // Dummy initial card
    int pass_count = 0;
    int finished[MAX_PLAYERS] = {0};
    int first_winner = -1;

    // Find the player with D3
    for (int i = 0; i < deck_size; i++) {
        if (deck[i].suit == 'D' && deck[i].rank == '3') {
            current_player = i % n;
            break;
        }
    }

    // Print the first play (D3)
    printf("Child %d: play D3\n", current_player + 1);
    printf("Parent: child %d plays D3\n", current_player + 1);
    last_card.suit = 'D';
    last_card.rank = '3';

    while (active_players > 1) {
        current_player = (current_player + 1) % n;
        if (finished[current_player]) continue;

        // Send the last played card to the current child
        char message[10];
        if (pass_count == active_players - 1) {
            strcpy(message, "any");  // All others passed, play any card
        } else {
            card_to_string(last_card, message);
        }
        write(parent_to_child[current_player][1], message, strlen(message) + 1);

        // Read the child's response
        char response[20];
        int n = read(child_to_parent[current_player][0], response, sizeof(response));
        if (n <= 0) break;
        response[n] = '\0';

        if (strcmp(response, "pass") == 0) {
            printf("Parent: child %d passes\n", current_player + 1);
            pass_count++;
        } else if (strcmp(response, "completion") == 0) {
            if (first_winner == -1) {
                printf("Parent: child %d is winner\n", current_player + 1);
                first_winner = current_player;
            } else {
                printf("Parent: child %d completes\n", current_player + 1);
            }
            finished[current_player] = 1;
            active_players--;
            pass_count = 0;
            if (active_players == 1) break;
        } else {
            // Child played a card
            last_card.suit = response[0];
            last_card.rank = response[1];
            printf("Parent: child %d plays %s\n", current_player + 1, response);
            pass_count = 0;
        }
    }

    // Identify the loser
    for (int i = 0; i < n; i++) {
        if (!finished[i]) {
            printf("Parent: child %d is loser\n", i + 1);
            break;
        }
    }

    // Signal children to terminate
    for (int i = 0; i < n; i++) {
        if (!finished[i]) {
            write(parent_to_child[i][1], "done", 5);
        }
    }

    // Close pipes
    for (int i = 0; i < n; i++) {
        close(parent_to_child[i][1]);
        close(child_to_parent[i][0]);
    }

    // Wait for all children to terminate
    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    return 0;
}