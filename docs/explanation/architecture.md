# Architecture

JsonFusion is a layered, zero-cost abstraction library. No virtual functions — everything
is resolved at compile time via C++20 concepts and templates.

## Layered Design

```mermaid
graph TD
    subgraph User["User Code"]
        US["C++ structs / classes"]
    end

    subgraph Core["Core Layer"]
        P["parser.hpp<br/>Parse(), ParseWithReader()"]
        S["serializer.hpp<br/>Serialize(), SerializeWithWriter()"]
    end

    subgraph Transformers["Transformer Layer (optional)"]
        T1["Transformed&lt;Stored, Wire, From, To&gt;"]
        T2["VariantOneOf&lt;Ts...&gt;"]
    end

    subgraph Concepts["Reader / Writer Concepts"]
        RC["ReaderLike concept<br/>reader_concept.hpp"]
        WC["WriterLike concept<br/>writer_concept.hpp"]
    end

    subgraph Formats["Format Implementations"]
        JSON["json.hpp<br/>JsonIteratorReader<br/>JsonIteratorWriter"]
        CBOR["cbor.hpp<br/>CborReader<br/>CborWriter"]
        YYJSON["yyjson.hpp<br/>YyjsonReader<br/>YyjsonWriter"]
        YAML["yaml.hpp<br/>RapidYaml reader/writer"]
    end

    US --> P
    US --> S
    P --> Transformers
    S --> Transformers
    Transformers --> RC
    Transformers --> WC
    P --> RC
    S --> WC
    RC --> JSON
    RC --> CBOR
    RC --> YYJSON
    RC --> YAML
    WC --> JSON
    WC --> CBOR
    WC --> YYJSON
    WC --> YAML

    style User fill:#1a1a2e,stroke:#00b4d8,color:#e0e0e0
    style Core fill:#16213e,stroke:#00b4d8,color:#e0e0e0
    style Transformers fill:#16213e,stroke:#ff9800,color:#e0e0e0
    style Concepts fill:#16213e,stroke:#4caf50,color:#e0e0e0
    style Formats fill:#0f3460,stroke:#4caf50,color:#e0e0e0
```

## How the layers work

**Core** (`parser.hpp`, `serializer.hpp`) contains the main `Parse()` and `Serialize()` functions.
They walk a C++ struct's fields at compile time using reflection, and delegate each value
to a **Reader** (for parsing) or **Writer** (for serialization).

**Reader/Writer concepts** (`reader_concept.hpp`, `writer_concept.hpp`) define the interface
that any format must implement — primitives, strings, arrays, maps, chunked I/O.
No base class — just C++20 concepts enforced at compile time.

**Format implementations** are concrete Reader/Writer pairs:

| Format | Header | Reader | Writer | Notes |
|--------|--------|--------|--------|-------|
| JSON | `json.hpp` | `JsonIteratorReader` | `JsonIteratorWriter` | Streaming, RFC 8259 |
| CBOR | `cbor.hpp` | `CborReader` | `CborWriter` | Binary, RFC 8949 |
| yyjson | `yyjson.hpp` | `YyjsonReader` | `YyjsonWriter` | DOM-based, optional fast backend |
| YAML | `yaml.hpp` | via rapidyaml | via rapidyaml | Uses rapidyaml library |

**Transformers** sit between the core and the wire format. They convert between
what the user stores (`StoredT`) and what appears on the wire (`WireT`):

```mermaid
graph LR
    Wire["Wire format<br/>(JSON string, CBOR bytes)"] -->|"Reader"| WireT["WireT<br/>(intermediate type)"]
    WireT -->|"transform_from()"| Stored["StoredT<br/>(user's type)"]
    Stored -->|"transform_to()"| WireT2["WireT"]
    WireT2 -->|"Writer"| Wire2["Wire format"]

    style Wire fill:#0f3460,stroke:#4caf50,color:#e0e0e0
    style Wire2 fill:#0f3460,stroke:#4caf50,color:#e0e0e0
    style WireT fill:#16213e,stroke:#ff9800,color:#e0e0e0
    style WireT2 fill:#16213e,stroke:#ff9800,color:#e0e0e0
    style Stored fill:#1a1a2e,stroke:#00b4d8,color:#e0e0e0
```

Examples of transformers:
- `Transformed<std::chrono::time_point, std::string, parseISO, toISO>` — dates as ISO strings
- `VariantOneOf<int, std::string, Config>` — discriminated union, tries each alternative

## Data flow: parsing

```mermaid
sequenceDiagram
    participant App as Application
    participant Core as Parser Core
    participant Reader as JsonIteratorReader
    participant Data as JSON bytes

    App->>Core: Parse(myStruct, jsonString)
    Core->>Reader: start_value_and_try_read_null()
    Reader->>Data: peek next token

    loop For each struct field
        Core->>Reader: read_number() / read_string_chunk() / ...
        Reader->>Data: consume tokens
        Reader-->>Core: value or error
        Core->>Core: validate, assign to field
    end

    Core-->>App: ParseResult (ok / error + path)
```

## Key design decisions

- **No virtual dispatch**: Format selection is a template parameter, resolved at compile time
- **Streaming**: Readers/Writers process data incrementally — no need to load entire input
- **Chunked strings**: `read_string_chunk()` / `write_string_chunk()` enable fixed-buffer operation
- **Frame-based nesting**: Each array/map gets a `Frame` object tracking iteration state
- **WireSink**: Captures raw format bytes for passthrough between formats without re-parsing

## File map

```
include/JsonFusion/
├── Core
│   ├── parser.hpp              Parse(), DeserializationContext
│   ├── serializer.hpp          Serialize(), SerializationContext
│   ├── static_schema.hpp       Type concepts, cursor abstractions
│   └── options.hpp             Compile-time annotations
│
├── Reader/Writer
│   ├── reader_concept.hpp      ReaderLike concept definition
│   └── writer_concept.hpp      WriterLike concept definition
│
├── Formats
│   ├── json.hpp                JSON reader/writer (streaming)
│   ├── cbor.hpp                CBOR reader/writer (binary)
│   ├── yyjson.hpp              yyjson DOM reader/writer
│   └── yaml.hpp                YAML via rapidyaml
│
├── Transformers
│   ├── generic_transformers.hpp    Transformed<>, ArrayReduceField
│   └── variant_transformer.hpp    VariantOneOf<>
│
└── Support
    ├── wire_sink.hpp           Raw format capture/passthrough
    ├── validators.hpp          Schema constraint checking
    ├── errors.hpp              Error enumerations
    ├── parse_result.hpp        Result with error path tracking
    ├── annotated.hpp           Field annotation support
    ├── struct_introspection.hpp    Compile-time reflection
    └── json_schema.hpp         JSON Schema generation
```

[Back to Documentation](../index.md)
