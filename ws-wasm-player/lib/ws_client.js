const WsClientOptions = {
  // required on open
  url: null,
  stream: null,

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

  constructor(options) {
    this.#options = { ...WsClientOptions, ...options };
    this.#logd('ws options:')
    this.#logd(this.#options);

    Module.Log.set_v(this.#options.wasm_log_v);
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
    this.#decoder.Open(JSON.stringify(this.#options.stream));

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
      this.#ws.close();
      this.#ws = null;
      this.#decoder.delete();
      this.#decoder = null;
    }
  }

  #onopen(e) {
    this.#logd(`ws open: ${this.#options.url}`);
    this.#options.onopen && this.#options.onopen(e);
  }

  #onmessage(e) {
    this.#logd(`ws message: ${this.#options.url}`);
    this.#options.onmessage && this.#options.onmessage(e);

    let data = new Uint8Array(e.data);
    if (this.#decoder) {
      const buf = Module._malloc(data.length);
      try {
        Module.HEAPU8.set(data, buf);
        const frame = this.#decoder.Decode(buf, data.length);
        if (frame != null) {
          this.#logd(`ws frame size=${frame.width}x${frame.height}`);
          frame.bytes = new Uint8Array(Module.HEAPU8.buffer, frame.data, frame.size);
          this.#logd(frame);
          if (this.#options.player) {
            this.#options.player.render(frame);
          }
          this.#options.ondata && this.#options.ondata(frame);
          frame.delete();
        } else {
          this.#logd("ws frame error: decode fail");
        }
      } finally {
        Module._free(buf);
      }
    } else {
      this.#options.ondata && this.#options.ondata(data);
    }
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