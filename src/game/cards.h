/* cards.h
 * Some helpers for dealing with our cards model.
 *
 * Each card has an id 0..51. That's suit 0..3 *13 + rank 0..12.
 * We'll express hands and decks as a 64-bit integer, which is a bitmap of cardid.
 */
 
#ifndef CARDS_H
#define CARDS_H

#define SUIT_HEART    0
#define SUIT_DIAMOND  1
#define SUIT_CLUB     2
#define SUIT_SPADE    3

// Note that the rank value does not line up with the displayed number.
// "two" etc are just arbitrary names.
#define RANK_ACE    0
#define RANK_TWO    1
#define RANK_THREE  2
#define RANK_FOUR   3
#define RANK_FIVE   4
#define RANK_SIX    5
#define RANK_SEVEN  6
#define RANK_EIGHT  7
#define RANK_NINE   8
#define RANK_TEN    9
#define RANK_JACK  10
#define RANK_QUEEN 11
#define RANK_KING  12

/* Some slightly overkill functions to convert between split (suit,rank), cardid, and bitmaps.
 */
int cardid_from_suit_rank(int suit,int rank);
int suit_from_cardid(int cardid);
int rank_from_cardid(int cardid);
uint64_t bit_from_cardid(int cardid);
int cardid_from_bit(uint64_t single_bit);

/* Masks of all 13 bits per suit.
 */
#define MASK_HEART    0x0000000000001fffll
#define MASK_DIAMOND  0x0000000003ffe000ll
#define MASK_CLUB     0x0000007ffc000000ll
#define MASK_SPADE    0x000fff8000000000ll

/* Simple hand queries.
 */
int hand_count_cards(uint64_t hand); // => 0..52
int hand_count_suits(uint64_t hand); // => 0..4
int hand_count_cards_of_suit(uint64_t hand,int suit); // => 0..13
int hand_has_card(uint64_t hand,int cardid);
int hand_has_suit(uint64_t hand,int suit);
uint64_t hands_overlap(uint64_t a,uint64_t b);

/* Add or remove card to hand.
 * It's just a bitwise OR or complemented AND. But we check for OOB and any 64-bit weirdness.
 */
uint64_t hand_add_cardid(uint64_t hand,int cardid);
uint64_t hand_remove_cardid(uint64_t hand,int cardid);

/* Write cardid 0..51 into (cardidv), sorted, and return the count.
 * Or hand_from_cardlist() to reverse the process.
 */
int cardlist_from_hand(uint8_t *cardidv,uint64_t hand);
uint64_t hand_from_cardlist(const uint8_t *cardidv,int cardidc);

/* Returns a subset of (hand) with exactly (suitc) bits set, in different suits.
 * The choices are uniformly random.
 */
uint64_t hand_pick_n(uint64_t hand,int suitc);

/* Returns a subset of (hand) with exactly (n) bits set.
 * Entirely random, not necessarily different suits.
 * If you ask for more than available, we return (hand) exactly.
 */
uint64_t hand_deal_n(uint64_t hand,int n);

/* Helper to log one line with a summary of the hand's content.
 */
void hand_log(const char *cmt,uint64_t hand);

#endif
