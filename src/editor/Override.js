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
        const msg = `Tile at (${ai%colc},${Math.floor(ai/colc)}) is repeated ${dups.length} times, first at (${dups[0]%colc},${Math.floor(dups[0]/colc)}).`;
        errors.push(msg);
      }
    }
    
    if (errors.length) throw errors.join("\n");
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
