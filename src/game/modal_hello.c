/* modal_hello.c
 * A simple splash, first thing the user sees.
 * Responsible for initializing the game on dismissal.
 */
 
#include "all52.h"

struct modal_hello {
  struct modal hdr;
};

#define MODAL ((struct modal_hello*)modal)

static int _hello_init(struct modal *modal,const void *args,int argslen) {
  modal->opaque=1;
  modal->interactive=1;
  
  all52_song(RID_song_cross_currents,1);
  
  return 0;
}

static void _hello_update(struct modal *modal,double elapsed,int input) {
  if (input&EGG_BTN_SOUTH) {
    struct modal_args_world args={0};
    struct modal *world=modal_spawn(&modal_type_world,&args,sizeof(args));
    if (world) {
      SFX(uiactivate)
      modal->defunct=1;
    }
  }
}

static void _hello_render(struct modal *modal) {
  graf_set_input(&g.graf,g.texid_modals);
  graf_decal(&g.graf,0,0,0,0,FBW,FBH);
}

const struct modal_type modal_type_hello={
  .name="hello",
  .objlen=sizeof(struct modal_hello),
  .init=_hello_init,
  .update=_hello_update,
  .render=_hello_render,
};
