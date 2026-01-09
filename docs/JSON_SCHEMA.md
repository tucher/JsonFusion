# Three Layers of JSON Data

*…and why JsonFusion doesn't have a native `std::variant` mode*

---

There are 3 layers of thinking between the moment JSON message bytes come into a parser and the moment application logic makes decisions based on them.

## Layer 1: JSON itself (the value model)

JSON only has **six kinds of values**:
- `null`
- `bool`
- `number`
- `string`
- `array`
- `object`

That's it. No `oneOf`, no date, no "discriminated union", no "RPC". Just those six.

**JsonFusion's core is about this layer only:**

> "Given a C++ type that looks like JSON, parse/serialize it efficiently."

"Looks like JSON" here means:
- **Object** → `struct` with fields / `std::map`-like / map streamers
- **Array** → `std::vector<T>`, `std::array<T, N>`, C arrays, array streamers, …
- **String** → `std::string`, `std::string_view`, `char[N]`, etc.
- **Number** → integral / floating scalar types
- **Bool** → `bool`
- **null** → `std::optional<T>`, `std::unique_ptr<T>`

Static schema concepts (`ObjectLike`, `JsonArray`, `ParsableStringLike`, `SerializableStringLike`, …) are all about answering:

> If I ignore annotations, options, validators, etc., does this C++ type represent one of the six JSON value categories?

That's the **wire type**: the shape of the JSON itself.

---

## Layer 2: Constraints on JSON (validators)

Once you have a pure JSON-shaped type, you usually want **constraints**:
- `range<0, 100>` on a number
- `min_length<3>` on a string
- `min_items<1>` on an array
- custom predicates via `fn_validator<...>`

These are still entirely about layer 1. They don't change what the JSON is, they just filter which values are allowed.

**Conceptually:**

```cpp
struct Config {
    A<int, range<0, 100>> level;
    A<std::string, min_length<3>> name;
};
```

This is still just:

```json
{
  "level": 42,
  "name": "foo"
}
```

**Validators are:**
- Attached to the wire type (`int`, `std::string`, `std::array<…>`, …)
- Triggered on parse events (number parsed, array finished, object finished, …)
- Never introduce new JSON categories

In other words, validators say:

> "From the universe of possible JSON values with this shape, I only accept this subset."

They don't try to introduce "schema algebra" like `oneOf`, `not`, etc. That stuff belongs to a different layer (we'll get there).

---

## Layer 3: Domain model (your actual types)

Now we leave JSON land and move into your app's world:
- You want `MyDate` instead of `"2025-01-01"`
- You want a reducer over an array instead of storing all elements
- You want some kind of `oneOf`/sum type
- You might even want no storage at all, just "parse and call a callback", RPC style

This is **layer 3**: domain model & behavior.

JsonFusion core doesn't know anything about `MyDate`, or reducers, or sum types. Instead, it gives you **three generic mechanisms** to bridge layer 1 (wire JSON) to layer 3 (domain):

1. **Transformers** – value ↔ value
2. **Streamers** – JSON array/map → callbacks instead of storage
3. **`WireSink` object** – "capture this subtree as raw JSON/CBOR" so you can interpret it later

Let's look at each.

---

## Transformers: wire JSON ↔ domain type

A transformer is a small object that says:

> "Treat me as JSON of type `wire_type`, but when parsing/serializing, run some custom code."

**Minimal core concept:**

```cpp
namespace static_schema {

template<class T>
concept ParseTransformerLike = requires(T t, const typename T::wire_type& w) {
    typename T::wire_type;         // the JSON-facing type
    { t.transform_from(w) } -> std::convertible_to<bool>;
};

template<class T>
concept SerializeTransformerLike = requires(const T t, typename T::wire_type& w) {
    typename T::wire_type;
    { t.transform_to(w) } -> std::convertible_to<bool>;
};

} // namespace static_schema
```

The parser logic becomes roughly:

```cpp
if constexpr (ParseTransformerLike<FieldT>) {
    typename FieldT::wire_type tmp;
    // Parse JSON into tmp as if tmp were the actual field
    if (!Parse(tmp, reader, ctx)) { ... }
    // Then hand tmp to the transformer
    if (!field.transform_from(tmp)) { ... }
}
```

**Example: string → MyDate**

```cpp
struct MyDate {
    int year, month, day;

    static bool from_string(const std::string& s, MyDate& out);
    static bool to_string(const MyDate& d, std::string& out);
};

template<class DateT>
struct DateTransformer {
    using wire_type = std::string;
    DateT value;

    bool transform_from(const std::string& s) {
        return DateT::from_string(s, value);
    }

    bool transform_to(std::string& s) const {
        return DateT::to_string(value, s);
    }
};

struct Event {
    // In JSON: "date": "2024-01-01"
    // In C++: Event::date.value is MyDate
    A<DateTransformer<MyDate>, key<"date">> date;
};
```

From the parser's point of view:
- It sees a `wire_type` of `std::string`
- It parses a JSON string into a temporary `std::string`
- It calls `transform_from` to populate your `MyDate`

The core doesn't know or care what `MyDate` is. It just knows how to drive `wire_type`.

---

## Streamers: array/map → "do something"

Streamers say:

> "Treat me like an array/map in JSON, but instead of storing elements, call `consume()` for each one."

**Basic example used in benchmarks:**

```cpp
template <class ValueT>
struct CountingStreamer {
    using value_type = ValueT;

    std::size_t counter = 0;

    void reset()  { counter = 0; }

    bool consume(const ValueT&) {
        ++counter;
        return true;
    }

    bool finalize(bool success) { return success; }
};
```

**More ergonomic variant with a context-bound lambda:**

```cpp
#include <JsonFusion/generic_streamer.hpp>

struct Stats {
    std::size_t totalPoints = 0;
};

struct Point { double x, y, z; };

using PointsArrayConsumer = streamers::LambdaStreamer<
    [](Stats* stats, const Point&) {
        stats->totalPoints++;
        return true;
    }
>;
```

To JsonFusion core, this is just "an array of `Point`", but instead of `std::vector<Point>` it has a streamer type that gets fed element by element.

Again: the core stays in layer 1; the semantics "count points", "accumulate sum", "send over UART" are all layer 3.

---

## `WireSink`: capturing raw JSON for later

Sometimes you don't want to interpret a subtree immediately at all. You just want:

> "Give me this part of JSON as a buffer/string/DOM, I'll decide later."

That's where `WireSink` comes in:
- JsonFusion walks the JSON as usual
- For the `WireSink` field, it re-emits the raw JSON into your sink
- You end up with a "sub-document" or chunk of JSON you can feed into:
  - another parser,
  - a schema engine (`oneOf`/`anyOf`/custom logic),
  - a domain-specific interpreter

This is the natural place to implement schema algebra like "try shape A, or B, or C" without making the core understand `oneOf`.

---

## So where does `std::variant` fit?

Here's the key realization:

> **JSON itself has no variant.**

`variant` is a **schema / domain concept**, not a JSON concept.

There are basically two things people mean when they say "variant":

1. **Schema-level "oneOf / anyOf"**  
   "The JSON at this location may be shape A or shape B or shape C."

2. **Domain-level sum type**  
   "In C++ I want a `std::variant<A,B,C>` because that's how my code thinks."

**Neither of those needs the core parser to know about `std::variant`.**

You can build them in layer 3:
- Use `WireSink` to capture the subtree
- Try multiple `Parse(...)` calls into different candidate wire types
- Wrap the chosen one into a `std::variant`
- Glue it to JsonFusion via a transformer

So instead of:

```cpp
std::variant<A, B, C> x; // baked into core
```

You write:

```cpp
struct OneOf_ABC {
    using wire_type = WireSink<1024>; 

    std::variant<A,B,C> value;

    bool transform_from(const auto& parseFn) {
        // Try parsing as A, then B, then C - first success wins
        if (parseFn(value.emplace<A>())) return true;
        if (parseFn(value.emplace<B>())) return true;
        if (parseFn(value.emplace<C>())) return true;
        return false; // None matched
    }

    bool transform_to(const auto& serializeFn) const {
        // Serialize whichever variant alternative is active
        return std::visit([&](const auto& v) {
            return serializeFn(v);
        }, value);
    }
};
```

This is precisely the kind of thing that belongs in "extras", not in the core:
- It's schema/domain logic ("pick one of multiple shapes")
- It's built entirely from:
  - core parser (`Parse`)
  - `WireSink`
  - transformers (which has special support for WIreSink: automatically configured proper Reader/Writer is created and callable object passed to user code)

The core stays beautifully ignorant.

---

## Putting it all together

The three layers look like this:

### 1. Layer 1 – JSON value model (core)
- Whatever the JSON standard defines: null/bool/number/string/array/object
- JsonFusion's `static_schema` concepts map C++ types onto those
- Parser + Reader operate here, nothing else

### 2. Layer 2 – Constraints / validators
- Restrict which JSON values are allowed, but don't change the type
- Still purely about layer 1, just "a valid subset"

### 3. Layer 3 – Domain model / semantics
- Your `MyDate`, `std::variant`, reducers, RPC, business logic
- Implemented via:
  - **Transformers** (wire JSON ↔ domain type)
  - **Streamers** (arrays/maps → callbacks with context)
  - **`WireSink`** (capture raw JSON to handle later or elsewhere)
- All arbitrarily composable

**JsonFusion core should never embed domain concepts into layer 1.**

It models pure JSON, and opens doors (Transformers/Streamers/`WireSink`) for everything else.

That's why you can now:
- Express arbitrary JSON shapes directly as C++ types (implicitly attaching some constraints, like the lack of abstract "number" in C++)
- Add constraints with validators
- Build all higher-level behavior (dates, reducers, oneOf-like constructs, RPC) using generic hooks, without ever polluting the core with "special cases"

---

## "Why not just add `std::variant` as `oneOf` in the core?"

You can imagine a very reasonable pushback:

> "Okay, okay, full schema algebra is complex, but… what if JsonFusion just treated `std::variant<A,B,C>` as `oneOf [A,B,C]`? Most people only need that, right?"

Tempting, but it breaks the nice 3-layer separation and creates more problems than it solves. Concretely:

### 1. You still need N full parsing passes in the generic case

Given JSON like:

```json
{ "value": ... }
```

and a C++ type:

```cpp
std::variant<A, B, C> value;
```

a generic "core-level" implementation can't know which arm will match without trying them:

1. Try parse as A
2. If that fails (or validation fails), try B
3. If that fails, try C
4. If all fail → error

That is **N independent attempts** over the same subtree, where N is the number of arms.

But now you've forced that strategy into the core:
- Even if the user knows "99% of the time it's B", the core can't specialize that
- Even if the user wants a totally different resolution strategy ("look at a type field first", "dispatch on shape"), the core's variant logic is in the way

If you do this in layer 3 instead (via transformer + `WireSink`), you can implement whichever strategy you want:
- First try B, then C, then A
- Or inspect a discriminator and only parse the matching type
- Or do shape-based dispatch

The core stays simple and predictable.

---

### 2. What do you do with intermediate buffers?

If the core owns `std::variant<A,B,C>` handling, it has to answer:

> "Where do I keep the JSON while I'm trying multiple arms?"

Options, none of them nice:

- **Re-parse from the original input stream N times**  
  → Terrible for streaming readers, and Reader API is deliberately forward-only

- **Copy the JSON subtree into some intermediate structure** (buffer, small DOM, etc.)  
  → Now you've baked buffering policy into the core:
  - How big?
  - Heap or stack?
  - What about embedded targets?
  - Who pays for this when they don't use variants?

But if you move this to layer 3:
- You can say: "for this particular oneOf-style transformer, I'll use a `WireSink` string buffer", or a small DOM, or whatever fits your constraints
- Different call sites can choose different strategies
- Core never needs to know about buffering policy at all

---

### 3. Schema algebra is non-trivial, and you can't stop at `oneOf`

The slippery slope looks like this:

1. **Start with:**

   ```cpp
   std::variant<A,B,C>
   ```
   
   ↔ "JSON matches one of these shapes."

2. **Then people want:**
   - `anyOf` (like `oneOf` but multiple shapes may be valid)
   - `allOf` (intersection of schemas)
   - `not` (negative constraints)
   - complex nested combinations

3. **Now you're halfway to a JSON Schema engine** – but:
   - half of the logic is in the parser core,
   - the other half ends up bolted on externally,
   - semantics get scattered and hard to reason about

And still:
- You'll want custom resolution policies
- You'll want better error reporting ("it failed arm B because field X missing", etc.)
- You'll want per-arm validation and custom messages

All of that clearly belongs in layer 3: a separate "schema algebra & resolution" layer that uses JsonFusion as a fast building block — not in the tight inner parsing loop, which is now scalable from tiny 15KB binary code on µC to high-performance ~1Gb/s-level parser.

---

## The clean alternative

Instead of putting `std::variant` into the core, you get:

- **Pure JSON core (layer 1)** — only the six JSON types
- **Validators (layer 2)** — filter allowed values, still purely JSON
- **Transformers / Streamers / `WireSink` (layer 3 hooks)** — everything domain-specific:
  - dates,
  - reducers,
  - RPC,
  - and yes, your own `oneOf` / `std::variant` handling

So if someone really wants variant + `oneOf`, they can build:

```cpp
struct VariantOneOf {
    using wire_type = WireSink<1024>;

    std::variant<A,B,C> value;

    bool transform_from(const auto& parseFn) {
        // custom resolution logic here
    }

    bool transform_to(const auto& serializeFn) const {
        // serialize active arm here
    }
};
```

No core pollution, no hardcoded buffering policy, no half-baked schema algebra glued into the parser.

---

## Summary

**That's the whole story in one line:**

> **JsonFusion models JSON, not your schema.**  
> Everything "schema-ish" – including `std::variant`/`oneOf` – belongs on top, built from transformers, streamers and sinks, not baked into the engine.

---

## P.S.

"But wait," you might say, "doesn't this make it really hard to use variants in practice?"

Not at all. In fact, we ship `JsonFusion::VariantOneOf` ready to use in [`include/JsonFusion/variant_transformer.hpp`](../include/JsonFusion/variant_transformer.hpp).

**The difference?** It's implemented as a **layer 3 transformer** using the exact building blocks described above—not as special-case logic in the parser core. Open that file and you'll see: it's ~50 lines proving that the architecture works. The core stays pure, and you still get convenient `std::variant` support. Best of both worlds.
