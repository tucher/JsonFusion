# Validation & Annotations

JsonFusion lets you attach constraints and metadata directly to your types using `Annotated<T, Options...>` (alias: `A<T, Options...>`).
This tutorial introduces the annotation system step by step — from a single constraint to composing multiple options on a single field.

## TODO: Sections to write

### The Annotated<> Wrapper
- What `A<T, opts...>` is: a thin, implicit-converting wrapper
- How it behaves like `T` in normal code (operator forwarding, implicit conversion)
- Interactive: define a struct with `A<int, range<1, 100>>`, parse valid and invalid input

### Validators
- `range<Min, Max>` — numeric bounds
- `min_length<N>`, `max_length<N>` — string length
- `min_items<N>`, `max_items<N>` — collection size
- `enum_values<V1, V2, ...>` — whitelist of allowed values
- `constant<V>`, `string_constant<"...">` — fixed values
- Interactive: build a struct with multiple validators, show what fails and why

### Options (Metadata / Behavior)
- `key<"name">` — rename JSON key
- `exclude` — skip field entirely
- `skip` — validate structure but discard value
- `as_array` — serialize struct as `[a, b, c]` tuple
- `allow_excess_fields` — tolerate unknown keys
- Interactive: demonstrate key renaming, exclusion, as_array

### Composing Multiple Annotations
- Multiple options on a single field: `A<string, key<"user_name">, min_length<1>, max_length<100>>`
- Nesting: `A<vector<A<Point, as_array>>, max_items<1000>>`
- Interactive: a GPS track model with composed annotations

### Custom Validators
- `fn_validator<Event, Lambda>` — arbitrary validation logic
- Validation events and their signatures
- Building reusable validator templates
- Interactive: divisibility validator, email-like pattern check

### External Annotations
- `Annotated<T>` — type-level (for generic types)
- `AnnotatedField<T, Index>` — field-level (for PFR-introspectable structs)
- `StructMeta<T>` — full manual control (required for C arrays)
- When to use which level
- Interactive: annotate a third-party type without modifying it

## What's Next
- [Error Handling](04-error-handling.md)
- [How-to Guides](../how-to/index.md) for specific recipes

[Back to Documentation](../index.md)
