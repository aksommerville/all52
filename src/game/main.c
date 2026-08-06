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
  REQIMG(modals,128,128)
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
  struct modal *modal=modal_spawn(&modal_type_hello,0,0);
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
  
  /* We naively tick the playclock at all times.
   * The world and victory modals reset and sample it at the right moments,
   * so it shouldn't be a problem that it ticks at times when it actually doesn't count.
   * We do want to count time spent in the battle and dialogue modals -- everything from start to finish of a session.
   */
  g.playclock+=elapsed;
  
  modals_update(elapsed,input);
  
  /* If the gameover or victory modal needs spawned, do it here.
   * It would be more convenient in modal_world update, but that won't run until the next frame, there would be flicker.
   */
  if (g.finish) {
    struct modal *next;
    if (g.finish>0) next=modal_spawn(&modal_type_victory,0,0);
    else next=modal_spawn(&modal_type_gameover,0,0);
    if (next) {
      struct modal *world=modal_topmost_of_type(&modal_type_world);
      if (world) world->defunct=1;
    }
    g.finish=0;
  }
}

void egg_client_render() {
  graf_reset(&g.graf);
  modals_render();
  graf_flush(&g.graf);
}

/* Sound effects.
 */

void all52_sound(int rid,double trim,double pan) {
  double now=egg_time_real();
  double old=now-SOUND_BLACKOUT_TIME;
  
  /* Just for hygiene's sake, drop all expired blackout records from the tail.
   */
  while (g.sound_blackoutc>0) {
    struct sound_blackout *sb=g.sound_blackoutv+g.sound_blackoutc-1;
    if (sb->when>=old) break;
    g.sound_blackoutc--;
  }
  
  /* Check all blackout slots.
   * If a slot is available for reuse, note it.
   * If this sound is already playing, abort.
   * Note that for no-replay purposes, we don't care about trim or pan.
   */
  struct sound_blackout *blackout=0;
  struct sound_blackout *q=g.sound_blackoutv;
  int i=g.sound_blackoutc;
  for (;i-->0;q++) {
    if (q->when<old) {
      if (!blackout) blackout=q;
    } else if (q->rid==rid) {
      return; // Already playing recently, forget it.
    }
  }
  
  /* If we didn't find an evictable slot, append one.
   * But if the list is full, abort: This is also a global too-many-sounds protection.
   */
  if (!blackout) {
    if (g.sound_blackoutc>=SOUND_BLACKOUT_LIMIT) return;
    blackout=g.sound_blackoutv+g.sound_blackoutc++;
  }
  
  /* Fill in the blackout record, and start playing it.
   */
  blackout->rid=rid;
  blackout->when=now;
  egg_play_sound(rid,trim,pan);
}

/* Play song.
 */
 
void all52_song(int rid,int repeat) {
  if (rid==g.song_playing) return;
  g.song_playing=rid;
  egg_play_song(1,rid,repeat,0.500,0.0);
}
