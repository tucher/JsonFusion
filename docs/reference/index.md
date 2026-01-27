# API Reference

Exhaustive, lookup-oriented documentation for every public API surface in JsonFusion.

---

## Core Functions

| Page | Content |
|------|---------|
| [Parse & Serialize](api-functions.md) | `Parse()`, `Serialize()`, `ParseWithReader()`, `SerializeWithWriter()` — all overloads, parameters, return types |
| [ParseResult](parse-result.md) | Result type methods, bool conversion, error access, path access, iterator position |

## Type System

| Page | Content |
|------|---------|
| [Type Mapping](../JSON_TYPE_MAPPING.md) | Complete JSON ↔ C++ type mapping table with edge cases |
| [Annotations Reference](../ANNOTATIONS_REFERENCE.md) | All validators (`range`, `min_length`, `enum_values`, ...) and options (`key`, `exclude`, `skip`, `as_array`, ...) |
| [Error Codes](error-codes.md) | Parse error enum, validation error enum, error code descriptions |

## Concepts & Interfaces

| Page | Content |
|------|---------|
| [Reader / Writer Concepts](reader-writer-concepts.md) | `ReaderLike`, `WriterLike` — required methods, type constraints, Frame protocol |
| [Streamer Concepts](streamer-concepts.md) | `ProducingStreamerLike`, `ConsumingStreamerLike` — required methods, `value_type`, lifecycle |
| [Transformer Concepts](transformer-concepts.md) | `ParseTransformerLike`, `SerializeTransformerLike` — required methods, `StoredT` / `WireT` |

## Format Implementations

| Page | Content |
|------|---------|
| [JSON Reader / Writer](formats-json.md) | `JsonIteratorReader`, `JsonIteratorWriter` — construction, iterator requirements |
| [CBOR Reader / Writer](formats-cbor.md) | `CborReader`, `CborWriter` — construction, binary layout notes |
| [yyjson Reader / Writer](formats-yyjson.md) | `YyjsonReader`, `YyjsonWriter` — DOM integration, WireSink behavior |
| [YAML Reader / Writer](formats-yaml.md) | rapidyaml integration — construction, dependencies |

## Transformers & Utilities

| Page | Content |
|------|---------|
| [Transformers Reference](../TRANSFORMERS.md) | `Transformed<>`, `ArrayReduceField`, `MapReduceField`, `VariantOneOf<>` — building blocks and composition |
| [WireSink](wiresink.md) | `WireSink<>` — raw fragment capture, format-specific behavior, round-trip semantics |
| [JSON Schema Generation](../JSON_SCHEMA.md) | `json_schema.hpp` — generation API, supported validators, recursive type handling |
| [Size Estimation](size-estimation.md) | `EstimateMaxSerializedSize<T>()` — algorithm, supported types, FixedMap integration |

## Configuration

| Page | Content |
|------|---------|
| [Compile-Time Options](compile-options.md) | Macros and `#define` knobs: FP backend selection, path tracking mode, etc. |
| [FP Backend Architecture](../FP_BACKEND_ARCHITECTURE.md) | Floating-point backend interface, available backends, swapping guide |

---

[Back to Documentation](../index.md)
