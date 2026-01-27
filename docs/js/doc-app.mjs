// ── Markdown-it setup ──────────────────────────────────────────────
const md = window.markdownit({
  html: true,
  linkify: true,
  highlight(str, lang) {
    if (lang && hljs.getLanguage(lang)) {
      try { return hljs.highlight(str, { language: lang }).value; } catch {}
    }
    return "";
  },
});

// Override fence renderer to detect "cpp live" and "mermaid" blocks
const defaultFence = md.renderer.rules.fence ||
  function (tokens, idx, options, env, self) {
    return self.renderToken(tokens, idx, options);
  };

md.renderer.rules.fence = function (tokens, idx, options, env, self) {
  const token = tokens[idx];
  const info = token.info.trim();

  // ── Live C++ editor block ──────────────────────────────────────
  if (info.startsWith("cpp") && info.includes("live")) {
    const code = token.content;
    const escaped = code.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");

    // Parse options from info string: "cpp live check-only result=myVar"
    const parts = info.split(/\s+/);
    const checkOnly = parts.includes("check-only");
    const resultPart = parts.find(p => p.startsWith("result="));
    const resultName = resultPart ? resultPart.split("=")[1] : "__result";
    const mode = checkOnly ? "check-only" : "eval";

    return `<div class="live-editor" data-code="${escaped}" data-mode="${mode}" data-result-name="${resultName}"></div>`;
  }

  // ── Mermaid diagram block ──────────────────────────────────────
  if (info === "mermaid") {
    const escaped = token.content.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
    return `<div class="mermaid-block" data-source="${escaped}"></div>`;
  }

  // ── Default: syntax-highlighted static block ───────────────────
  return defaultFence(tokens, idx, options, env, self);
};

// ── Router ─────────────────────────────────────────────────────────
const contentEl = document.getElementById("content");
let currentPath = "";

export function getCurrentPath() { return currentPath; }

async function navigate(path) {
  path = path || "index";

  // Strip leading slash
  if (path.startsWith("/")) path = path.slice(1);

  // Strip .md extension if present
  path = path.replace(/\.md$/, "");

  currentPath = path;

  // Update page title
  const pageName = path.split("/").pop().replace(/-/g, " ");
  document.title = path === "index"
    ? "JsonFusion Documentation"
    : `${pageName} — JsonFusion`;

  // Show loading
  contentEl.innerHTML = `<div class="loading">Loading...</div>`;

  try {
    const resp = await fetch(`./${path}.md`);
    if (!resp.ok) {
      contentEl.innerHTML = `<div class="not-found"><h2>Page not found</h2><p><code>${path}.md</code> does not exist.</p><p><a href="#">Back to home</a></p></div>`;
      return;
    }

    const markdown = await resp.text();

    // Build breadcrumb
    const breadcrumb = buildBreadcrumb(path);

    // Render markdown
    const html = md.render(markdown);
    contentEl.innerHTML = breadcrumb + html;

    // Post-render: live editors
    const liveBlocks = contentEl.querySelectorAll(".live-editor");
    if (liveBlocks.length > 0) {
      const { initLiveEditors } = await import("./live-editor.mjs");
      initLiveEditors(contentEl);
    }

    // Post-render: mermaid
    const mermaidBlocks = contentEl.querySelectorAll(".mermaid-block");
    if (mermaidBlocks.length > 0) {
      await renderMermaidBlocks(mermaidBlocks);
    }

    // Scroll to top
    window.scrollTo(0, 0);

  } catch (err) {
    contentEl.innerHTML = `<div class="not-found"><h2>Error</h2><p>${err.message}</p><p><a href="#">Back to home</a></p></div>`;
  }
}

function buildBreadcrumb(path) {
  if (path === "index") return "";

  const parts = path.split("/");
  let html = `<div class="breadcrumb"><a href="#">Home</a>`;

  // Build intermediate crumbs (section index pages)
  for (let i = 0; i < parts.length - 1; i++) {
    const sectionPath = parts.slice(0, i + 1).join("/");
    const label = parts[i].replace(/-/g, " ");
    html += `<span class="sep">/</span><a href="#/${sectionPath}">${label}</a>`;
  }

  // Current page (not a link)
  const label = parts[parts.length - 1].replace(/^\d+-/, "").replace(/-/g, " ");
  html += `<span class="sep">/</span>${label}`;
  html += `</div>`;

  return html;
}

// ── Link interception ──────────────────────────────────────────────
contentEl.addEventListener("click", (e) => {
  const link = e.target.closest("a");
  if (!link) return;

  const href = link.getAttribute("href");
  if (!href) return;

  // External links: open normally
  if (href.startsWith("http://") || href.startsWith("https://")) return;

  // .html links (like constexpr-tests.html): open normally
  if (href.endsWith(".html")) return;

  // Markdown links: intercept and use hash routing
  if (href.endsWith(".md") || (!href.includes(".") && !href.startsWith("#"))) {
    e.preventDefault();

    // Resolve relative path against current page's directory
    const resolved = resolveLink(href, currentPath);
    location.hash = "#/" + resolved;
  }
});

function resolveLink(href, fromPath) {
  // Strip .md extension
  href = href.replace(/\.md$/, "");

  // Absolute from docs root
  if (href.startsWith("/")) return href.slice(1);

  // Relative: resolve against current page's directory
  const dir = fromPath.includes("/")
    ? fromPath.substring(0, fromPath.lastIndexOf("/") + 1)
    : "";

  // Use URL API for reliable relative resolution
  const base = new URL("http://x/" + dir);
  const resolved = new URL(href, base);
  return resolved.pathname.slice(1); // strip leading /
}

// ── Mermaid (lazy) ─────────────────────────────────────────────────
let mermaidModule = null;

async function renderMermaidBlocks(blocks) {
  if (!mermaidModule) {
    mermaidModule = await import("https://cdn.jsdelivr.net/npm/mermaid@11/dist/mermaid.esm.min.mjs");
    mermaidModule.default.initialize({ startOnLoad: false, theme: "dark" });
  }

  for (const block of blocks) {
    const source = block.dataset.source
      .replace(/&amp;/g, "&").replace(/&lt;/g, "<").replace(/&gt;/g, ">").replace(/&quot;/g, '"');

    const id = "mermaid-" + Math.random().toString(36).slice(2, 10);
    try {
      const { svg } = await mermaidModule.default.render(id, source);
      block.innerHTML = svg;
    } catch (err) {
      block.innerHTML = `<pre style="color:var(--fail)">${err.message}</pre>`;
    }
  }
}

// ── Hash change listener ───────────────────────────────────────────
function onHashChange() {
  const hash = location.hash.slice(1); // remove #
  const path = hash.startsWith("/") ? hash.slice(1) : hash;
  navigate(path || "index");
}

window.addEventListener("hashchange", onHashChange);

// ── Initial load ───────────────────────────────────────────────────
onHashChange();
