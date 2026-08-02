#include "all52.h"

/* Compose and decompose card id.
 */
 
int cardid_from_suit_rank(int suit,int rank) {
  if ((suit<0)||(suit>3)) return -1;
  if ((rank<0)||(rank>12)) return -1;
  return suit*13+rank;
}

int suit_from_cardid(int cardid) {
  if ((cardid<0)||(cardid>51)) return -1;
  return cardid/13;
}

int rank_from_cardid(int cardid) {
  if ((cardid<0)||(cardid>51)) return -1;
  return cardid%13;
}

uint64_t bit_from_cardid(int cardid) {
  if ((cardid<0)||(cardid>51)) return 0;
  return 1ll<<cardid;
}

int cardid_from_bit(uint64_t single_bit) {
  if (!single_bit) return -1;
  int cardid=0;
  for (;cardid<52;cardid++,single_bit>>=1) {
    if (single_bit&1) return cardid;
  }
  return -1;
}

/* Simple hand analysis.
 */

int hand_count_cards(uint64_t hand) {
  hand&=0x000fffffffffffffll;
  int c=0;
  while (hand) {
    if (hand&1) c++;
    hand>>=1;
  }
  return c;
}

int hand_count_suits(uint64_t hand) {
  hand&=0x000fffffffffffffll;
  int c=0;
  while (hand) {
    if (hand&0x1fff) c++;
    hand>>=13;
  }
  return c;
}

int hand_count_cards_of_suit(uint64_t hand,int suit) {
  if ((suit<0)||(suit>3)) return 0;
  hand>>=suit*13;
  hand&=0x1fff;
  int c=0;
  while (hand) {
    if (hand&1) c++;
    hand>>=1;
  }
  return c;
}

int hand_has_card(uint64_t hand,int cardid) {
  if ((cardid<0)||(cardid>51)) return 0;
  return (hand&(1ll<<cardid))?1:0;
}

int hand_has_suit(uint64_t hand,int suit) {
  if ((suit<0)||(suit>3)) return 0;
  return (hand&(0x1fffll<<(suit*13)))?1:0;
}

uint64_t hands_overlap(uint64_t a,uint64_t b) {
  return a&b;
}

uint64_t hand_add_cardid(uint64_t hand,int cardid) {
  if ((cardid<0)||(cardid>=52)) return hand;
  return hand|(1ll<<cardid);
}

uint64_t hand_remove_cardid(uint64_t hand,int cardid) {
  if ((cardid<0)||(cardid>=52)) return hand;
  return hand&~(1ll<<cardid);
}

/* Card list to or from hand.
 */

int cardlist_from_hand(uint8_t *cardidv,uint64_t hand) {
  int cardidc=0;
  hand&=0x000fffffffffffffll;
  int cardid=0;
  while (hand) {
    if (hand&1) {
      cardidv[cardidc++]=cardid;
    }
    hand>>=1;
    cardid++;
  }
  return cardidc;
}

uint64_t hand_from_cardlist(const uint8_t *cardidv,int cardidc) {
  uint64_t hand=0;
  for (;cardidc-->0;cardidv++) {
    uint8_t cardid=*cardidv;
    if (cardid>51) continue; // naughty naughty caller
    hand|=1ll<<cardid;
  }
  return hand;
}

/* Random selection of a given card count, of different suits.
 */

uint64_t hand_pick_n(uint64_t hand,int suitc) {

  /* Which suits do we have?
   * Confirm that (suitc) makes sense for us.
   */
  if (suitc<1) return 0;
  int havesuitv[4];
  int havesuitc=0;
  if (hand&MASK_HEART  ) havesuitv[havesuitc++]=0;
  if (hand&MASK_DIAMOND) havesuitv[havesuitc++]=1;
  if (hand&MASK_CLUB   ) havesuitv[havesuitc++]=2;
  if (hand&MASK_SPADE  ) havesuitv[havesuitc++]=3;
  if (havesuitc<suitc) return 0;
  
  /* Eliminate randomly from (havesuitv) until we've reduced it to (suitc).
   */
  while (havesuitc>suitc) {
    int rmp=rand()%havesuitc;
    havesuitc--;
    memmove(havesuitv+rmp,havesuitv+rmp+1,sizeof(int)*(havesuitc-rmp));
  }
  
  /* Pick one at random from each of (havesuitv) and combine them.
   */
  uint64_t dst=0;
  while (havesuitc-->0) {
    int suit=havesuitv[havesuitc];
    uint16_t bits=(hand>>(suit*13))&0x1fff;
    int rankv[13];
    int rankc=0;
    int rank=0;
    for (;rank<13;rank++,bits>>=1) {
      if (bits&1) rankv[rankc++]=rank;
    }
    if (!rankc) return 0; // Oops.
    rank=rankv[rand()%rankc];
    dst|=bit_from_cardid(cardid_from_suit_rank(suit,rank));
  }
  return dst;
}

/* Simple dealing.
 */
 
uint64_t hand_deal_n(uint64_t hand,int n) {
  if (n<1) return 0;
  hand&=0x000fffffffffffffll;
  int cardidv[52];
  int cardidc=0;
  int cardid=0;
  uint64_t bit=1;
  for (;bit<hand;bit<<=1,cardid++) {
    if (hand&bit) cardidv[cardidc++]=cardid;
  }
  if (n>=cardidc) return hand;
  uint64_t dst=0;
  while (n-->0) {
    int p=rand()%cardidc;
    dst|=1ll<<cardidv[p];
    cardidc--;
    memmove(cardidv+p,cardidv+p+1,sizeof(int)*(cardidc-p));
  }
  return dst;
}

/* Log a hand for troubleshooting.
 */
 
void hand_log(const char *cmt,uint64_t hand) {
  char msg[256];
  int msgc=0;
  if (cmt) {
    // 3 bytes output per card. So as long as the comment is a bit below 100, there's room.
    int cmtc=0;
    while (cmt[cmtc]&&(cmtc<90)) cmtc++;
    memcpy(msg,cmt,cmtc);
    msgc=cmtc;
    msg[msgc++]=':';
  }
  int suit=0,rank=0;
  while (hand) {
    if (hand&1) {
      msg[msgc++]=' ';
      msg[msgc++]="A23456789TJQK"[rank];
      msg[msgc++]="HDCS?"[suit]; // Extra '?' because there's room in uint64_t for most of a fifth suit.
    }
    hand>>=1;
    if (++rank>=13) { rank=0; suit++; }
  }
  fprintf(stderr,"%.*s\n",msgc,msg);
}
