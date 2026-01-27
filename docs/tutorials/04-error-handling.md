# Error Handling

When parsing fails, you need to know *what* went wrong and *where*.
This tutorial covers JsonFusion's error reporting system — from basic bool checks
to extracting JSON paths, error codes, and constraint details.

## TODO: Sections to write

### ParseResult Basics
- `Parse()` returns a `ParseResult`, not a raw bool
- Bool conversion: `if (!result) { ... }`
- Accessing the error code: `result.error()`
- Interactive: parse invalid JSON, check the result

### Error Categories
- Parse errors — malformed JSON (missing braces, bad tokens)
- Type mismatch errors — JSON string where number expected, etc.
- Validation errors — value parsed correctly but constraint failed
- Interactive: trigger each category, show how they differ

### JSON Path Tracking
- `result.path()` — e.g., `$.users[3].address.street`
- Compile-time sized path storage (based on schema depth)
- How nesting depth affects path buffer
- Interactive: deep struct, trigger error at leaf, inspect path

### Formatting Diagnostics
- Converting error + path to human-readable string
- Iterator position for locating the error in input
- Combining error code + path + position for logging
- Interactive: full diagnostic output example

### Validation Error Details
- Which constraint failed and with what value
- Validator error codes
- Interactive: range violation, show constraint details

### Constexpr Error Handling
- Errors work in constexpr context too
- `static_assert` with error inspection
- Interactive: compile-time error detection and reporting

## What's Next
- [How-to Guides](../how-to/index.md) for specific recipes
- [Advanced Error Handling](../how-to/advanced-error-handling.md) how-to guide

[Back to Documentation](../index.md)
