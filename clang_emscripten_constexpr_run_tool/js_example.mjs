import path from "node:path";
import { loadHeadersFromLocal } from "./load_headers.mjs";

const modPath = process.argv[2];
if (!modPath) {
  console.error("Usage: node js_example.mjs ./clang-constexpr-run.mjs");
  process.exit(2);
}

const scriptDir = path.dirname(new URL(import.meta.url).pathname);
const projectRoot = path.resolve(scriptDir, "..");

const { default: createModule } = await import(modPath);

// Test 1: bare constexpr (no stdlib)
const test1 = `
constexpr int fib(int n){ return n<2? n : fib(n-1)+fib(n-2); }
constexpr auto __result = fib(10);
`;

// Test 2: using <array> from libc++
const test2 = `
#include <array>
constexpr auto __result = std::array<int,5>{10,20,30,40,50}[2];
`;

// Test 3: using <string_view>
const test3 = `
#include <string_view>
constexpr auto __result = std::string_view("hello world").size();
`;

const test4 = `
static_assert(false, "intentional failure");
constexpr auto __result = 0;
`

const test5 = `
#include <string_view>
#include <string>
#include <JsonFusion/parser.hpp>
#include <JsonFusion/serializer.hpp>

constexpr auto __result = []() consteval {
    using namespace JsonFusion;
    constexpr auto json = std::string_view(R"({"key": "value", "number": 42})");
    struct Model {
        std::string key{};
        int number{};
    };
    Model model;
    auto result = Parse(model, json);
    
    std::string serialized;
    auto sRes = Serialize(model, serialized);
    return (
        !!result && model.key == "value" && model.number == 42 
        && 
        !!sRes && serialized == R"({"key":"value","number":42})"
        ) ? 0: 1;
}();
`;

const mod = await createModule({
  print: (s) => process.stdout.write(s + "\n"),
  printErr: (s) => process.stderr.write(s + "\n"),
  noInitialRun: true,
});

// Load JsonFusion + PFR headers into the virtual FS
const nHeaders = loadHeadersFromLocal(mod.FS, projectRoot);
console.log(`Loaded ${nHeaders} JsonFusion/PFR headers into virtual FS`);

for (const [name, code] of [
        ["fib(10)", test1],
        ["array[2]", test2],
        ["string_view.size()", test3],
        ["static_assert failure", test4],
        ["JsonFusion parse/serialize", test5],
    ]) {
  console.log(`\n--- Test: ${name} ---`);
  mod.FS.writeFile("/input.cpp", code);
  const rc = mod.callMain(["--path=/input.cpp", "--std=c++23", "--result=__result"]);
  if (rc !== 0) console.log(`Exit code: ${rc}`);
}
