# Modeling JSON with C++ Types

Your C++ types are the schema. This tutorial walks through each of the six JSON types
and shows which C++ types map to them — with interactive examples you can edit and compile.

## TODO: Sections to write

### Numbers
- `int`, `unsigned`, `int64_t`, `float`, `double`
- Integer overflow behavior
- Interactive: parse a struct with mixed numeric types

### Strings
- `std::string` (dynamic)
- `std::array<char, N>` (fixed-size, null-terminated)
- `std::vector<char>` (dynamic bytes)
- Interactive: parse into each string type, show serialization differences

### Booleans
- `bool`
- Interactive: parse true/false, show type mismatch error on `"true"` (string vs bool)

### Null
- `std::optional<T>` — field present-or-null
- `std::unique_ptr<T>` — nullable with heap allocation
- Interactive: parse with null fields, show optional access patterns

### Arrays
- `std::vector<T>` — dynamic length
- `std::array<T, N>` — fixed length
- `std::list<T>`, `std::deque<T>` — other containers
- Nested arrays: `std::vector<std::vector<int>>`
- Interactive: parse nested arrays, show round-trip

### Objects
- Structs — named fields, static schema
- `std::map<std::string, T>` — dynamic keys
- Nested structs
- Interactive: struct vs map, when to use which

## Composing Types
- Struct containing vectors of optional structs
- `std::vector<std::optional<T>>`
- `std::map<std::string, std::vector<T>>`
- Interactive: a realistic nested model (e.g., API response with users, addresses, tags)

## What's Next
- [Validation & Annotations](03-validation-and-annotations.md)
- [Error Handling](04-error-handling.md)

[Back to Documentation](../index.md)
