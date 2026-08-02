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

/* Using the composable bits in image:sprites, make a texture of 53 card images, 11x18 each.
 */
 
static int generate_card_images() {
  int imgw=CARDW*52;
  int imgh=CARDH;
  if ((g.texid_cards=egg_texture_new())<1) return -1;
  if (egg_texture_load_raw(g.texid_cards,imgw,imgh,imgw<<2,0,0)<0) return -1;
  egg_texture_clear(g.texid_cards);
  graf_reset(&g.graf);
  graf_set_output(&g.graf,g.texid_cards);
  graf_set_input(&g.graf,g.texid_sprites);
  int x=0,cardid=0;
  for (;cardid<52;cardid++,x+=CARDW) {
    graf_decal(&g.graf,x,0,11,96,CARDW,CARDH); // Background.
    int rank=rank_from_cardid(cardid);
    int suit=suit_from_cardid(cardid);
    int rankx=22+rank*5;
    int ranky=96;
    if (suit>=2) ranky+=5;
    int suitx=22+suit*5;
    int suity=106;
    graf_decal(&g.graf,x+3,3,rankx,ranky,5,5);
    graf_decal(&g.graf,x+3,10,suitx,suity,5,5);
  }
  graf_flush(&g.graf);
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
  if (generate_card_images()<0) return -1;

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
  if (g.input_blackout) {
    if (input&g.input_blackout) { // Await release, and call them off.
      input&=~g.input_blackout;
    } else { // OK, end of blackout.
      g.input_blackout=0;
    }
  }
  modals_update(elapsed,input);
}

void egg_client_render() {
  graf_reset(&g.graf);
  modals_render();
  graf_flush(&g.graf);
}
