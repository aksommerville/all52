/* modal_battle.c
 * We manage the whole card game.
 */

#include "all52.h"

#define INTRO_TIME 1.000
#define BGCOLOR 0x0a5617ff
#define TEXTCOLOR 0xe8ddbbff
#define CHEAT_LIMIT 20 /* Longer than the longest cheat code. */

#define STAGE_INTRO    1 /* Lower layers remain visible, transition to bg color. */
#define STAGE_PICK     2 /* Player picks her cards. */
#define STAGE_REVEAL   3 /* Each card turns over, non-interactive. */
#define STAGE_ALIGN    4 /* Cards slide to align with the appropriate opponent card. */
#define STAGE_DISBURSE 5 /* Cards slide up or down per suit, to the winner. */
#define STAGE_CHEAT_WIN_ALL 6 /* BBB. Win all the monster's cards without a fight. */
#define STAGE_CHEAT_ABORT   7 /* DDD. Cancel the fight. */

struct modal_battle {
  struct modal hdr;
  int hltx,hlty;
  int pvinput;
  void (*cb)(struct modal *modal);
  void *userdata;
  
  /* Which cards does everybody have, overall?
   * Played cards are also in a hand at all times.
   */
  uint64_t cpu_hand;
  uint64_t man_hand;
  
  /* Which cards are being played?
   */
  uint64_t cpu_play;
  uint64_t man_play;
  
  /* Minimum of the two players' suit counts, ie how many cards get played in this battle.
   */
  int suitc;
  
  int stage;
  double stageclock;
  int prompt,promptw,prompth; // "Pick N"
  int okprompt,okpromptw,okprompth; // "OK?"
  int chprompt,chpromptw,chprompth;
  
  int pickp; // 0..3 ie suit. Which suit is highlighted.
  int pickv[4]; // Value is rank or -1, index is suit.
  int rankv[14]; // List of ranks available for the highlighted suit, plus always a -1 for none.
  int rankc;
  int rankp; // Where is Dot's finger, in (rankv).
  int picking_rank;
  int confirmp; // -1,0,1 = irrelevant,yes,no. >=0 when asking for confirmation
  
  /* Same as (cpu_play,man_play) but cardid.
   * Length is (suitc).
   */
  int cpu_cardidv[4];
  int man_cardidv[4];
  
  /* Parallel to (*_cardidv). Values are -1 if the cpu wins it, or 1 if the man wins it.
   */
  int cpu_disbv[4];
  int man_disbv[4];
  
  /* Same as (*_disbv) but indexed by suit.
   */
  int cpu_suit_disbv[4];
  int man_suit_disbv[4];
  
  /* Record keystrokes during INTRO, for cheat codes.
   */
  uint16_t cheatv[CHEAT_LIMIT];
  int cheatc;
};

#define MODAL ((struct modal_battle*)modal)

/* Cleanup.
 */
 
static void _battle_del(struct modal *modal) {
  egg_texture_del(MODAL->prompt);
  egg_texture_del(MODAL->okprompt);
  egg_texture_del(MODAL->chprompt);
}

/* Init.
 */
 
static int _battle_init(struct modal *modal,const void *args,int argslen) {

  modal->interactive=1;
  modal->opaque=0; // We'll become opaque after our intro.
  MODAL->stage=STAGE_INTRO;
  MODAL->stageclock=INTRO_TIME;
  MODAL->pickp=-1;
  MODAL->pickv[0]=MODAL->pickv[1]=MODAL->pickv[2]=MODAL->pickv[3]=-1;
  MODAL->confirmp=-1;

  if (!args||(argslen!=sizeof(struct modal_args_battle))) return -1;
  const struct modal_args_battle *ARGS=args;
  MODAL->hltx=ARGS->hltx;
  MODAL->hlty=ARGS->hlty;
  MODAL->cpu_hand=ARGS->cpu_hand;
  MODAL->man_hand=ARGS->man_hand;
  MODAL->cb=ARGS->cb;
  MODAL->userdata=ARGS->userdata;
  
  /* Both players must have at least one card, and they must not overlap.
   */
  if (!MODAL->cpu_hand||!MODAL->man_hand||hands_overlap(MODAL->cpu_hand,MODAL->man_hand)) {
    fprintf(stderr,"%s: Invalid input hands.\n",__func__);
    hand_log("cpu",MODAL->cpu_hand);
    hand_log("man",MODAL->man_hand);
    return -1;
  }
  
  /* Count the suits in each hand.
   */
  int cpusuitc=hand_count_suits(MODAL->cpu_hand);
  int mansuitc=hand_count_suits(MODAL->man_hand);
  MODAL->suitc=(cpusuitc<mansuitc)?cpusuitc:mansuitc;
  if ((MODAL->suitc<1)||(MODAL->suitc>4)) {
    fprintf(stderr,"%s: Invalid input hands at suit count.\n",__func__);
    hand_log("cpu",MODAL->cpu_hand);
    hand_log("man",MODAL->man_hand);
    return -1;
  }
  
  /* Pick the CPU's play, might as well do it immediately.
   */
  if (!(MODAL->cpu_play=hand_pick_n(MODAL->cpu_hand,MODAL->suitc))) {
    fprintf(stderr,"%s:%d: hand_pick_n() failed\n",__FILE__,__LINE__);
    return -1;
  }
  
  /* Generate the prompts.
   */
  const char *msg=0;
  switch (MODAL->suitc) {
    case 1: msg="Pick one"; break;
    case 2: msg="Pick two"; break;
    case 3: msg="Pick three"; break;
    case 4: msg="Pick four"; break;
    default: return -1;
  }
  MODAL->prompt=font_render_multiline(msg,-1,FBW,TEXTCOLOR,-1);
  egg_texture_get_size(&MODAL->promptw,&MODAL->prompth,MODAL->prompt);
  MODAL->okprompt=font_render_multiline("OK?",3,FBW,TEXTCOLOR,-1);
  egg_texture_get_size(&MODAL->okpromptw,&MODAL->okprompth,MODAL->okprompt);
  
  return 0;
}

/* Focus.
 */
 
static void _battle_focus(struct modal *modal,int focus) {
}

/* Sanitize (pickp) and rebuild (rankv,rankc,rankp) to agree with it.
 */
 
static void battle_pick_changed(struct modal *modal) {
  if ((MODAL->pickp<0)||(MODAL->pickp>=4)) MODAL->pickp=0;
  MODAL->rankp=0;
  MODAL->rankc=hand_count_cards_of_suit(MODAL->man_hand,MODAL->pickp);
  // If there are no cards of this rank, leave it empty. Otherwise, add "none" at the start and list them all.
  if (MODAL->rankc) {
    MODAL->rankc++;
    MODAL->rankv[0]=-1;
    int p=1,rank=0,cardid=cardid_from_suit_rank(MODAL->pickp,rank);
    while ((p<MODAL->rankc)&&(rank<13)) {
      if (hand_has_card(MODAL->man_hand,cardid)) {
        if (MODAL->pickv[MODAL->pickp]==rank) MODAL->rankp=p;
        MODAL->rankv[p++]=rank;
      }
      rank++;
      cardid++;
    }
  }
}

/* Check a cheat code.
 * Input must terminate with a zero.
 */
 
static int battle_check_cheat(struct modal *modal,...) {
  va_list vargs;
  va_start(vargs,modal);
  int cheatp=0;
  for (;;) {
    int btnid=va_arg(vargs,int);
    if (!btnid) {
      if (cheatp==MODAL->cheatc) return 1; // Match!
      return 0; // More strokes entered. No match.
    }
    if (cheatp>=MODAL->cheatc) return 0; // Not enough strokes. No match.
    if (btnid!=MODAL->cheatv[cheatp]) return 0; // Mismatch.
    cheatp++;
  }
}

/* Describe the cheat.
 */
 
static void battle_cheat_prompt(struct modal *modal,const char *src) {
  if (MODAL->chprompt) egg_texture_del(MODAL->chprompt);
  MODAL->chprompt=font_render_multiline(src,-1,FBW,0xffffffff,1);
  egg_texture_get_size(&MODAL->chpromptw,&MODAL->chprompth,MODAL->chprompt);
}

/* Update, INTRO.
 */
 
static void battle_update_INTRO(struct modal *modal,double elapsed,int input) {
  if ((MODAL->stageclock-=elapsed)<=0.0) {
    if (battle_check_cheat(modal,EGG_BTN_WEST,EGG_BTN_WEST,EGG_BTN_WEST,0)) {
      MODAL->stage=STAGE_CHEAT_WIN_ALL;
      battle_cheat_prompt(modal,"Cheat: you win!");
    } else {
      //if (MODAL->cheatc) fprintf(stderr,"battle ignoring unknown %d-stroke cheat code.\n",MODAL->cheatc);
      MODAL->stage=STAGE_PICK;
      MODAL->stageclock=0.0;
      modal->opaque=1;
      MODAL->picking_rank=0;
      MODAL->confirmp=-1;
      battle_pick_changed(modal);
    }
  }
  if (input!=MODAL->pvinput) {
    // At first we had the whole gamepad, but now we're only blacking out SOUTH and WEST, so those are the only two we should use here.
    if ((input&EGG_BTN_SOUTH)&&!(MODAL->pvinput&EGG_BTN_SOUTH)) MODAL->cheatv[MODAL->cheatc++]=EGG_BTN_SOUTH;
    if ((input&EGG_BTN_WEST)&&!(MODAL->pvinput&EGG_BTN_WEST)) MODAL->cheatv[MODAL->cheatc++]=EGG_BTN_WEST;
  }
}

/* Call when player's selection is final and confirmed.
 */
 
static void battle_player_ready(struct modal *modal) {
  
  /* Just to be on the safe side, validate once more that the correct suit count is selected,
   * and that all selected cards are in fact in the player's hand.
   * It should not be possible for this to fail.
   */
  int validc=0;
  if ((MODAL->pickv[0]>=0)&&hand_has_card(MODAL->man_hand,cardid_from_suit_rank(0,MODAL->pickv[0]))) validc++;
  if ((MODAL->pickv[1]>=0)&&hand_has_card(MODAL->man_hand,cardid_from_suit_rank(1,MODAL->pickv[1]))) validc++;
  if ((MODAL->pickv[2]>=0)&&hand_has_card(MODAL->man_hand,cardid_from_suit_rank(2,MODAL->pickv[2]))) validc++;
  if ((MODAL->pickv[3]>=0)&&hand_has_card(MODAL->man_hand,cardid_from_suit_rank(3,MODAL->pickv[3]))) validc++;
  if (validc!=MODAL->suitc) {
    fprintf(stderr,"%s:%d:PANIC: Selections don't look kosher anymore!\n",__FILE__,__LINE__);
    MODAL->confirmp=-1;
    return;
  }
  
  /* Populate (cpu_cardidv,man_cardidv) now that the choices are final.
   */
  memset(MODAL->cpu_cardidv,-1,sizeof(MODAL->cpu_cardidv));
  memset(MODAL->man_cardidv,-1,sizeof(MODAL->man_cardidv));
  int cpu_cardidc=0,man_cardidc=0;
  uint64_t bits=MODAL->cpu_play;
  int cardid=0;
  while (bits) {
    if (bits&1) {
      MODAL->cpu_cardidv[cpu_cardidc++]=cardid;
      if (cpu_cardidc>=MODAL->suitc) break;
    }
    bits>>=1;
    cardid++;
  }
  int i=0;
  for (;i<4;i++) {
    if (MODAL->pickv[i]<0) continue;
    MODAL->man_cardidv[man_cardidc++]=cardid_from_suit_rank(i,MODAL->pickv[i]);
    if (man_cardidc>=MODAL->suitc) break;
  }
  
  SFX(battle_commit)
  MODAL->stage=STAGE_REVEAL;
  MODAL->stageclock=0.0;
}

/* Call after the pick set changes.
 * If the right count of suits is chosen, prompt for confirmation.
 */
 
static void battle_maybe_confirm(struct modal *modal) {
  int validc=0;
  if (MODAL->pickv[0]>=0) validc++;
  if (MODAL->pickv[1]>=0) validc++;
  if (MODAL->pickv[2]>=0) validc++;
  if (MODAL->pickv[3]>=0) validc++;
  if (validc==MODAL->suitc) {
    MODAL->confirmp=0;
  }
}

/* Horizontal adjustment of cursor.
 * May be suit, rank, or confirmation.
 */
 
static void battle_adjust(struct modal *modal,int d) {
  SFX(uimotion)
  if (MODAL->confirmp>=0) {
    MODAL->confirmp+=d;
    if (MODAL->confirmp<0) MODAL->confirmp=0;
    else if (MODAL->confirmp>1) MODAL->confirmp=1;
  } else if (MODAL->picking_rank) {
    // Ranks can be empty. And they wrap around, because it's potentially a long scrolling list.
    if (MODAL->rankc<1) return;
    MODAL->rankp+=d;
    if (MODAL->rankp<0) MODAL->rankp=MODAL->rankc-1;
    else if (MODAL->rankp>=MODAL->rankc) MODAL->rankp=0;
  } else {
    // Suits are fixed, there's four of them. Do still wrap, because it's weird not to, when ranks do.
    MODAL->pickp+=d;
    if (MODAL->pickp<0) MODAL->pickp=3;
    else if (MODAL->pickp>3) MODAL->pickp=0;
    battle_pick_changed(modal);
  }
}

/* Vertical adjustment of cursor.
 * No wrapping.
 */
 
static void battle_pick_suit(struct modal *modal) {
  SFX(uiback)
  MODAL->picking_rank=0;
}

static void battle_pick_rank(struct modal *modal) {
  if (MODAL->rankc<1) return;
  SFX(uiactivate)
  MODAL->picking_rank=1;
  // Prefer to select the lowest rank, but not "none":
  if ((MODAL->rankc>=2)&&(MODAL->rankv[0]<0)&&(MODAL->rankp==0)) MODAL->rankp=1;
}

/* WEST to cancel.
 * Whichever suit is focussed, clear it.
 */
 
static void battle_cancel(struct modal *modal) {
  if (MODAL->confirmp>=0) {
    SFX(uiback)
    MODAL->confirmp=-1;
    return;
  }
  SFX(uiactivate)
  MODAL->pickv[MODAL->pickp]=-1;
  MODAL->picking_rank=0;
  battle_maybe_confirm(modal);
}

/* SOUTH to activate.
 * This is equivalent to DOWN when suits are focussed.
 */
 
static void battle_activate(struct modal *modal) {
  if (MODAL->confirmp>=0) {
    if (MODAL->confirmp==1) {
      battle_cancel(modal);
    } else {
      battle_player_ready(modal);
    }
    return;
  }
  if (!MODAL->picking_rank) {
    battle_pick_rank(modal);
    return;
  }
  SFX(uiactivate)
  MODAL->pickv[MODAL->pickp]=MODAL->rankv[MODAL->rankp];
  MODAL->picking_rank=0; // Return focus to the suit row.
  battle_maybe_confirm(modal);
}

/* Update, in PICK stage. The only interactive stage.
 */
 
static void battle_update_PICK(struct modal *modal,double elapsed,int input) {
  // LEFT,RIGHT to adjust. Either the suit or rank, whichever is focussed.
  if ((input&EGG_BTN_LEFT)&&!(MODAL->pvinput&EGG_BTN_LEFT)) battle_adjust(modal,-1);
  else if ((input&EGG_BTN_RIGHT)&&!(MODAL->pvinput&EGG_BTN_RIGHT)) battle_adjust(modal,1);
  // UP,DOWN to toggle between suit and rank.
  if (MODAL->confirmp<0) {
    if ((input&EGG_BTN_UP)&&!(MODAL->pvinput&EGG_BTN_UP)&&MODAL->picking_rank) battle_pick_suit(modal);
    else if ((input&EGG_BTN_DOWN)&&!(MODAL->pvinput&EGG_BTN_DOWN)&&!MODAL->picking_rank) battle_pick_rank(modal);
  }
  // SOUTH to pick a card.
  if ((input&EGG_BTN_SOUTH)&&!(MODAL->pvinput&EGG_BTN_SOUTH)) battle_activate(modal);
  // WEST to cancel.
  if ((input&EGG_BTN_WEST)&&!(MODAL->pvinput&EGG_BTN_WEST)) battle_cancel(modal);
}

/* Update, REVEAL.
 */
 
static void battle_update_REVEAL(struct modal *modal,double elapsed,int input) {
  MODAL->stageclock+=elapsed;
  if (MODAL->stageclock>=2.0) {
    MODAL->stage=STAGE_ALIGN;
    MODAL->stageclock=0.0;
  }
}

/* Find card in an int list, by suit.
 */
 
static int battle_card_of_suit(const int *v,int c,int suit) {
  for (;c-->0;v++) {
    if (suit_from_cardid(*v)==suit) return *v;
  }
  return -1;
}

/* Update, ALIGN.
 */
 
static void battle_update_ALIGN(struct modal *modal,double elapsed,int input) {
  MODAL->stageclock+=elapsed;
  if (MODAL->stageclock>=1.0) {
    MODAL->stage=STAGE_DISBURSE;
    MODAL->stageclock=0.0;
    
    /* Decide the disbursement for each card, ie who walks away with it.
     * We'll examine each side independantly and trust that the other side is making the same decision.
     */
    int i=MODAL->suitc;
    while (i-->0) {
      int cpucardid=MODAL->cpu_cardidv[i];
      int mancardid=battle_card_of_suit(MODAL->man_cardidv,MODAL->suitc,suit_from_cardid(cpucardid));
      if (mancardid<0) { // This cpu card has a unique suit. Give it to the man.
        MODAL->cpu_disbv[i]=1;
      } else if (cpucardid>mancardid) { // CPU played higher rank.
        MODAL->cpu_disbv[i]=-1;
      } else { // Man played higher rank.
        MODAL->cpu_disbv[i]=1;
      }
      MODAL->cpu_suit_disbv[suit_from_cardid(cpucardid)]=MODAL->cpu_disbv[i];
      mancardid=MODAL->man_cardidv[i];
      cpucardid=battle_card_of_suit(MODAL->cpu_cardidv,MODAL->suitc,suit_from_cardid(mancardid));
      if (cpucardid<0) { // Man card unmatched.
        MODAL->man_disbv[i]=-1;
      } else if (mancardid>cpucardid) { // Man wins.
        MODAL->man_disbv[i]=1;
      } else { // CPU wins.
        MODAL->man_disbv[i]=-1;
      }
      MODAL->man_suit_disbv[suit_from_cardid(mancardid)]=MODAL->man_disbv[i];
    }
    
    /* Then go ahead and effect the disbursement, against our (cpu_hand,man_hand).
     * That change goes live in the world when we dismiss and our caller reads those back.
     */
    for (i=MODAL->suitc;i-->0;) {
      if (MODAL->cpu_disbv[i]>0) { // CPU card goes to the man's hand.
        MODAL->cpu_hand=hand_remove_cardid(MODAL->cpu_hand,MODAL->cpu_cardidv[i]);
        MODAL->man_hand=hand_add_cardid(MODAL->man_hand,MODAL->cpu_cardidv[i]);
      }
      if (MODAL->man_disbv[i]<0) { // Man card goes to the CPU's hand.
        MODAL->man_hand=hand_remove_cardid(MODAL->man_hand,MODAL->man_cardidv[i]);
        MODAL->cpu_hand=hand_add_cardid(MODAL->cpu_hand,MODAL->man_cardidv[i]);
      }
    }
  }
}

/* Update, DISBURSE.
 * Disbursement was affected at the end of ALIGN stage, the moment it was calculated.
 * Just wait for a keystroke and dismiss.
 */
 
static void battle_update_DISBURSE(struct modal *modal,double elapsed,int input) {
  MODAL->stageclock+=elapsed; // For animation only.
  if ((input&EGG_BTN_SOUTH)&&!(MODAL->pvinput&EGG_BTN_SOUTH)) {
    g.input_blackout|=EGG_BTN_SOUTH;
    if (MODAL->cb) {
      MODAL->cb(modal);
      MODAL->cb=0;
    }
    modal->defunct=1;
  }
}

/* Cheat stages.
 * All should wait for SOUTH, then effect the cheat and dismiss.
 */
 
static void battle_update_CHEAT_WIN_ALL(struct modal *modal,double elapsed,int input) {
  if ((input&EGG_BTN_SOUTH)&&!(MODAL->pvinput&EGG_BTN_SOUTH)) {
    g.input_blackout|=EGG_BTN_SOUTH;
    MODAL->man_hand|=MODAL->cpu_hand; // gimme!
    MODAL->cpu_hand=0;
    if (MODAL->cb) {
      MODAL->cb(modal);
      MODAL->cb=0;
    }
    modal->defunct=1;
  }
}

static void battle_update_CHEAT_ABORT(struct modal *modal,double elapsed,int input) {
  if ((input&EGG_BTN_SOUTH)&&!(MODAL->pvinput&EGG_BTN_SOUTH)) {
    g.input_blackout|=EGG_BTN_SOUTH;
    // ABORT happens to be the default behavior if we just terminate.
    if (MODAL->cb) {
      MODAL->cb(modal);
      MODAL->cb=0;
    }
    modal->defunct=1;
  }
}

/* Update.
 */
 
static void _battle_update(struct modal *modal,double elapsed,int input) {
  switch (MODAL->stage) {
    case STAGE_INTRO: battle_update_INTRO(modal,elapsed,input); break;
    case STAGE_PICK: battle_update_PICK(modal,elapsed,input); break;
    case STAGE_REVEAL: battle_update_REVEAL(modal,elapsed,input); break;
    case STAGE_ALIGN: battle_update_ALIGN(modal,elapsed,input); break;
    case STAGE_DISBURSE: battle_update_DISBURSE(modal,elapsed,input); break;
    case STAGE_CHEAT_WIN_ALL: battle_update_CHEAT_WIN_ALL(modal,elapsed,input); break;
    case STAGE_CHEAT_ABORT: battle_update_CHEAT_ABORT(modal,elapsed,input); break;
    default: modal->defunct=1;
  }
  MODAL->pvinput=input;
}

/* Render, INTRO.
 */
 
static void battle_render_INTRO(struct modal *modal) {
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
}

/* Render, PICK.
 */
 
static void battle_render_PICK(struct modal *modal) {

  // Prompt at the very top. eg "Pick four"
  graf_set_input(&g.graf,MODAL->prompt);
  graf_decal(&g.graf,0,0,0,0,MODAL->promptw,MODAL->prompth);
  
  // Interative suit buckets just below that.
  graf_set_input(&g.graf,g.texid_sprites);
  int suity=8;
  int suitx=2;
  int suitdx=13;
  int suit=0;
  for (;suit<4;suit++,suitx+=suitdx) {
    if (suit==MODAL->pickp) { // Highlight.
      graf_decal(&g.graf,suitx-1,suity-1,0,114,11,14);
    }
    graf_decal(&g.graf,suitx,suity,suit*9,64,9,12); // Bucket indicator.
    if (MODAL->pickv[suit]>=0) { // If a card is picked, its mini face goes at the bottom of the bucket.
      int cardy=76;
      if (suit>=2) cardy+=10;
      int cardx=7+7*MODAL->pickv[suit];
      graf_decal(&g.graf,suitx+1,suity+8,cardx,cardy,7,10);
    }
  }
  
  // Confirmation dialogue in the lower half, if we're there.
  if (MODAL->confirmp>=0) {
    graf_set_input(&g.graf,MODAL->okprompt);
    graf_decal(&g.graf,0,31,0,0,MODAL->okpromptw,MODAL->okprompth);
    graf_set_input(&g.graf,g.texid_sprites);
    graf_decal(&g.graf,19,30,41,64,9,9);
    graf_decal(&g.graf,31,30,50,64,9,9);
    int handx=21+MODAL->confirmp*12;
    graf_decal(&g.graf,handx,40,36,64,5,12);
    
  /* If not confirming, show all the available ranks.
   * If five or fewer, their positions are fixed and the hand moves.
   * More than five, the hand is fixed and cards slide under it, infinitely.
   * Don't render anything if the focussed suit has no ranks.
   */
  } else if (MODAL->rankc>0) {
    int handx=FBW;
    if (MODAL->rankc<=5) {
      int totalw=8*MODAL->rankc-1;
      int cardx=(FBW>>1)-(totalw>>1);
      int i=0;
      for (;i<MODAL->rankc;i++,cardx+=8) {
        if (i==MODAL->rankp) {
          handx=cardx+2;
        }
        int srcx=7+7*MODAL->rankv[i];
        int srcy=76;
        if (MODAL->pickp>=2) srcy+=10;
        graf_decal(&g.graf,cardx,29,srcx,srcy,7,10);
      }
    } else {
      int lx=(FBW>>1)-3;
      int lp=MODAL->rankp;
      int rx=lx+8;
      int rp=lp+1;
      handx=lx+2;
      int srcy=(MODAL->pickp>=2)?86:76;
      for (;;) {
        int srcx;
        if (lp<0) lp=MODAL->rankc-1;
        srcx=7+7*MODAL->rankv[lp];
        graf_decal(&g.graf,lx,29,srcx,srcy,7,10);
        if (rp>=MODAL->rankc) rp=0;
        srcx=7+7*MODAL->rankv[rp];
        graf_decal(&g.graf,rx,29,srcx,srcy,7,10);
        lp--; lx-=8;
        rp++; rx+=8;
        if ((lx<-7)&&(rx>=FBW)) break;
      }
    }
    if (MODAL->picking_rank) {
      graf_decal(&g.graf,handx,40,36,64,5,12);
    }
  }
}

/* Render, REVEAL.
 * Stage is fixed duration, 2 seconds long.
 * During the first second, slide back-up cards into position from above and below.
 * After 1 second, they're face-up and stationary.
 */
 
static void battle_render_REVEAL(struct modal *modal) {
  if (MODAL->stageclock<1.0) {
    double offset=(1.0-MODAL->stageclock);
    offset*=22.0;
    int yt=4-lround(offset);
    int yb=30+lround(offset);
    int x=(FBW>>1)-((MODAL->suitc*13-2)>>1);
    int i=MODAL->suitc;
    graf_set_input(&g.graf,g.texid_sprites);
    for (;i-->0;x+=13) {
      graf_decal(&g.graf,x,yt,0,96,11,18);
      graf_decal(&g.graf,x,yb,0,96,11,18);
    }
  } else {
    int yt=4;
    int yb=30;
    int x=(FBW>>1)-((MODAL->suitc*13-2)>>1);
    int i=0;
    graf_set_input(&g.graf,g.texid_cards);
    for (;i<MODAL->suitc;i++,x+=13) {
      graf_decal(&g.graf,x,yt,MODAL->cpu_cardidv[i]*CARDW,0,CARDW,CARDH);
      graf_decal(&g.graf,x,yb,MODAL->man_cardidv[i]*CARDW,0,CARDW,CARDH);
    }
  }
}

/* Render, ALIGN.
 */
 
static void battle_render_ALIGN(struct modal *modal) {
  int x0=(FBW>>1)-((MODAL->suitc*13-2)>>1); // Must match the left edge calculated by battle_render_REVEAL.
  double t=MODAL->stageclock/1.0;
  if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
  graf_set_input(&g.graf,g.texid_cards);
  int cp=MODAL->suitc;
  while (cp-->0) {
    int srcx=x0+cp*13;
    int cpucardid=MODAL->cpu_cardidv[cp];
    int cpudstx=1+suit_from_cardid(cpucardid)*13;
    int mancardid=MODAL->man_cardidv[cp];
    int mandstx=1+suit_from_cardid(mancardid)*13;
    
    int cx=lround(srcx*(1.0-t)+cpudstx*t);
    graf_decal(&g.graf,cx,4,cpucardid*CARDW,0,CARDW,CARDH);
    int mx=lround(srcx*(1.0-t)+mandstx*t);
    graf_decal(&g.graf,mx,30,mancardid*CARDW,0,CARDW,CARDH);
  }
}

/* Render, DISBURSE.
 */
 
static void battle_render_DISBURSE(struct modal *modal) {
  double t=MODAL->stageclock/1.0;
  if (t<0.0) t=0.0; else if (t>1.0) t=1.0;
  graf_set_input(&g.graf,g.texid_cards);
  int cp=MODAL->suitc;
  while (cp-->0) {
    // Match the final horizontal placement from STAGE_ALIGN. Also the initial vertical positions "ya".
    int cpucardid=MODAL->cpu_cardidv[cp];
    int cpudstx=1+suit_from_cardid(cpucardid)*13;
    int mancardid=MODAL->man_cardidv[cp];
    int mandstx=1+suit_from_cardid(mancardid)*13;
    int cpuya=4;
    int manya=30;
    // Final vertical positions "yz" depend on disbursement.
    // We must compare to this card's partner-suit disbursement, which is not necessarily the pair we're looking at.
    int cpuyz=cpuya;
    if (MODAL->cpu_disbv[cp]<0) {
      // CPU is keeping it. Cool.
    } else {
      int mandisb=MODAL->man_suit_disbv[suit_from_cardid(cpucardid)];
      if (mandisb>0) { // Man takes both.
        cpuyz=11;
      } else { // Must be a swap.
        cpuyz=manya;
      }
    }
    int manyz=manya;
    if (MODAL->man_disbv[cp]>0) {
      // Man is keeping it. Cool.
    } else {
      int cpudisb=MODAL->cpu_suit_disbv[suit_from_cardid(mancardid)];
      if (cpudisb<0) { // CPU takes both.
        manyz=23;
      } else { // Must be a swap.
        manyz=cpuya;
      }
    }
    // Then interpolate according to elapsed time.
    int cpuy=lround(cpuya*(1.0-t)+cpuyz*t);
    int many=lround(manya*(1.0-t)+manyz*t);
    graf_decal(&g.graf,cpudstx,cpuy,cpucardid*CARDW,0,CARDW,CARDH);
    graf_decal(&g.graf,mandstx,many,mancardid*CARDW,0,CARDW,CARDH);
  }
}

/* Render, any CHEAT stage.
 */
 
static void battle_render_CHEAT(struct modal *modal) {
  graf_set_input(&g.graf,MODAL->chprompt);
  graf_decal(&g.graf,(FBW>>1)-(MODAL->chpromptw>>1),(FBH>>1)-(MODAL->chprompth>>1),0,0,MODAL->chpromptw,MODAL->chprompth);
}

/* Render.
 */
 
static void _battle_render(struct modal *modal) {

  /* INTRO alone does not blot the background.
   */
  if (MODAL->stage==STAGE_INTRO) {
    battle_render_INTRO(modal);
    return;
  }
  
  /* All the rest start with the green blotter, then do their thing.
   */
  graf_fill_rect(&g.graf,0,0,FBW,FBH,BGCOLOR);
  switch (MODAL->stage) {
    case STAGE_PICK: battle_render_PICK(modal); break;
    case STAGE_REVEAL: battle_render_REVEAL(modal); break;
    case STAGE_ALIGN: battle_render_ALIGN(modal); break;
    case STAGE_DISBURSE: battle_render_DISBURSE(modal); break;
    case STAGE_CHEAT_WIN_ALL: case STAGE_CHEAT_ABORT: battle_render_CHEAT(modal); break;
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

/* Public accessors.
 */
 
void *modal_battle_get_userdata(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->userdata;
}

uint64_t modal_battle_get_cpu_hand(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->cpu_hand;
}

uint64_t modal_battle_get_man_hand(const struct modal *modal) {
  if (!modal||(modal->type!=&modal_type_battle)) return 0;
  return MODAL->man_hand;
}
