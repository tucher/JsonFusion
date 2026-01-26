const modPath = process.argv[2];
if (!modPath) {
  console.error("Usage: node js_example.mjs ./clang-constexpr-run.mjs");
  process.exit(2);
}

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

const mod = await createModule({
  print: (s) => process.stdout.write(s + "\n"),
  printErr: (s) => process.stderr.write(s + "\n"),
  noInitialRun: true,
});

for (const [name, code] of [["fib(10)", test1], ["array[2]", test2], ["string_view.size()", test3]]) {
  console.log(`\n--- Test: ${name} ---`);
  mod.FS.writeFile("/input.cpp", code);
  const rc = mod.callMain(["--path=/input.cpp", "--std=c++23", "--result=__result"]);
  if (rc !== 0) console.log(`Exit code: ${rc}`);
}
