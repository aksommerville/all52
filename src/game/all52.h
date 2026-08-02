#ifndef EGG_GAME_MAIN_H
#define EGG_GAME_MAIN_H

#include "egg/egg.h"
#include "util/stdlib/egg-stdlib.h"
#include "util/graf/graf.h"
#include "util/res/res.h"
#include "util/text/text.h"
#include "egg_res_toc.h"
#include "shared_symbols.h"

struct modal;
struct modal_type;
struct sprite;
struct sprite_type;

#include "modal.h"
#include "world.h"
#include "sprite.h"
#include "font.h"

#define FBW 52
#define FBH 52

// Map size in cells. There are also hard-coded assumptions around this, so don't change lightly.
#define MAPW 64
#define MAPH 64

// For the full-size card images that show both suit and rank.
#define CARDW 11
#define CARDH 18

/* Card values are zero for none, or 1..52 running suit-major with aces low.
 * A hand is a sorted and zero-terminated array of uint8_t cards, so 53 bytes max.
 * When suit and rank are supplied separately, they're both one-based.
 * Suits are 1..4 = heart,diamond,club,spade. Tho game logic never needs to care which is which.
 */
static inline uint8_t CARD_COMPOSE(uint8_t suit,uint8_t rank) {
  if ((suit<1)||(suit>4)||(rank<1)||(rank>13)) return 0;
  return (suit-1)*13+rank;
}
static inline uint8_t CARD_SUIT(uint8_t card) {
  if ((card<1)||(card>52)) return 0;
  return 1+(card-1)/13;
}
static inline uint8_t CARD_RANK(uint8_t card) {
  if ((card<1)||(card>52)) return 0;
  return (card-1)%13+1;
}

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  
  int texid_world;
  int texid_sprites;
  int texid_font;
  int texid_cards; // Generated at init from sprites.
  const uint8_t *physics; // LRTB big-endian, 1 bit per cell. 512 bytes.
} g;

#define CKPH(x,y) (g.physics[(y)*8+((x)>>3)]&(0x80>>((x)&7)))

#endif
