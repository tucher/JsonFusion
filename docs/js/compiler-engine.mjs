// ── Worker source (inlined, spawned via blob URL) ───────────────────
const WORKER_SOURCE = `
let mod = null;

function ensureDir(fs, dirPath) {
  const parts = dirPath.split("/").filter(Boolean);
  let cur = "";
  for (const p of parts) {
    cur += "/" + p;
    try { fs.stat(cur); } catch { fs.mkdir(cur); }
  }
}

self.onmessage = async (e) => {
  const msg = e.data;

  if (msg.type === "init") {
    try {
      const blob = new Blob([msg.mjsCode], { type: "application/javascript" });
      const url = URL.createObjectURL(blob);
      const { default: createModule } = await import(url);
      URL.revokeObjectURL(url);

      mod = await createModule({
        locateFile: (path) => msg.wasmBaseUrl + path,
        print: () => {},
        printErr: () => {},
        noInitialRun: true,
      });
      self.postMessage({ type: "ready" });
    } catch (err) {
      self.postMessage({ type: "error", message: err.message });
    }
    return;
  }

  if (msg.type === "writeFiles") {
    for (const f of msg.files) {
      ensureDir(mod.FS, f.path.substring(0, f.path.lastIndexOf("/")));
      mod.FS.writeFile(f.path, f.content);
    }
    self.postMessage({ type: "ack", id: msg.id });
    return;
  }

  if (msg.type === "compile") {
    // Write the source file into FS
    ensureDir(mod.FS, msg.path.substring(0, msg.path.lastIndexOf("/")));
    mod.FS.writeFile(msg.path, msg.code);

    // Build args
    const args = [];
    if (msg.mode === "check-only") args.push("--check-only");
    args.push("--std=" + (msg.std || "c++23"));
    if (msg.resultName && msg.mode !== "check-only") args.push("--result=" + msg.resultName);
    for (const d of (msg.includeDirs || [])) args.push("--include-dir=" + d);
    args.push("--path=" + msg.path);

    let stdout = "";
    mod.print = (s) => { stdout += s + "\\n"; };

    const rc = mod.callMain(args);

    let result = { ok: rc === 0, result: "", error: "", diagnostics: "" };
    try { Object.assign(result, JSON.parse(stdout.trim())); } catch {}

    self.postMessage({ type: "result", id: msg.id, ...result });
    return;
  }
};
`;

// ── CompilerEngine ──────────────────────────────────────────────────

export class CompilerEngine {
  #config;
  #workers = [];
  #available = [];
  #queue = [];       // { id, job, resolve, reject }
  #pending = new Map();
  #nextId = 0;
  #cache = new Map();
  #mjsCode = null;

  /**
   * @param {object} config
   * @param {string} config.wasmBaseUrl  - URL prefix for clang-constexpr-run.mjs/.wasm
   * @param {string} [config.githubOwner]  - For loadStandardHeaders()
   * @param {string} [config.githubRepo]
   * @param {string} [config.githubBranch]
   * @param {number} [config.workerCount] - Number of Web Workers (default: hardwareConcurrency, max 8)
   */
  constructor(config) {
    this.#config = {
      wasmBaseUrl: "",
      githubOwner: "tucher",
      githubRepo: "JsonFusion",
      githubBranch: "master",
      workerCount: Math.min(navigator.hardwareConcurrency || 4, 8),
      ...config,
    };
  }

  /** Fetch .mjs, spawn workers, instantiate WASM in each. */
  async init() {
    const { wasmBaseUrl, workerCount } = this.#config;

    // Fetch the Emscripten glue code once
    const mjsUrl = wasmBaseUrl + "clang-constexpr-run.mjs";
    this.#mjsCode = await this.#fetchText(mjsUrl);

    // Create worker blob
    const workerBlob = new Blob([WORKER_SOURCE], { type: "application/javascript" });
    const workerUrl = URL.createObjectURL(workerBlob);

    // Spawn workers and wait for all to be ready
    const readyPromises = [];
    for (let i = 0; i < workerCount; i++) {
      const w = new Worker(workerUrl);
      this.#workers.push(w);

      readyPromises.push(new Promise((resolve, reject) => {
        const handler = (e) => {
          w.removeEventListener("message", handler);
          if (e.data.type === "ready") resolve();
          else reject(new Error(e.data.message || "Worker init failed"));
        };
        w.addEventListener("message", handler);
      }));

      w.postMessage({ type: "init", mjsCode: this.#mjsCode, wasmBaseUrl });
    }

    await Promise.all(readyPromises);
    URL.revokeObjectURL(workerUrl);

    // Install permanent message handlers and mark all workers available
    for (let i = 0; i < workerCount; i++) {
      this.#workers[i].onmessage = (e) => this.#onMessage(i, e.data);
      this.#available.push(i);
    }
  }

  /**
   * Write files to ALL workers (persistent across compilations).
   * Use for standard headers, test helpers, etc.
   * @param {{path:string, content:string}[]} files
   */
  async writeSharedFiles(files) {
    const id = this.#nextId++;
    const promises = this.#workers.map(w => new Promise(resolve => {
      const handler = (e) => {
        if (e.data.type === "ack" && e.data.id === id) {
          w.removeEventListener("message", handler);
          resolve();
        }
      };
      w.addEventListener("message", handler);
      w.postMessage({ type: "writeFiles", id, files });
    }));
    await Promise.all(promises);
  }

  /**
   * Discover and load JsonFusion + PFR headers from GitHub into all workers.
   * Uses Trees API (1 call) + raw.githubusercontent.com for file contents.
   * @returns {number} Number of headers loaded
   */
  async loadStandardHeaders() {
    const { githubOwner, githubRepo, githubBranch } = this.#config;
    const rawBase = `https://raw.githubusercontent.com/${githubOwner}/${githubRepo}/${githubBranch}`;
    const apiUrl = `https://api.github.com/repos/${githubOwner}/${githubRepo}/git/trees/${githubBranch}?recursive=1`;

    const treeData = JSON.parse(await this.#fetchText(apiUrl));
    const headerPaths = treeData.tree
      .filter(e => e.type === "blob")
      .map(e => e.path)
      .filter(p =>
        (p.startsWith("include/JsonFusion/") || p.startsWith("include/pfr/")) &&
        (p.endsWith(".hpp") || p.endsWith(".h"))
      );

    const urls = headerPaths.map(p => `${rawBase}/${p}`);
    const contents = await this.#fetchAllPooled(urls);

    const files = headerPaths.map((p, i) => ({
      path: "/sysroot/include/" + p.replace(/^include\//, ""),
      content: contents[i],
    }));

    await this.writeSharedFiles(files);
    return files.length;
  }

  /**
   * Compile C++ code.
   * @param {object} opts
   * @param {string} [opts.code]        - Source code text
   * @param {string} [opts.codeUrl]     - URL to fetch source from (cached)
   * @param {string} [opts.path]        - Virtual FS path (default "/input.cpp")
   * @param {string} [opts.std]         - C++ standard (default "c++23")
   * @param {string} [opts.mode]        - "check-only" | "eval" (default "eval")
   * @param {string} [opts.resultName]  - Variable name for eval mode (default "__result")
   * @param {string[]} [opts.includeDirs] - Additional -I paths
   * @returns {Promise<{ok:boolean, result:string, error:string, diagnostics:string}>}
   */
  async compile(opts) {
    let code = opts.code;
    if (opts.codeUrl && !code) {
      code = await this.#fetchText(opts.codeUrl);
    }
    if (!code) throw new Error("compile() requires code or codeUrl");

    const job = {
      code,
      path: opts.path || "/input.cpp",
      std: opts.std || "c++23",
      mode: opts.mode || "eval",
      resultName: opts.resultName || "__result",
      includeDirs: opts.includeDirs || [],
    };

    return new Promise((resolve, reject) => {
      const id = this.#nextId++;
      this.#pending.set(id, { resolve, reject });

      if (this.#available.length > 0) {
        this.#dispatch(this.#available.shift(), id, job);
      } else {
        this.#queue.push({ id, job });
      }
    });
  }

  /** Number of workers in the pool. */
  get workerCount() { return this.#workers.length; }

  /** Terminate all workers. */
  destroy() {
    for (const w of this.#workers) w.terminate();
    this.#workers = [];
    this.#available = [];
    this.#queue = [];
    this.#pending.clear();
  }

  // ── Private ────────────────────────────────────────────────────────

  #dispatch(workerIdx, id, job) {
    this.#workers[workerIdx].postMessage({
      type: "compile",
      id,
      code: job.code,
      path: job.path,
      std: job.std,
      mode: job.mode,
      resultName: job.resultName,
      includeDirs: job.includeDirs,
    });
  }

  #onMessage(workerIdx, msg) {
    if (msg.type !== "result") return;
    const { id, ok, result, error, diagnostics } = msg;
    const p = this.#pending.get(id);
    if (p) {
      this.#pending.delete(id);
      p.resolve({ ok, result, error, diagnostics });
    }
    // Dispatch next queued job
    if (this.#queue.length > 0) {
      const next = this.#queue.shift();
      this.#dispatch(workerIdx, next.id, next.job);
    } else {
      this.#available.push(workerIdx);
    }
  }

  async #fetchText(url) {
    if (this.#cache.has(url)) return this.#cache.get(url);
    const resp = await fetch(url);
    if (!resp.ok) throw new Error(`Fetch failed: ${url} (${resp.status})`);
    const text = await resp.text();
    this.#cache.set(url, text);
    return text;
  }

  async #fetchAllPooled(urls, concurrency = 30) {
    const results = new Array(urls.length);
    let idx = 0;
    const self = this;
    async function worker() {
      while (idx < urls.length) {
        const i = idx++;
        results[i] = await self.#fetchText(urls[i]);
      }
    }
    const workers = [];
    for (let w = 0; w < Math.min(concurrency, urls.length); w++) {
      workers.push(worker());
    }
    await Promise.all(workers);
    return results;
  }
}
