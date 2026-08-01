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

#define FBW 52
#define FBH 52

// Map size in cells. There are also hard-coded assumptions around this, so don't change lightly.
#define MAPW 64
#define MAPH 64

extern struct g {
  void *rom;
  int romc;
  struct graf graf;
  
  int texid_world;
  int texid_sprites;
  int texid_font;
  const uint8_t *physics; // LRTB big-endian, 1 bit per cell. 512 bytes.
} g;

#define CKPH(x,y) (g.physics[(y)*8+((x)>>3)]&(0x80>>((x)&7)))

#endif
