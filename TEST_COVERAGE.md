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
  `NonTrivialType`).

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
- See `RM_DataframeTypedExampleTest.cpp` for the reference typed-test.
- Each index-agnostic behaviour is written **once** as a `TYPED_TEST` and runs against
  every config in the type list automatically.

---

## Dataframe

### cd-tors

Test file: [RM_DataframeTorsTest.cpp](/lib/test/RM_DataframeTorsTest.cpp)

- [ ] ```DataFrame(DataFrame const&)```
- [ ] ```operator=(DataFrame const&) -> auto```
- [ ] ```DataFrame(DataFrame&& other)```
- [ ] ```operator=(DataFrame&& other) -> DataFrame&```
- [ ] ```~DataFrame()```

### construction

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

### adding fields and records

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

### removing fields and records

Test file: [RM_DataframeDropTest.cpp](/lib/test/RM_DataframeDropTest.cpp)

| Done | Function Name              | Returns | UU | R1 | Rs |
|------|----------------------------|---------|----|----|----|
| ✅    | DropField(F const& index)  | bool    | ✔  | —  | —  |
| ✅    | DropRecord(R const& index) | bool    | ✔  | —  | —  |

### properties

Test file: [RM_DataframePropertiesTest.cpp](/lib/test/RM_DataframePropertiesTest.cpp)

- [ ] ```HasField<C>(C const& field) const -> bool``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```HasRecord<C>(C const& record) const -> bool``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```Size() const -> size_t``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```FieldSize() const -> size_t``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```RecordSize() const -> size_t``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```Empty() const -> bool``` — UU:[ ] R1:[ ] Rs:[ ]

### accessing values

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

### mutating values

Test file: [RM_DataframeMutatingTest.cpp](/lib/test/RM_DataframeMutatingTest.cpp)

| Done | Function Name                                                | Returns | UU | R1 | Rs |
|------|--------------------------------------------------------------|---------|----|----|----|
| ✅    | AssignValue(FldT const& field, RecT const& record, U&& value)| bool    | ✔  | ✔  | ✔  |
| ✅    | UpsertValue(FldT const& field, RecT const& record, U&& value)| void    | ✔  | —  | —  |
| ✅    | AssignFieldValues(FldT const& field, std::span<U const> vals)| bool    | ✔  | ✘  | ✘  |
| ✅    | AssignRecordValues(RecT const& record, std::span<U const> vs)| bool    | ✔  | ✘  | ✘  |

- TODO `AssignFieldValues`/`AssignRecordValues` are index-agnostic but currently only covered for `UU`

### views

Test file: [RM_DataframeViewsTest.cpp](/lib/test/RM_DataframeViewsTest.cpp)

- [ ] ```ViewField(F const& index) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```ViewFieldIndexed(F const& index) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```ViewRecord(R const& index) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```ViewRecordIndexed(R const& index) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```operator|(SelectField<F>) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```operator|(SelectRecord<R>) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```operator|(SelectFieldIndexed<F>) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```operator|(SelectRecordIndexed<R>) [const]``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```Fields() const -> Flds``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```Records() const -> Recs``` — UU:[ ] R1:[ ] Rs:[ ]

> `Values()` is raw value-buffer access — tracked under [accessing values](#accessing-values).

### sort

Test file: [RM_DataframeSortTest.cpp](/lib/test/RM_DataframeSortTest.cpp)

- [ ] ```SortFields(Compare comp = {})``` — UU:[ ] R1:[ ] Rs:[ ]
- [ ] ```SortRecords(Compare comp = {})``` — UU:[ ] R1:[ ] Rs:[ ]

### functional

Test file: _TODO_

- [ ] ```ForEachOnField<Func>(F const& index, Func&& func) [const]```
- [ ] ```ForEachOnRecord<Func>(R const& index, Func&& func) [const]```

### io

Test file: _TODO_

- [ ] ```Print() const -> void```
- [ ] ```PrintTo(std::ostream& stream) const -> void```
- [ ] ```PrintCSV(std::ostream& stream, char sep) const -> void```

### non-trivial element types

Test file: [RM_DataframeNonTrivialTypeTest.cpp](/lib/test/RM_DataframeNonTrivialTypeTest.cpp)

- [ ] move/copy/destruction semantics for non-trivially-copyable `T`

---

## Examples

Usability-oriented walkthroughs in `lib/test/examples/` (assert, but read as usage):

- [ ] [RM_DFExamples_RangeIndices.cpp](/lib/test/examples/RM_DFExamples_RangeIndices.cpp) —
  building/mutating range-indexed frames, strided ranges, mdspan extraction.

---

TODO add other classes e.g. layouts, indices, selectors, views, containers, errors etc.
