import fs from "node:fs";
import path from "node:path";
import { loadHeadersFromLocal, ensureDir } from "./load_headers.mjs";

const modPath = process.argv[2];
if (!modPath) {
  console.error("Usage: node run_constexpr_tests.mjs <path-to-clang-constexpr-run.mjs>");
  process.exit(2);
}

// Resolve project root (one level up from this script's directory)
const scriptDir = path.dirname(new URL(import.meta.url).pathname);
const projectRoot = path.resolve(scriptDir, "..");
const testRoot = path.join(projectRoot, "tests", "constexpr");

// Discover all test_*.cpp files
function findTests(dir) {
  const results = [];
  for (const entry of fs.readdirSync(dir, { withFileTypes: true })) {
    const full = path.join(dir, entry.name);
    if (entry.isDirectory()) {
      results.push(...findTests(full));
    } else if (entry.isFile() && entry.name.startsWith("test_") && entry.name.endsWith(".cpp")) {
      results.push(full);
    }
  }
  return results;
}

const allTests = findTests(testRoot).sort();
console.log(`Found ${allTests.length} constexpr test files\n`);

// Skip list: tests that require C++26 / reflection
const skipPatterns = ["annotations/test_cpp26"];

// Load the WASM module
const { default: createModule } = await import(modPath);

let stdout = "";
const mod = await createModule({
  print: (s) => { stdout += s + "\n"; },
  printErr: (s) => process.stderr.write(s + "\n"),
  noInitialRun: true,
});

// Load JsonFusion + PFR headers into the virtual FS
const nHeaders = loadHeadersFromLocal(mod.FS, projectRoot);
console.log(`Loaded ${nHeaders} JsonFusion/PFR headers into virtual FS`);

// Mount test_helpers.hpp into the virtual FS
const helpersPath = path.join(testRoot, "test_helpers.hpp");
const helpersContent = fs.readFileSync(helpersPath, "utf-8");

ensureDir(mod.FS, "/tests/constexpr");
mod.FS.writeFile("/tests/constexpr/test_helpers.hpp", helpersContent);

// Write all test files into the virtual FS (preserving directory structure)
const testPaths = []; // virtual paths
for (const testFile of allTests) {
  const rel = path.relative(testRoot, testFile); // e.g. "basic/test_parse_int.cpp"
  const vpath = "/tests/constexpr/" + rel;
  const dir = path.dirname(vpath);
  ensureDir(mod.FS, dir);
  mod.FS.writeFile(vpath, fs.readFileSync(testFile, "utf-8"));
  testPaths.push({ native: testFile, vpath, rel });
}

// Run tests
let pass = 0;
let fail = 0;
let skip = 0;
const failures = [];

for (const { native, vpath, rel } of testPaths) {
  // Check skip list
  if (skipPatterns.some(pat => rel.includes(pat))) {
    const category = path.dirname(rel);
    const name = path.basename(rel, ".cpp");
    console.log(`  SKIP  ${category.padEnd(20)} ${name}`);
    skip++;
    continue;
  }

  const category = path.dirname(rel);
  const name = path.basename(rel, ".cpp");

  stdout = "";
  const rc = mod.callMain([
    "--check-only",
    "--std=c++23",
    "--include-dir=/tests/constexpr",
    `--path=${vpath}`,
  ]);

  if (rc === 0) {
    console.log(`  PASS  ${category.padEnd(20)} ${name}`);
    pass++;
  } else {
    console.log(`  FAIL  ${category.padEnd(20)} ${name}`);
    fail++;
    // Parse diagnostics from JSON output
    let diag = "";
    try {
      const json = JSON.parse(stdout.trim());
      diag = json.diagnostics || "";
    } catch {
      diag = stdout;
    }
    failures.push({ category, name, diag });
  }
}

// Summary
console.log("\n=======================================");
console.log(`Results: ${pass} passed, ${fail} failed, ${skip} skipped`);
if (fail === 0) {
  console.log("All tests passed!");
} else {
  console.log(`\n--- Failures ---`);
  for (const { category, name, diag } of failures) {
    console.log(`\n${category}/${name}:`);
    // Show first few lines of diagnostics
    const lines = diag.split("\n").slice(0, 10);
    console.log(lines.join("\n"));
    if (diag.split("\n").length > 10) console.log("  ...(truncated)");
  }
  process.exit(1);
}
