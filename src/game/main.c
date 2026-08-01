#include "all52.h"

struct g g={0};

void egg_client_quit(int status) {
  modals_quit();
}

int egg_client_init() {

  int fbw=0,fbh=0;
  egg_texture_get_size(&fbw,&fbh,1);
  if ((fbw!=FBW)||(fbh!=FBH)) {
    fprintf(stderr,"Framebuffer size mismatch! metadata=%dx%d header=%dx%d\n",fbw,fbh,FBW,FBH);
    return -1;
  }

  g.romc=egg_rom_get(0,0);
  if (!(g.rom=malloc(g.romc))) return -1;
  egg_rom_get(g.rom,g.romc);
  text_set_rom(g.rom,g.romc);

  srand_auto();
  
  if (modals_init()<0) return -1;
  //TODO should eventually be a "hello" modal
  struct modal_args_world args={0};
  struct modal *modal=modal_spawn(&modal_type_world,&args,sizeof(args));
  if (!modal) return -1;

  return 0;
}

void egg_client_notify(int k,int v) {
}

void egg_client_update(double elapsed) {
  int input=egg_input_get_one(0);
  modals_update(elapsed,input);
}

void egg_client_render() {
  graf_reset(&g.graf);
  modals_render();
  graf_flush(&g.graf);
}
