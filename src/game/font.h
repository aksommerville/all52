/* font.h
 * Egg provides a similar font unit, but it's overkill.
 * All of our text is monospaced ASCII, 3x5-pixel glyphs.
 */
 
#ifndef FONT_H
#define FONT_H

/* Returns the length of (src) to consume for one line of text that hopefully fits in (wlimit).
 * Includes trailing space and possibly a trailing newline.
 */
int font_break_line(const char *src,int srcc,int wlimit);

/* How big an image is required to hold this text?
 * We return something greater than (wlimit) if (wlimit) is too small for the longest word.
 * Our bounds do not include any margin. We'll touch all four edges.
 */
void font_measure(int *w,int *h,const char *src,int srcc,int wlimit);

/* Render one line of text to an RGBA buffer.
 * Newlines are treated like spaces.
 */
int font_print_line(void *dst,int dstw,int dsth,int dststride,const char *src,int srcc,uint32_t rgba);

/* Generate a texture ideally no wider than (wlimit), including (margin).
 * (margin<0) for a cheesy shadow effect.
 */
int font_render_multiline(const char *src,int srcc,int wlimit,uint32_t rgba,int margin);

#endif
