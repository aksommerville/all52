/* modal_dialogue.c
 * Black box full of text that overlays the world view.
 */
 
#include "all52.h"

struct modal_dialogue {
  struct modal hdr;
  int texid,texw,texh;
  int boxx,boxy,boxw,boxh;
};

#define MODAL ((struct modal_dialogue*)modal)

static void _dialogue_del(struct modal *modal) {
  egg_texture_del(MODAL->texid);
}

static int _dialogue_init(struct modal *modal,const void *args,int argslen) {
  if (!args||(argslen!=sizeof(struct modal_args_dialogue))) return -1;
  const struct modal_args_dialogue *ARGS=args;
  
  char tmp[256];
  int tmpc=text_format_res(tmp,sizeof(tmp),ARGS->rid,ARGS->strix,ARGS->insv,ARGS->insc);
  if ((tmpc<0)||(tmpc>sizeof(tmp))) return -1;
  MODAL->texid=font_render_multiline(tmp,tmpc,FBW-4,0xffffffff,1);
  egg_texture_get_size(&MODAL->texw,&MODAL->texh,MODAL->texid);
  
  MODAL->boxw=MODAL->texw; // Font gives us a margin, so the texture's size is exactly its box's size.
  MODAL->boxh=MODAL->texh;
  MODAL->boxx=(FBW>>1)-(MODAL->boxw>>1);
  MODAL->boxy=(FBH>>1)-(MODAL->boxh>>1);
  
  modal->opaque=0;
  modal->interactive=1;
  
  return 0;
}

static void _dialogue_update(struct modal *modal,double elapsed,int input) {
  if (input&EGG_BTN_SOUTH) {
    modal->defunct=1;
  }
}

static void _dialogue_render(struct modal *modal) {
  graf_fill_rect(&g.graf,MODAL->boxx,MODAL->boxy,MODAL->boxw,MODAL->boxh,0x000000ff);
  graf_set_input(&g.graf,MODAL->texid);
  graf_decal(&g.graf,MODAL->boxx,MODAL->boxy,0,0,MODAL->texw,MODAL->texh);
}

const struct modal_type modal_type_dialogue={
  .name="dialogue",
  .objlen=sizeof(struct modal_dialogue),
  .del=_dialogue_del,
  .init=_dialogue_init,
  .update=_dialogue_update,
  .render=_dialogue_render,
};
