#include "all52.h"

struct g g={0};

/* Quit: Don't actually need to do anything here but it pleases me to clean up after myself*.
 * [*] When coding. Certainly not in real life.
 */

void egg_client_quit(int status) {
  modals_quit();
  world_quit();
}

/* Scan resources.
 */
 
static int scan_resources() {
  
  #define REQIMG(tag,w,h) { \
    if ((g.texid_##tag=egg_texture_new())<1) return -1; \
    if (egg_texture_load_image(g.texid_##tag,RID_image_##tag)<0) return -1; \
    int actualw=0,actualh=0; \
    egg_texture_get_size(&actualw,&actualh,g.texid_##tag); \
    if ((actualw!=(w))||(actualh!=(h))) { \
      fprintf(stderr,"image:"#tag": Expected %dx%d but found %dx%d\n",w,h,actualw,actualh); \
      return -1; \
    } \
  }
  REQIMG(world,512,512)
  REQIMG(sprites,128,128)
  REQIMG(font,48,30)
  #undef REQIMG
  
  struct rom_reader reader;
  if (rom_reader_init(&reader,g.rom,g.romc)<0) return -1;
  struct rom_entry res;
  while (rom_reader_next(&res,&reader)>0) {
    switch (res.tid) {
      case EGG_TID_physics: if (res.rid==1) {
          if (res.c!=512) {
            fprintf(stderr,"physics:1 must be exactly 512 bytes\n");
            return -1;
          }
          g.physics=res.v;
        } break;
    }
  }
  
  if (!g.physics) {
    fprintf(stderr,"physics:1 not found\n");
    return -1;
  }
  
  return 0;
}

/* Init.
 */

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
  if (scan_resources()<0) return -1;

  srand_auto();
  
  if (modals_init()<0) return -1;
  //TODO should eventually be a "hello" modal
  struct modal_args_world args={0};
  struct modal *modal=modal_spawn(&modal_type_world,&args,sizeof(args));
  if (!modal) return -1;

  return 0;
}

/* Notify, update render: Defer to modal stack.
 */

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
