/* modal_gameover.c
 * Splash triggered after you lose your last card.
 * Technically the game can continue at that point, but if there are no loose cards reachable, you can't win.
 */
 
#include "all52.h"

struct modal_gameover {
  struct modal hdr;
};

#define MODAL ((struct modal_gameover*)modal)

static int _gameover_init(struct modal *modal,const void *args,int argslen) {
  modal->opaque=1;
  modal->interactive=1;
  
  all52_song(0,0);
  
  return 0;
}

static void _gameover_update(struct modal *modal,double elapsed,int input) {
  if (input&EGG_BTN_SOUTH) {
    struct modal *world=modal_spawn(&modal_type_hello,0,0);
    if (world) {
      modal->defunct=1;
    }
  }
}

static void _gameover_render(struct modal *modal) {
  graf_set_input(&g.graf,g.texid_modals);
  graf_decal(&g.graf,0,0,0,FBH,FBW,FBH);
}

const struct modal_type modal_type_gameover={
  .name="gameover",
  .objlen=sizeof(struct modal_gameover),
  .init=_gameover_init,
  .update=_gameover_update,
  .render=_gameover_render,
};
