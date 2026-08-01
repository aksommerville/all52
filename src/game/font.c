#include "all52.h"

#define GLYPHW 3
#define GLYPHH 5
#define XSPACING 4
#define YSPACING 6
#define SRCW (GLYPHW*16)
#define SRCH (GLYPHH*6)

/* Global source image.
 * We copy this from image:font, and convert to i8. But only using values 0 or 1.
 */
 
static uint8_t font_src[SRCW*SRCH];
static int font_loaded=0;

static int font_require() {
  if (font_loaded) return 0;
  uint32_t *tmp=malloc(SRCW*SRCH*4);
  if (!tmp) return -1;
  if (egg_texture_get_pixels(tmp,SRCW*SRCH*4,g.texid_font)<0) {
    free(tmp);
    return -1;
  }
  uint32_t amask=0x000000ff;
  uint32_t bodetect=0x44332211;
  if (*(uint8_t*)&bodetect==0x11) amask=0xff000000;
  uint8_t *dstp=font_src;
  const uint32_t *srcp=tmp;
  int i=SRCW*SRCH;
  for (;i-->0;dstp++,srcp++) {
    if ((*srcp)&amask) *dstp=1;
    else *dstp=0;
  }
  free(tmp);
  font_loaded=1;
  return 0;
}

/* Break one line.
 */
 
int font_break_line(const char *src,int srcc,int wlimit) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcw=0,srcp=0;
  for (;;) {
  
    // End of input, always a good place to stop.
    if (srcp>=srcc) return srcp;
    
    // We can always consume whitespace, but an LF stops us dead after consuming.
    if (src[srcp]==0x0a) {
      srcp++;
      return srcp;
    }
    if (((unsigned char)src[srcp]<=0x20)||((unsigned char)src[srcp]>0x7f)) {
      srcp++;
      srcw+=XSPACING;
      continue;
    }
    
    // Measure the next word. Words to us are anything that isn't whitespace.
    int wordlen=0;
    while ((srcp+wordlen<srcc)&&((unsigned char)src[srcp+wordlen]>0x20)) wordlen++;
    int wordw=wordlen*XSPACING;
    
    // If we're off the start of (src), and this word exceeds (wlimit), forget it, return.
    if (srcp&&(srcw+wordw>wlimit)) return srcp;
    
    // Consume this word and carry on.
    srcp+=wordlen;
    srcw+=wordw;
  }
}

/* Measure text 2-dimensionally.
 */

void font_measure(int *w,int *h,const char *src,int srcc,int wlimit) {
  *w=*h=0;
  if (!src) return;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int srcp=0,linec=0;
  while (srcp<srcc) {
    int linelen=font_break_line(src+srcp,srcc-srcp,wlimit);
    if (linelen<=0) return;
    int truelen=linelen;
    while ((truelen>0)&&((unsigned char)src[srcp+truelen-1]<=0x20)) truelen--;
    int linew=XSPACING*truelen-1;
    if (linew>*w) *w=linew;
    linec++;
    srcp+=linelen;
  }
  if (linec) *h=linec*YSPACING-1;
}

/* Copy from i8 (0,1) to i32.
 */
 
static void font_copy_bits(uint32_t *dst,int stridewords,const uint8_t *src,int w,int h,uint32_t pixel) {
  for (;h-->0;dst+=stridewords,src+=SRCW) {
    uint32_t *dstp=dst;
    const uint8_t *srcp=src;
    int xi=w;
    for (;xi-->0;dstp++,srcp++) {
      if (*srcp) *dstp=pixel;
    }
  }
}

/* Print single line to RGBA buffer.
 */

int font_print_line(void *_dst,int dstw,int dsth,int dststride,const char *src,int srcc,uint32_t rgba) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  
  uint32_t bodetect=0x44332211;
  if ((*(uint8_t*)&bodetect)==0x11) { // little-endian, extremely likely
    rgba=(rgba>>24)|(rgba<<24)|((rgba&0xff0000)>>8)|((rgba&0xff00)<<8);
  }
  uint32_t *dst=_dst;
  dststride>>=2;
  if (dsth>GLYPHH) dsth=GLYPHH;
  
  if (font_require()<0) return 0;
  
  int dstx=0;
  for (;srcc-->0;src++,dstx+=XSPACING) {
    if (dstx>=dstw) break;
    int ch=*src;
    if ((ch<=0x20)||(ch>0x7f)) continue;
    int srcx=(ch&0x0f)*GLYPHW;
    int srcy=((ch-0x20)>>4)*GLYPHH;
    int cpw=GLYPHW;
    if (dstx>dstw-cpw) cpw=dstw-dstx;
    font_copy_bits(dst+dstx,dststride,font_src+srcy*SRCW+srcx,cpw,dsth,rgba);
  }
  
  return dstx;
}

/* Generate shadow.
 */
 
static void font_generate_shadow(uint32_t *v,int w,int h) {
  int i=w*(h-1);
  uint32_t *p=v+w*h;
  const uint32_t *rp=p-w;
  for (;i-->0;p--,rp--) if (*rp&&!*p) *p=0x40000000;
}

/* Render to new texture.
 */

int font_render_multiline(const char *src,int srcc,int wlimit,uint32_t rgba,int margin) {
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int shadow=0;
  if (margin<0) {
    shadow=1;
    margin=-margin;
  }
  wlimit-=margin<<1;
  int w=0,h=0;
  font_measure(&w,&h,src,srcc,wlimit);
  if (w<1) w=1;
  if (h<1) h=1;
  int imgw=w+(margin<<1);
  int imgh=h+(margin<<1);
  uint32_t *tmp=calloc(4,imgw*imgh);
  if (!tmp) return -1;
  int srcp=0,dsty=margin;
  while (srcp<srcc) {
    int linelen=font_break_line(src+srcp,srcc-srcp,wlimit);
    if (linelen<1) break;
    font_print_line(tmp+dsty*imgw+margin,imgw-margin,imgh-dsty,imgw<<2,src+srcp,linelen,rgba);
    dsty+=YSPACING;
    srcp+=linelen;
  }
  if (shadow) font_generate_shadow(tmp,imgw,imgh);
  int texid=egg_texture_new();
  if (texid<1) {
    free(tmp);
    return -1;
  }
  int err=egg_texture_load_raw(texid,imgw,imgh,imgw<<2,tmp,imgw*imgh*4);
  free(tmp);
  if (err<0) {
    egg_texture_del(texid);
    return -1;
  }
  return texid;
}
