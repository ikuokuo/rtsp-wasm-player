const WsClientOptions = {
  url: null,
  player: null,

  onopen: null,
  onmessage: null,
  ondata: null,
  onclose: null,
  onerror: null,

  dataHandler: null,

  dbg: true,
  log: console.log,
};

class WsClient {
  #options;
  #ws = null;

  constructor(options) {
    this.#options = { ...WsClientOptions, ...options };
    this.#logd('ws options:')
    this.#logd(this.#options);
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
    }
  }

  #onopen(e) {
    this.#logd(`ws open: ${this.#options.url}`);
    this.#options.onopen && this.#options.onopen(e);
  }

  #onmessage(e) {
    this.#logd(`ws message: ${this.#options.url}`);
    this.#options.onmessage && this.#options.onmessage(e);

    let data = e.data;
    if (this.#options.dataHandler) {
      data = this.#options.dataHandler(data);
    }
    if (this.#options.player) {
      this.#options.player.render();
    }
    this.#logd(data);
    this.#options.ondata && this.#options.ondata(data);
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
