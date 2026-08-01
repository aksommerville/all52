import { Dom } from "./js/Dom.js";
import { Data } from "./js/Data.js";

export class PhysicsEditor {
  static getDependencies() {
    return [HTMLElement, Dom, Data, Window];
  }
  constructor(element, dom, data, window) {
    this.element = element;
    this.dom = dom;
    this.data = data;
    this.window = window;
    
    this.res = null;
    this.renderTimeout = null;
    this.view = null; // Canvas
    this.focus = [256, 256]; // Center of view.
    this.zoom = 1; // Higher to magnify.
    this.image = null;
    this.capturedPointerId = 0;
    this.penMode = true; // true to set a bit (ie make solid), false to clear (ie make vacant)
    this.penx = -1;
    this.peny = -1;
    this.dirty = false;
    
    this.data.getImageAsync("world").then(img => {
      this.image = img;
      this.renderSoon();
    }).catch(e => {
      console.log(`Error loading world image`, e);
    });
  }
  
  static checkResource(res) {
    if (res.type === "physics") return 2;
    return 0;
  }
  
  onRemoveFromDom() {
    if (this.renderTimeout) {
      this.window.clearTimeout(this.renderTimeout);
      this.renderTimeout = null;
    }
  }
  
  setup(res) {
    this.res = res;
    if (this.res.serial.length !== 512) {
      throw new Error(`PhysicsEditor expected 512 bytes, found ${this.res.serial.length}`);
    }
    this.buildUi();
  }
  
  buildUi() {
    this.element.innerHTML = "";
    if (!this.res) return;
    const bounds = this.element.getBoundingClientRect();
    this.view = this.dom.spawn(this.element, "CANVAS", ["view"], {
      "on-pointerdown": e => this.onPointerDown(e),
      "on-pointerup": e => this.onPointerUp(e),
      "on-pointermove": e => this.onPointerMove(e),
      "on-mousewheel": e => this.onMouseWheel(e),
    });
    this.view.width = bounds.width;
    this.view.height = bounds.height;
    this.renderSoon();
  }
  
  renderSoon() {
    if (this.renderTimeout) return;
    this.renderTimeout = this.window.setTimeout(() => {
      this.renderTimeout = null;
      this.renderNow();
    }, 50);
  }
  
  renderNow() {
    if (!this.view) return;
    const ctx = this.view.getContext("2d");
    ctx.imageSmoothingEnabled = false;
    ctx.fillStyle = "#000";
    ctx.fillRect(0, 0, this.view.width, this.view.height);
    
    /* Determine the bounds in view, in source pixels.
     */
    const srcw = this.view.width / this.zoom;
    const srch = this.view.height / this.zoom;
    const srcx = this.focus[0] - srcw * 0.5;
    const srcy = this.focus[1] - srch * 0.5;
    
    /* Show the map's pixels.
     */
    if (this.image) {
      ctx.drawImage(this.image, srcx, srcy, srcw, srch, 0, 0, this.view.width, this.view.height);
    }
    
    /* Show the physics.
     */
    if (this.res) {
      const src = this.res.serial;
      const colw = 8 * this.zoom;
      const rowh = 8 * this.zoom;
      ctx.fillStyle = "#f00c";
      let srcp = 0;
      for (let row=0; row<64; row++) {
        for (let col=0, mask=0x80; col<64; col++) {
          if (src[srcp] & mask) {
            ctx.fillRect(col * colw - srcx*this.zoom, row * rowh - srcy*this.zoom, colw, rowh);
          }
          if (mask === 0x01) {
            mask = 0x80;
            srcp++;
          } else {
            mask >>= 1;
          }
        }
      }
    }
  }
  
  onPointerDown(event) {
    if (!this.res) return;
    this.view.setPointerCapture(event.pointerId);
    this.capturedPointerId = event.pointerId;
    this.onPointerMove(event, true);
  }
  
  onPointerUp(event) {
    this.capturedPointerId = 0;
    if (this.dirty) {
      this.dirty = false;
      this.data.dirty(this.res.path, () => this.res.serial);
    }
  }
  
  onPointerMove(event, starting) {
    if (event.pointerId !== this.capturedPointerId) return;
    if (!this.view) return;
    const bounds = this.view.getBoundingClientRect();
    const srcw = this.view.width / this.zoom;
    const srch = this.view.height / this.zoom;
    const srcx = this.focus[0] - srcw * 0.5;
    const srcy = this.focus[1] - srch * 0.5;
    const px = srcx + ((event.x - bounds.x) * srcw) / bounds.width;
    const py = srcy + ((event.y - bounds.y) * srch) / bounds.height;
    const col = Math.floor(px / 8);
    const row = Math.floor(py / 8);
    if (starting) {
      this.penx = -1;
      this.peny = -1;
      if ((col < 0) || (row < 0) || (col >= 64) || (row >= 64)) this.penMode = true;
      else this.penMode = (this.res.serial[row * 8 + (col >> 3)] & (0x80 >> (col & 7))) ? false : true;
    }
    if ((col < 0) || (row < 0) || (col >= 64) || (row >= 64)) return;
    if ((col === this.penx) && (row === this.peny)) return;
    this.penx = col;
    this.peny = row;
    const p = row * 8 + (col >> 3);
    const mask = 0x80 >> (col & 7);
    if (this.penMode) {
      if (!(this.res.serial[p] & mask)) {
        this.res.serial[p] |= mask;
        this.dirty = true;
      }
    } else {
      if (this.res.serial[p] & mask) {
        this.res.serial[p] &= ~mask;
        this.dirty = true;
      }
    }
    this.renderSoon();
  }
  
  onMouseWheel(event) {
    event.preventDefault();
    event.stopPropagation();
    
    /* Control to zoom.
     */
    if (event.ctrlKey) {
      const zoomRate = 1.500;
      if (event.wheelDelta > 0) this.zoom *= zoomRate;
      else this.zoom /= zoomRate;
      this.renderSoon();
      return;
    }
    
    /* Shift for horizontal, neutral for vertical.
     */
    let nx=0, ny=0;
    if (event.shiftKey) nx = 1;
    else ny = 1;
    if (event.wheelDelta > 0) {
      nx *= -1;
      ny *= -1;
    }
    
    /* Estimate magnitude of move based on zoom.
     */
    const mag = (this.view.height * 0.250) / this.zoom;
    this.focus[0] += nx * mag;
    this.focus[1] += ny * mag;
    this.renderSoon();
  }
}
