const WsClientOptions = {
  // required on open
  url: null,
  stream: null,

  decode_async: false,
  decode_queue_size: 0,
  decode_thread_count: 0,
  decode_thread_type: 0,

  // need pthreads support
  //  COOP, COEP headers are correctly set
  //  https://emscripten.org/docs/porting/pthreads.html
  // decode_async: true,
  // decode_queue_size: 10,
  // decode_thread_count: 4,
  // // 1: FF_THREAD_FRAME, 2: FF_THREAD_SLICE
  // decode_thread_type: 1,

  player: null,

  onopen: null,
  onmessage: null,
  onclose: null,
  onerror: null,

  ondata: null,

  dbg: false,
  log: console.log,
  wasm_log_v: 0,
};

class WsClient {
  #options;
  #ws = null;
  #decoder = null;
  #t_onmsg = Date.now();

  constructor(options) {
    this.#options = { ...WsClientOptions, ...options };
    this.#logd('ws options:')
    this.#logd(this.#options);

    Module.Log.set_v(this.#options.wasm_log_v);
  }

  static createOpenGLPlayer() {
    return new Module.OpenGLPlayer();
  }

  #log(...args) {
    this.#options.log(...args);
  }

  #logd(...args) {
    this.#options.dbg && this.#options.log(...args);
  }

  isOpen() {
    return this.#ws != null;
  }

  open(options) {
    if (this.#ws != null) {
      this.#log('ws open error: already opened');
      return;
    }
    this.#options = { ...this.#options, ...options };
    this.#logd('ws open options:')
    this.#logd(this.#options);
    if (this.#options.url == null) {
      this.#log('ws open error: url is null');
      return;
    }
    if (this.#options.stream == null) {
      this.#log('ws open error: stream is null');
      return;
    }

    this.#decoder = new Module.Decoder();
    this.#decoder.open(
        JSON.stringify(this.#options.stream),
        this.#options.decode_queue_size,
        this.#options.decode_thread_count,
        this.#options.decode_thread_type,
        this.#ondecode.bind(this));

    const ws = new WebSocket(this.#options.url);
    ws.binaryType = 'arraybuffer';
    ws.onopen = (e) => this.#onopen(e);
    ws.onmessage = (e) => this.#onmessage(e);
    ws.onclose = (e) => this.#onclose(e);
    ws.onerror = (e) => this.#onerror(e);
    this.#ws = ws;
  }

  close() {
    if (this.#ws != null) {
      this.#logd('ws close');
      this.#ws.close();
      this.#ws = null;
      this.#decoder.delete();
      this.#decoder = null;
      this.#options.dbg && Module.DoLeakCheck && Module.DoLeakCheck();
    }
  }

  #onopen(e) {
    this.#logd(`ws open: ${this.#options.url}`);
    this.#options.onopen && this.#options.onopen(e);
  }

  #onmessage(e) {
    if (this.#options.dbg) {
      const t = Date.now();
      this.#logd(`ws message: ${this.#options.url}` +
          `, interval: ${t - this.#t_onmsg} ms`);
      this.#t_onmsg = t;
    }
    this.#options.dbg && console.time("ws onmessage");

    let data = new Uint8Array(e.data);
    if (this.#decoder) {
      const buf = Module._malloc(data.length);
      try {
        Module.HEAPU8.set(data, buf);
        if (this.#options.decode_async) {
          this.#decoder.decodeAsync(buf, data.length);
        } else {
          // process the frame in #ondecode callback
          const frame = this.#decoder.decode(buf, data.length);
          // release the return frame reference
          if (frame != null) frame.delete();
        }
      } finally {
        Module._free(buf);
      }
    } else {
      this.#options.ondata && this.#options.ondata(data);
    }

    this.#options.dbg && console.timeEnd("ws onmessage");
  }

  #ondecode(frame) {
    this.#options.dbg && console.time("ws ondecode");
    if (frame != null) {
      this.#logd(`ws frame size=${frame.width}x${frame.height}`);
      const player = this.#options.player;
      if (player) {
        if (player instanceof WebGLPlayer) {
          frame.bytes = frame.getBytes();
          // frame.bytes = new Uint8Array(Module.HEAPU8.buffer, frame.data, frame.size);
        }
        player.render(frame);
      }
      this.#options.ondata && this.#options.ondata(frame);
      frame.delete();
    } else {
      this.#logd("ws frame is null: decode error or need new packets");
    }
    this.#options.dbg && console.timeEnd("ws ondecode");
  }

  #onclose(e) {
    this.#logd(`ws close: ${this.#options.url}`);
    this.#options.onclose && this.#options.onclose(e);
  }

  #onerror(e) {
    this.#logd(`ws error: ${this.#options.url}, ${e}`);
    this.#options.onerror && this.#options.onerror(e);
  }
}
