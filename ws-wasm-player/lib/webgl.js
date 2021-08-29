class Texture {
  constructor(gl) {
    this.gl = gl;
    this.texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
  }

  bind(n, program, name) {
    const gl = this.gl;
    gl.activeTexture(gl.TEXTURE0 + n);
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.uniform1i(gl.getUniformLocation(program, name), n);
  }

  fill(width, height, data, format) {
    const gl = this.gl;
    format = format || gl.LUMINANCE;
    gl.bindTexture(gl.TEXTURE_2D, this.texture);
    gl.texImage2D(gl.TEXTURE_2D, 0, format, width, height, 0, format,
                  gl.UNSIGNED_BYTE, data);
  }
};

class WebGLPlayer {
  constructor(canvas) {
    this.canvas = canvas;
    this.gl = canvas.getContext("webgl") || canvas.getContext("experimental-webgl");
    this.#init();
  }

  #init() {
    if (!this.gl) {
      console.log("[ERROR] WebGL not supported");
      return;
    }

    const gl = this.gl;
    gl.pixelStorei(gl.UNPACK_ALIGNMENT, 1);

    const program = gl.createProgram();

    const vertexShaderSource = [
      "attribute highp vec3 aPos;",
      "attribute vec2 aTexCoord;",
      "varying highp vec2 vTexCoord;",
      "void main(void) {",
      "  gl_Position = vec4(aPos, 1.0);",
      "  vTexCoord = aTexCoord;",
      "}",
    ].join("\n");
    const vertexShader = gl.createShader(gl.VERTEX_SHADER);
    gl.shaderSource(vertexShader, vertexShaderSource);
    gl.compileShader(vertexShader);
    {
      const msg = gl.getShaderInfoLog(vertexShader);
      if (msg) {
        console.log("[ERROR] Vertex shader compile failed");
        console.log(msg);
      }
    }

    const fragmentShaderSource = [
      "precision highp float;",
      "varying lowp vec2 vTexCoord;",
      "uniform sampler2D yTex;",
      "uniform sampler2D uTex;",
      "uniform sampler2D vTex;",
      "const mat4 YUV2RGB = mat4(",
      "  1.1643828125,             0, 1.59602734375, -.87078515625,",
      "  1.1643828125, -.39176171875,    -.81296875,     .52959375,",
      "  1.1643828125,   2.017234375,             0,  -1.081390625,",
      "             0,             0,             0,             1",
      ");",
      "void main(void) {",
      "  // gl_FragColor = vec4(vTexCoord.x, vTexCoord.y, 0., 1.0);",
      "  gl_FragColor = vec4(",
      "    texture2D(yTex, vTexCoord).x,",
      "    texture2D(uTex, vTexCoord).x,",
      "    texture2D(vTex, vTexCoord).x,",
      "    1",
      "  ) * YUV2RGB;",
      "}",
    ].join("\n");
    const fragmentShader = gl.createShader(gl.FRAGMENT_SHADER);
    gl.shaderSource(fragmentShader, fragmentShaderSource);
    gl.compileShader(fragmentShader);
    {
      const msg = gl.getShaderInfoLog(fragmentShader);
      if (msg) {
        console.log("[ERROR] Fragment shader compile failed");
        console.log(msg);
      }
    }

    gl.attachShader(program, vertexShader);
    gl.attachShader(program, fragmentShader);
    gl.linkProgram(program);
    gl.useProgram(program);
    if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
      console.log("[ERROR] Shader link failed");
    }

    const vertices = new Float32Array([
      // positions      // texture coords
      -1.0, -1.0, 0.0,  0.0, 1.0,  // bottom left
       1.0, -1.0, 0.0,  1.0, 1.0,  // bottom right
      -1.0,  1.0, 0.0,  0.0, 0.0,  // top left
       1.0,  1.0, 0.0,  1.0, 0.0,  // top right
    ])
    const verticesBuffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, verticesBuffer);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);

    const vertexPositionAttribute = gl.getAttribLocation(program, "aPos");
    gl.enableVertexAttribArray(vertexPositionAttribute);
    gl.vertexAttribPointer(vertexPositionAttribute, 3, gl.FLOAT, false, 20, 0);

    const textureCoordAttribute = gl.getAttribLocation(program, "aTexCoord");
    gl.enableVertexAttribArray(textureCoordAttribute);
    gl.vertexAttribPointer(textureCoordAttribute, 2, gl.FLOAT, false, 20, 12);

    gl.y = new Texture(gl);
    gl.u = new Texture(gl);
    gl.v = new Texture(gl);
    gl.y.bind(0, program, "yTex");
    gl.u.bind(1, program, "uTex");
    gl.v.bind(2, program, "vTex");
  }

  render(frame) {
    if (!this.gl) {
      console.log("[ERROR] Render failed due to WebGL not supported");
      return;
    }

    const gl = this.gl;
    gl.viewport(0, 0, gl.canvas.width, gl.canvas.height);
    gl.clearColor(0.0, 0.0, 0.0, 0.0);
    gl.clear(gl.COLOR_BUFFER_BIT);

    {
      const width = frame.width;
      const height = frame.height;
      const bytes = frame.bytes;

      const len_y = width * height;
      const len_u = len_y >> 2;
      const len_uv = len_y >> 1;

      gl.y.fill(width, height, bytes.subarray(0, len_y));
      gl.u.fill(width >> 1, height >> 1, bytes.subarray(len_y, len_y+len_u));
      gl.v.fill(width >> 1, height >> 1, bytes.subarray(len_y+len_u, len_y+len_uv));
    }

    gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
  }

  fullscreen() {
    const canvas = this.canvas;
    if (canvas.RequestFullScreen) {
      canvas.RequestFullScreen();
    } else if (canvas.webkitRequestFullScreen) {
      canvas.webkitRequestFullScreen();
    } else if (canvas.mozRequestFullScreen) {
      canvas.mozRequestFullScreen();
    } else if (canvas.msRequestFullscreen) {
      canvas.msRequestFullscreen();
    } else {
      alert("This browser doesn't support fullscreen");
    }
  }

  exitFullscreen() {
    if (document.exitFullscreen) {
      document.exitFullscreen();
    } else if (document.webkitExitFullscreen) {
      document.webkitExitFullscreen();
    } else if (document.mozCancelFullScreen) {
      document.mozCancelFullScreen();
    } else if (document.msExitFullscreen) {
      document.msExitFullscreen();
    } else {
      alert("Exit fullscreen doesn't work");
    }
  }
};
