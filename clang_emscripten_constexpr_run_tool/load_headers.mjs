import fs from "node:fs";
import path from "node:path";

/**
 * Ensure a directory path exists in Emscripten's virtual FS.
 */
export function ensureDir(fsObj, dirPath) {
  const parts = dirPath.split("/").filter(Boolean);
  let current = "";
  for (const part of parts) {
    current += "/" + part;
    try {
      fsObj.stat(current);
    } catch {
      fsObj.mkdir(current);
    }
  }
}

/**
 * Load JsonFusion + PFR headers from the local filesystem into Emscripten FS.
 * Uses headers-manifest.json for the file list.
 *
 * @param {object} fsObj - Emscripten FS object (mod.FS)
 * @param {string} projectRoot - Absolute path to the JsonFusion project root
 * @param {string} sysrootPrefix - Where to mount in the virtual FS (default "/sysroot/include")
 */
export function loadHeadersFromLocal(fsObj, projectRoot, sysrootPrefix = "/sysroot/include") {
  const manifestPath = path.join(
    path.dirname(new URL(import.meta.url).pathname),
    "headers-manifest.json"
  );
  const manifest = JSON.parse(fs.readFileSync(manifestPath, "utf-8"));

  let count = 0;
  for (const relPath of manifest) {
    // relPath is like "include/JsonFusion/parser.hpp"
    // Strip the leading "include/" to get the FS-relative path
    const stripped = relPath.replace(/^include\//, "");
    const vpath = sysrootPrefix + "/" + stripped;
    const nativePath = path.join(projectRoot, relPath);

    ensureDir(fsObj, path.dirname(vpath));
    fsObj.writeFile(vpath, fs.readFileSync(nativePath, "utf-8"));
    count++;
  }
  return count;
}

/**
 * Load JsonFusion + PFR headers from GitHub raw URLs into Emscripten FS.
 * For browser use.
 *
 * @param {object} fsObj - Emscripten FS object
 * @param {string} manifestUrl - URL to headers-manifest.json
 * @param {string} rawBaseUrl - Base URL for raw file content
 *        (e.g. "https://raw.githubusercontent.com/user/JsonFusion/master")
 * @param {string} sysrootPrefix - Where to mount in the virtual FS
 */
export async function loadHeadersFromGitHub(fsObj, manifestUrl, rawBaseUrl, sysrootPrefix = "/sysroot/include") {
  const manifest = await fetch(manifestUrl).then(r => r.json());

  const fetches = manifest.map(async (relPath) => {
    const stripped = relPath.replace(/^include\//, "");
    const vpath = sysrootPrefix + "/" + stripped;
    const url = rawBaseUrl + "/" + relPath;

    const content = await fetch(url).then(r => r.text());
    ensureDir(fsObj, vpath.substring(0, vpath.lastIndexOf("/")));
    fsObj.writeFile(vpath, content);
  });

  await Promise.all(fetches);
  return manifest.length;
}
