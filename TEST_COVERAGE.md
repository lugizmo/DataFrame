# Test Coverage

## Conventions

### File layout & naming

Correctness tests live in `lib/test/` and follow:

```
RM_Dataframe<Area>Test.cpp
```

- `RM_` = row-major layout. Drop the prefix for tests that are not layout-specific
  (e.g. index, container, memory tests). A future column-major suite would use `CM_`.
- `<Area>` groups by functional area (`Tors`, `Construct`, `Adding`, `Drop`,
  `Properties`, `Access`, `Mutating`, `Views`, `Sort`, `Functional`, `Io`,
  `UserTypes`).

Usability / example tests live in `lib/test/examples/` and follow:

```
RM_DFExamples_<Topic>.cpp
```

Examples read like real usage and still `ASSERT`, but they check *usability* (does the
API compose naturally) rather than exhaustively probing correctness.

### Test style

- **One `TEST` per function.** Different cases for the same function are separate
  scoped blocks `{ ... }`, each opened with a short `// comment`.
- See `RM_DataframeAccessTest.cpp` for the reference style.

### Index coverage

Every *index-agnostic* function is exercised across index configurations
(field × record). Legend:

- **UU** — `DFUniqueIndex` × `DFUniqueIndex`
- **R1** — `DFRangeIndex` × `DFRangeIndex`, `step == 1`
- **Rs** — `DFRangeIndex` × `DFRangeIndex`, `step != 1`
- **Mix** — mixed (e.g. `DFRangeIndex` × `DFUniqueIndex`), where relevant

Each such function carries a matrix, e.g. `UU:[ ] R1:[ ] Rs:[ ]`.

Index-*specific* functions (e.g. `SetFieldRange`, `AddField`, `UpsertValue`) belong to a
single index family and are tracked without a matrix.

### Managing cross-index tests

The index type is a compile-time `DataFrame` template parameter, so cross-index coverage
is driven by **GoogleTest typed tests** (`TYPED_TEST_SUITE` + `TYPED_TEST`):

- A shared `lib/test/RM_DataframeTestConfigs.h` defines config-traits types —
  `Unique_IndexTest` (UU), `RangeS1_IndexTest` (R1), `RangeS2_IndexTest` (Rs), and a mixed
  config where relevant — each exposing its `DataFrame` type plus helpers to **build a
  populated frame and hand back valid / invalid keys**. (The traits hide the difference
  between `AddFields(...)` for unique and `SetFieldRange(...)` for range; `step != 1` is a
  runtime bound carried by `RangeS2_IndexTest`, whose `MissingRecord()` returns an off-grid
  key.)
- See `RM_DataframeAccessTest.cpp` for the reference typed-test.
- Each index-agnostic behaviour is written **once** as a `TYPED_TEST` and runs against
  every config in the type list automatically.

---

## Dataframe

### Tor's

Test file: [RM_DataframeTorsTest.cpp](/lib/test/RM_DataframeTorsTest.cpp)

| Done | Function Name                      | Returns     | UU | R1 | Rs |
|------|------------------------------------|-------------|----|----|----|
| ✅    | DataFrame(DataFrame const&)        |             | ✔  | ✔  | ✔  |
| ✅    | operator=(DataFrame const&)        | DataFrame&  | ✔  | ✔  | ✔  |
| ✅    | DataFrame(DataFrame&& other)       |             | ✔  | ✔  | ✔  |
| ✅    | operator=(DataFrame&& other)       | DataFrame&  | ✔  | ✔  | ✔  |
| ✅    | ~DataFrame()                       |             | ✔  | ✔  | ✔  |

> Lifecycle is typed across all configs; the free path (`~DataFrame`, move-assign) is additionally
> asserted with a counting memory resource in `RM_DataframeTorsMemory`.

### Construction

Test file: [RM_DataframeConstructTest.cpp](/lib/test/RM_DataframeConstructTest.cpp)

- [ ] ```DataFrame(MemRsc res)```
- [ ] ```DataFrame(size_t reservedValues, MemRsc res)```
- [ ] ```FromFields(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<FldT const>> fields, size_t reservedValues, MemRsc res) -> DataFrame```
- [ ] ```FromFields(std::initializer_list<FldT const> fields, size_t reservedValues, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecord(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices, std::span<T const> recValues, size_t capacity, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecords(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices, IterableOfIterable auto const& recValues, size_t capacity, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecords(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::initializer_list<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::initializer_list<R const>> recIndices, std::initializer_list<std::initializer_list<T>> recValues, size_t capacity, MemRsc res) -> DataFrame```

> Note: `FromFields*` construction is inherently index-family-specific (the `std::conditional_t`
> parameter selects bounds vs. span), so it is covered per family rather than via the UU/R1/Rs matrix.

### Adding Fields and Records

Test file: [RM_DataframeAddingTest.cpp](/lib/test/RM_DataframeAddingTest.cpp)

| Done | Function Name                                       | Returns     | UU | R1 | Rs |
|------|-----------------------------------------------------|-------------|----|----|----|
| ✅    | AddField(F index, T const& defaultValue)            | bool        | ✔  | —  | —  |
| ✅    | AddFields(std::span<F const> indices, T const& def) | std::size_t | ✔  | —  | —  |
| ✅    | AddRecord(R index, T const& defaultValue)           | bool        | ✔  | —  | —  |
| ✅    | AddRecords(std::span<R const> indices, T const& def)| std::size_t | ✔  | —  | —  |
| ☐    | AddRecordPopulated(R index, std::span<T const> recs)| bool        | ✘  | —  | —  |

Range-family (sequence-index) only:

- [ ] ```SetFieldRange(std::optional<FldT> lower, std::optional<FldT> upper, T const& defaultVal) -> bool```
- [ ] ```SetFieldRange(DFRangeIndexBounds<FldT>, T const& defaultVal) -> bool```
- [ ] ```SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, T const& defaultVal) -> bool```
- [ ] ```SetRecordRange(DFRangeIndexBounds<RecT>, T const& defaultVal) -> bool```
- [ ] ```SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::span<T const> records) -> bool```
- [ ] ```SetRecordRange(DFRangeIndexBounds<RecT>, std::span<T const> records) -> bool```
- [ ] ```SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, IterableOfIterable auto const& records) -> bool```
- [ ] ```SetRecordRange(DFRangeIndexBounds<RecT>, IterableOfIterable auto const& records) -> bool```
- [ ] ```SetRecordRange(std::optional<RecT> lower, std::optional<RecT> upper, std::initializer_list<std::initializer_list<T>> records) -> bool```
- [ ] ```SetRecordRange(DFRangeIndexBounds<RecT>, std::initializer_list<std::initializer_list<T>> records) -> bool```

> `SetFieldRange`/`SetRecordRange` must additionally cover `step == 1` **and** `step != 1`,
> including: off-grid bound moves are rejected, re-spacing a populated range is rejected,
> and element-count (not key-span) resizing.

### Removing Fields and Records

Test file: [RM_DataframeDropTest.cpp](/lib/test/RM_DataframeDropTest.cpp)

| Done | Function Name              | Returns | UU | R1 | Rs |
|------|----------------------------|---------|----|----|----|
| ✅    | DropField(F const& index)  | bool    | ✔  | —  | —  |
| ✅    | DropRecord(R const& index) | bool    | ✔  | —  | —  |

### Properties

Test file: [RM_DataframePropertiesTest.cpp](/lib/test/RM_DataframePropertiesTest.cpp)

| Done | Function Name                          | Returns | UU | R1 | Rs |
|------|----------------------------------------|---------|----|----|----|
| ✅    | HasField<C>(C const& field) const      | bool    | ✔  | ✔  | ✔  |
| ✅    | HasRecord<C>(C const& record) const    | bool    | ✔  | ✔  | ✔  |
| ✅    | Size() const                           | size_t  | ✔  | ✔  | ✔  |
| ✅    | FieldSize() const                      | size_t  | ✔  | ✔  | ✔  |
| ✅    | RecordSize() const                     | size_t  | ✔  | ✔  | ✔  |
| ✅    | Empty() const                          | bool    | ✔  | ✔  | ✔  |

### Accessing Values

Test file: [RM_DataframeAccessTest.cpp](/lib/test/RM_DataframeAccessTest.cpp)

| Done | Function Name                                           | Returns                | UU | R1 | Rs |
|------|---------------------------------------------------------|------------------------|----|----|----|
| ✅    | GetValue(FldT const& field, RecT const& record) const   | OptionalRef<T const>   | ✔  | ✔  | ✔  |
| ✅    | GetValue(FldT const& field, RecT const& record)         | OptionalRef<T>         | ✔  | ✔  | ✔  |
| ✅    | operator[](FldT const& field, RecT const& record)       | T&                     | ✔  | ✔  | ✔  |
| ✅    | operator[](FldT const& field, RecT const& record) const | T const&               | ✔  | ✔  | ✔  |
| ✅    | Data() const                                            | T const*               | ✔  | ✔  | ✔  |
| ✅    | MDSpan() const                                          | RecsData<T const>      | ✔  | ✔  | ✔  |
| ✅    | Values(this auto& self)                                 | std::span<Value<Self>> | ✔  | ✔  | ✔  |

### Mutating Values

Test file: [RM_DataframeMutatingTest.cpp](/lib/test/RM_DataframeMutatingTest.cpp)

| Done | Function Name                                                | Returns | UU | R1 | Rs |
|------|--------------------------------------------------------------|---------|----|----|----|
| ✅    | AssignValue(FldT const& field, RecT const& record, U&& value)| bool    | ✔  | ✔  | ✔  |
| ✅    | UpsertValue(FldT const& field, RecT const& record, U&& value)| void    | ✔  | —  | —  |
| ✅    | AssignFieldValues(FldT const& field, std::span<U const> vals)| bool    | ✔  | ✘  | ✘  |
| ✅    | AssignRecordValues(RecT const& record, std::span<U const> vs)| bool    | ✔  | ✘  | ✘  |

- TODO `AssignFieldValues`/`AssignRecordValues` are index-agnostic but currently only covered for `UU`

### Views

Test file: [RM_DataframeViewsTest.cpp](/lib/test/RM_DataframeViewsTest.cpp)

| Done | Function Name                          | Returns                | UU | R1 | Rs |
|------|----------------------------------------|------------------------|----|----|----|
| ✅    | ViewField(field) [const]               | DFView<T[ const], RecI>         | ✔  | ✔  | ✔  |
| ✅    | ViewFieldIndexed(field) [const]        | DFViewIndexed<T[ const], ...>   | ✔  | ✔  | ✔  |
| ✅    | ViewRecordIndexed(record) [const]      | DFViewIndexed<T[ const], ...>   | ✔  | ✔  | ✔  |
| ✅    | ViewRecord(record) [const]             | DFView<T[ const], FldI>         | ✔  | ✘  | ✘  |
| ✅    | operator\|(SelectField<F>) [const]     | DFView<T[ const], RecI>         | ✔  | ✘  | ✘  |
| ✅    | operator\|(SelectRecord<R>) [const]    | DFView<T[ const], FldI>         | ✔  | ✘  | ✘  |
| ✅    | operator\|(SelectFieldIndexed<F>) [const]  | DFViewIndexed<...>          | ✔  | ✘  | ✘  |
| ✅    | operator\|(SelectRecordIndexed<R>) [const] | DFViewIndexed<...>          | ✔  | ✘  | ✘  |
| ✅    | Fields() const                         | Flds                   | ✔  | ✔  | ✔  |
| ✅    | Records() const                        | Recs                   | ✔  | ✔  | ✔  |

- TODO `ViewRecord` and the four `operator\|` selectors are value-index only because their `requires
     DFSeqIndex` overloads are not implemented yet (the `✘` for `R1`/`Rs` is missing *production*, not
     a missing test). Once added, the typed suite extends to cover them.

### Sort

Test file: [RM_DataframeSortTest.cpp](/lib/test/RM_DataframeSortTest.cpp)

Value-index only (`requires DFUnqIndex`; an arithmetic range is already ordered); `—` = n/a.

| Done | Function Name              | Returns | UU | R1 | Rs |
|------|----------------------------|---------|----|----|----|
| ✅    | SortFields(Compare comp)   | void    | ✔  | —  | —  |
| ✅    | SortRecords(Compare comp)  | void    | ✔  | —  | —  |

### Functional

Test file: _TODO_

- [ ] ```ForEachOnField<Func>(F const& index, Func&& func) [const]```
- [ ] ```ForEachOnRecord<Func>(R const& index, Func&& func) [const]```

### IO

Test file: _TODO_

- [ ] ```Print() const -> void```
- [ ] ```PrintTo(std::ostream& stream) const -> void```
- [ ] ```PrintCSV(std::ostream& stream, char sep) const -> void```

### User-Types

Test file: [RM_DataframeUserTypesTest.cpp](/lib/test/RM_DataframeUserTypesTest.cpp)

Behavior with user-provided value/key types (not a per-function area).

- [x] non-trivial value type `T` (C++-like move/copy, rule of five): copy/move on add & upsert, destruction, no leaks
- [x] `std::string` keys: transparent hash / equality lookup (`std::string` / `std::string_view` / `const char*`)
- [ ] `std::chrono` keys: range index + formatting

---

## Examples

Usability-oriented walkthroughs in `lib/test/examples/` (assert, but read as usage):

- [ ] [RM_DFExamples_RangeIndices.cpp](/lib/test/examples/RM_DFExamples_RangeIndices.cpp) —
  building/mutating range-indexed frames, strided ranges, mdspan extraction.

---

TODO add other classes e.g. layouts, indices, selectors, views, containers, errors etc.
