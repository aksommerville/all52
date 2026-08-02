#include "all52.h"

#define INTRO_TIME 1.000
#define BGCOLOR 0x0a5617ff
#define TEXTCOLOR 0xe8ddbbff

#define STAGE_INTRO    1 /* Lower layers remain visible, transition to bg color. */
#define STAGE_PICK     2 /* Player picks her cards. */
#define STAGE_REVEAL   3 /* Each card turns over, non-interactive. */
#define STAGE_ALIGN    4 /* Cards slide to align with the appropriate opponent card. */
#define STAGE_DISBURSE 5 /* Cards slide up or down per suit, to the winner. */

struct modal_battle {
  struct modal hdr;
  int hltx,hlty;
  uint8_t *cpu_hand;
  uint8_t *man_hand;
  int cpuc,manc;
  int cpusuitc,mansuitc;
  int suitc; // Lower of the two.
  uint8_t cpuplay[4];
  uint8_t manplay[4];
  int cpuplayc,manplayc;
  int stage;
  double stageclock;
  int prompt,promptw,prompth;
};

#define MODAL ((struct modal_battle*)modal)

/* Cleanup.
 */
 
static void _battle_del(struct modal *modal) {
  egg_texture_del(MODAL->prompt);
}

/* Init.
 */
 
static int _battle_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=0; // We'll become opaque after our intro.
  MODAL->stage=STAGE_INTRO;
  MODAL->stageclock=INTRO_TIME;

  if (!args||(argslen!=sizeof(struct modal_args_battle))) return -1;
  const struct modal_args_battle *ARGS=args;
  MODAL->hltx=ARGS->hltx;
  MODAL->hlty=ARGS->hlty;
  if (!(MODAL->cpu_hand=ARGS->cpu_hand)) return -1;
  if (!(MODAL->man_hand=ARGS->man_hand)) return -1;
  while (MODAL->cpu_hand[MODAL->cpuc]) MODAL->cpuc++;
  while (MODAL->man_hand[MODAL->manc]) MODAL->manc++;
  if ((MODAL->cpuc<1)||(MODAL->manc<1)) return -1;
  fprintf(stderr,"%s: cpuc=%d manc=%d\n",__func__,MODAL->cpuc,MODAL->manc);
  
  /* Count the suits in each hand.
   */
  uint8_t pv=CARD_SUIT(MODAL->cpu_hand[0]);
  int i;
  MODAL->cpusuitc=1;
  for (i=0;i<MODAL->cpuc;i++) {
    uint8_t suit=CARD_SUIT(MODAL->cpu_hand[i]);
    if (suit!=pv) {
      MODAL->cpusuitc++;
      pv=suit;
    }
  }
  MODAL->mansuitc=1;
  pv=CARD_SUIT(MODAL->man_hand[0]);
  for (i=0;i<MODAL->manc;i++) {
    uint8_t suit=CARD_SUIT(MODAL->man_hand[i]);
    if (suit!=pv) {
      MODAL->mansuitc++;
      pv=suit;
    }
  }
  MODAL->suitc=(MODAL->cpusuitc<MODAL->mansuitc)?MODAL->cpusuitc:MODAL->mansuitc;
  fprintf(stderr,"cpusuitc=%d mansuitc=%d\n",MODAL->cpusuitc,MODAL->mansuitc);
  
  /* Generate the prompt.
   */
  const char *msg=0;
  switch (MODAL->suitc) {
    case 1: msg="Pick one"; break;
    case 2: msg="Pick two"; break;
    case 3: msg="Pick three"; break;
    case 4: msg="Pick four"; break;
    default: return -1;
  }
  MODAL->prompt=font_render_multiline(msg,-1,FBW,TEXTCOLOR,1);
  egg_texture_get_size(&MODAL->promptw,&MODAL->prompth,MODAL->prompt);
  
  //TODO Pick CPU cards.
    
  return 0;
}

/* Focus.
 */
 
static void _battle_focus(struct modal *modal,int focus) {
}

/* Pick a CPU card at random.
 */
 
static void battle_cpu_pick(struct modal *modal) {
  //TODO Pull one from (cpu_hand) and put it in (cpuplay). Ensure it's a suit not already represented in (cpuplay).
}

/* Update, in PICK stage. The only interactive stage.
 */
 
static void battle_update_PICK(struct modal *modal,double elapsed,int input) {
  // Track input for the player's pick.
  //TODO
}

/* Update.
 */
 
static void _battle_update(struct modal *modal,double elapsed,int input) {
  switch (MODAL->stage) {
  
    case STAGE_INTRO: {
        if ((MODAL->stageclock-=elapsed)<=0.0) {
          MODAL->stage=STAGE_PICK;
          MODAL->stageclock=0.0;
          modal->opaque=1;
        }
      } break;
      
    case STAGE_PICK: battle_update_PICK(modal,elapsed,input); break;
    case STAGE_REVEAL:
    case STAGE_ALIGN:
    case STAGE_DISBURSE:
    default: modal->defunct=1;
  }
  
  //TODO
  if (input&EGG_BTN_WEST) modal->defunct=1;
}

/* Render.
 */
 
static void _battle_render(struct modal *modal) {
  switch (MODAL->stage) {
  
    case STAGE_INTRO: {
        double t=1.0-MODAL->stageclock/INTRO_TIME;
        int lmax=MODAL->hltx;
        int rmax=FBW-lmax;
        int tmax=MODAL->hlty;
        int bmax=FBH-tmax;
        if ((lmax<0)||(rmax<0)||(tmax<0)||(bmax<0)) {
          // We got OOB coordinates for the focus point. Do a plain fade instead.
          int alpha=(int)(t*255.0);
          if (alpha>0) {
            if (alpha>0xff) alpha=0xff;
            graf_fill_rect(&g.graf,0,0,FBW,FBH,(BGCOLOR&0xffffff00)|alpha);
          }
        } else {
          // Close a rectangle in on the focus point.
          int lw=lround(t*lmax);
          int rw=lround(t*rmax);
          int th=lround(t*tmax);
          int bh=lround(t*bmax);
          if (th>0) { // The top part gets a shadow under.
            graf_fill_rect(&g.graf,0,0,FBW,th+1,0x00000080);
            graf_fill_rect(&g.graf,0,0,FBW,th,BGCOLOR);
          }
          if (bh>0) graf_fill_rect(&g.graf,0,FBH-bh,FBW,bh,BGCOLOR);
          if (lw>0) graf_fill_rect(&g.graf,0,0,lw,FBH,BGCOLOR);
          if (rw>0) graf_fill_rect(&g.graf,FBW-rw,0,rw,FBH,BGCOLOR);
        }
      } break;
      
    case STAGE_PICK: {
        graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
        graf_set_input(&g.graf,MODAL->prompt);
        graf_decal(&g.graf,0,0,0,0,MODAL->promptw,MODAL->prompth);
        //TODO
        graf_set_input(&g.graf,g.texid_cards);
        graf_decal(&g.graf,1,20,0,0,CARDW,CARDH);
        graf_decal(&g.graf,13,20,CARDW,0,CARDW,CARDH);
      } break;
      
    case STAGE_REVEAL: break;//TODO
    case STAGE_ALIGN: break;//TODO
    case STAGE_DISBURSE: break;//TODO
  }
}

/* Type definition.
 */
 
const struct modal_type modal_type_battle={
  .name="battle",
  .objlen=sizeof(struct modal_battle),
  .del=_battle_del,
  .init=_battle_init,
  .focus=_battle_focus,
  .update=_battle_update,
  .render=_battle_render,
};
