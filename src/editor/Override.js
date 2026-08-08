/* This file exists only to be overridden by clients.
 * You can inject dependencies just like any other class.
 */
 
import { PhysicsEditor } from "./PhysicsEditor.js";
import { Data } from "./js/Data.js";
import { Dom } from "./js/Dom.js";
 
export class Override {
  static getDependencies() {
    return [Data, Dom];
  }
  constructor(data, dom) {
    this.data = data;
    this.dom = dom;
    
    this.actions = [
      { name: "validateTiles", label: "Validate Tiles", fn: () => this.validateTiles() },
    ];
    this.editors = [
      PhysicsEditor,
    ];
    this.poiIconGenerators = {
    };
  }
  
  validateTiles() {
    this.data.getImageAsync("world").then(image => {
      // Is there really no way to get an ImageData from an Image, without an intermediate Canvas and rendering context?
      const canvas = this.dom.spawn(null, "CANVAS", { width: image.naturalWidth, height: image.naturalHeight });
      const ctx = canvas.getContext("2d");
      ctx.drawImage(image, 0, 0);
      const imageData = ctx.getImageData(0, 0, image.naturalWidth, image.naturalHeight);
      this.validateTilesInner(imageData);
      this.dom.toast("Image validated. Looks good.");
    }).catch(e => {
      this.dom.modalError(e);
      console.error(e);
    });
  }
  
  /* ImageData in.
   * Throws a sensible exception if anything is wrong (even business level things like a tile mismatch).
   * If we return, report success.
   * We'll scan for a PhysicsEditor and if present, we'll deliver a highlight image to it.
   */
  validateTilesInner(d) {
    const colc = 64;
    const rowc = 64;
    const colw = 8;
    const rowh = 8;
    const imgw = colc * colw;
    const imgh = rowc * rowh;
    if ((d.width !== imgw) || (d.height !== imgh) || (d.data.length !== imgw * imgh * 4)) {
      throw new Error(`Invalid image dimensions ${d.width},${d.height}. Expected ${imgw},${imgh}`);
    }
    
    /* If there's a PhysicsEditor, retain it and prepare a highlight image.
     */
    let physicsEditor = null;
    let highlightImage = null;
    let highlightCtx = null;
    const peElement = this.dom.document.querySelector(".PhysicsEditor");
    if (peElement) physicsEditor = peElement.__egg_controller;
    if (physicsEditor && !(physicsEditor instanceof PhysicsEditor)) peElement = null;
    if (physicsEditor) {
      highlightImage = this.dom.spawn(null, "CANVAS", { width: imgw, height: imgh });
      highlightCtx = highlightImage.getContext("2d");
    } else {
      console.log(`Run Validate Tiles with PhysicsEditor open to see the dups.`);
    }
    
    /* Examine each tile LRTB and write its hash into a new array.
     */
    const hashes = []; // Length eventually (colc*rowc) ie 4096. Value is int.
    for (let dp=0, yi=rowc; yi-->0; dp+=imgw*rowh*4) {
      for (let subdp=dp, xi=colc; xi-->0; subdp+=colw*4) {
        hashes.push(this.hashTile(d.data, subdp, colw, rowh, imgw*4));
      }
    }
    
    /* Now check every hash against every other.
     * If there's a collision, check the full pixels against each other and if it's a real collision, note it.
     * We want a single line for each duplicated tile, something like "Tile at (1,2) is repeated 65 times, first at (2,2)."
     * Any future hash that we identify as a duplicate, zero it so we don't reexamine.
     */
    const errors = [];
    for (let ai=0; ai<hashes.length; ai++) {
      const a = hashes[ai];
      if (!a) continue; // Already marked as a duplicate of something earlier.
      const dups = [];
      for (let bi=ai+1; bi<hashes.length; bi++) {
        if (hashes[bi] !== a) continue;
        if (this.tilesIdentical(d, ai%colc, Math.floor(ai/colc), bi%colc, Math.floor(bi/colc), colw, rowh)) {
          hashes[bi] = 0;
          dups.push(bi);
        }
      }
      if (dups.length > 0) {
        if (highlightCtx) {
          highlightCtx.fillStyle = this.colorForSequence(errors.length);
          highlightCtx.fillRect((ai%colc) * colw, Math.floor(ai/colc) * rowh, colw, rowh); // (ai) is not in (dups)
          for (const ix of dups) {
            const col = ix % colc;
            const row = Math.floor(ix / colc);
            highlightCtx.fillRect(col * colw, row * rowh, colw, rowh);
          }
        }
        const msg = `Tile at (${ai%colc},${Math.floor(ai/colc)}) is repeated ${dups.length} times, first at (${dups[0]%colc},${Math.floor(dups[0]/colc)}).`;
        errors.push(msg);
      }
    }
    
    if (errors.length) {
      if (physicsEditor) physicsEditor.setDupsHighlight(highlightImage);
      throw errors.join("\n");
    } else {
      if (physicsEditor) physicsEditor.setDupsHighlight(null);
    }
  }
  
  colorForSequence(ix) {
    switch (ix % 14) {
      case 0: return "#f00";
      case 1: return "#0f0";
      case 2: return "#00f";
      case 3: return "#ff0";
      case 4: return "#f0f";
      case 5: return "#0ff";
      case 6: return "#fff";
      case 7: return "#888";
      case 8: return "#f08";
      case 9: return "#f80";
      case 10: return "#8f0";
      case 11: return "#80f";
      case 12: return "#08f";
      case 13: return "#0f8";
    }
    return "#888";
  }
  
  hashTile(src, srcp, w, h, stride) {
    let hash = 0;
    for (let yi=h; yi-->0; srcp+=stride) {
      for (let xi=w*4, subp=srcp; xi-->0; subp++) {
        hash <<= 1;
        hash ^= src[subp];
      }
    }
    if (!hash) return 1; // Don't let it be zero; that will be a marker value.
    return hash;
  }
  
  tilesIdentical(src, acol, arow, bcol, brow, colw, rowh) {
    const stride = src.width * 4;
    const bc = colw * 4;
    let arp = ((arow * rowh) * src.width + acol * colw) * 4;
    let brp = ((brow * rowh) * src.width + bcol * colw) * 4;
    for (let yi=rowh; yi-->0; arp+=stride, brp+=stride) {
      for (let xi=bc, ap=arp, bp=brp; xi-->0; ap++, bp++) {
        if (src.data[ap] !== src.data[bp]) return false;
      }
    }
    return true;
  }
};

Override.singleton = true;
