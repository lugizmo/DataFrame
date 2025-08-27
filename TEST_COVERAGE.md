# Test Coverage

## Dataframe

### dataframe cd-tors

- [ ] ```DataFrame(MemRsc res)```
- [ ] ```DataFrame(size_t reservedValues, MemRsc res)```
- [ ] ```FromFields(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<FldT const>> fields, size_t reservedValues, MemRsc res) -> DataFrame```
- [ ] ```FromFields(std::initializer_list<FldT const> fields, size_t reservedValues, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecord(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices, std::span<T const> recValues, size_t capacity, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecords(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::span<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::span<R const>> recIndices, IterableOfIterable auto const& recValues, size_t capacity, MemRsc res) -> DataFrame```
- [ ] ```FromFieldsAndRecords(std::conditional_t<IsFISeq, DFRangeIndexBounds<FldT>, std::initializer_list<F const>> fldIndices, std::conditional_t<IsRISeq, DFRangeIndexBounds<RecT>, std::initializer_list<R const>> recIndices, std::initializer_list<std::initializer_list<T>> recValues, size_t capacity, MemRsc res) -> DataFrame```
- [ ] ```DataFrame(DataFrame const&)```
- [ ] ```operator=(DataFrame const&) -> auto```
- [ ] ```DataFrame(DataFrame&& other)```
- [ ] ```operator=(DataFrame&& other) -> DataFrame&```
- [ ] ```~DataFrame()```

### adding fields and records

Test can be found in: [File](/lib/test/RM_DataframeAddingTest.cpp)

- [x] ```AddField(F index, T const& defaultValue) -> bool```
- [x] ```AddFields(std::span<F const> const indices, T const& defaultValue) -> std::size_t```
- [x] ```AddRecord(R index, T const& defaultValue) -> bool```
- [x] ```AddRecords(std::span<R const> const indices, T const& defaultValue) -> std::size_t```


- [ ] ```AddRecordPopulated(R index, std::span<T const> records) -> bool```


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


- [ ] ```InsertValue(FldT const& field, RecT const& record, T const& value) -> bool;``` 
- [ ] ```InsertOrAssignValue(FldT const& field, RecT const& record, T const& value) -> bool;```

### removing fields and records

- [ ] ```DropField(F const& index) -> bool```
- [ ] ```DropRecord(R const& index) -> bool```

### dataframe properties

Test can be found in: [File](/lib/test/RM_DataframePropertiesTest.cpp)

- [x] ```HasField<C>(C const& field) const -> bool```
- [x] ```HasRecord<C>(C const& record) const -> bool```


- [x] ```Size() const -> size_t```
- [x] ```FieldSize() const -> size_t```
- [x] ```RecordSize() const -> size_t```
- [x] ```Empty() const -> bool```

### accessing values

Test can be found in: [File](/lib/test/RM_DataframeAccessTest.cpp)

- [x] ```GetValue(FldT const& field, RecT const& record) const -> std::optional<std::reference_wrapper<T const>>```
- [x] ```GetValue(FldT const& field, RecT const& record) -> std::optional<std::reference_wrapper<T>>```


- [x] ```operator[](FldT const& field, RecT const& record) -> T&```
- [x] ```operator[](FldT const& field, RecT const& record) const -> T const&```


- [x] ```Data() const -> T const*```

### mutating values

Test can be found in: [File](/lib/test/RM_DataframeMutatingTest.cpp)

- [x] ```AssignValue(FldT const& field, RecT const& record, T const& value) -> bool```

### view fields and records

Test can be found in: [File](/lib/test/RM_DataframeViewsTest.cpp)

- [x] ```ViewField(F const& index) -> DFView<T, RecI>```
- [x] ```ViewField(F const& index) const -> DFView<T const, RecI>```
- [x] ```ViewField(FldT const& index) -> DFView<T, RecI>```
- [x] ```ViewField(FldT const& index) const -> DFView<T const, RecI>```


- [x] ```ViewFieldIndexed(F const& index) -> DFViewIndexed<T, RecI const>```
- [x] ```ViewFieldIndexed(F const& index) const -> DFViewIndexed<T const, RecI const>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```ViewRecord(R const& index) -> DFView<T, FldI>```
- [x] ```ViewRecord(R const& index) const -> DFView<T const, FldI>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```ViewRecordIndexed(R const& index) -> DFViewIndexed<T, FldI const>```
- [x] ```ViewRecordIndexed(R const& index) const -> DFViewIndexed<T const, FldI const>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```operator|(SelectField<F> const& index) -> DFView<T, RecI>```
- [x] ```operator|(SelectField<F> const& index) const -> DFView<T const, RecI>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```operator|(SelectRecord<R> const& index) -> DFView<T, FldI>```
- [x] ```operator|(SelectRecord<R> const& index) const -> DFView<T const, FldI>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```operator|(SelectFieldIndexed<F> const& index) -> DFViewIndexed<T, RecI const>```
- [x] ```operator|(SelectFieldIndexed<F> const& index) const -> DFViewIndexed<T const, RecI const>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```operator|(SelectRecordIndexed<R> const& index) -> DFViewIndexed<T, RecI const>```
- [x] ```operator|(SelectRecordIndexed<R> const& index) const -> DFViewIndexed<T const, RecI const>```
- [ ] add sequence version
- [ ] add sequence version


- [x] ```Fields() const -> Flds```
- [x] ```Records() const -> Recs```
- [x] ```Values() const -> std::span<T>```

### functional

- [ ] ```ForEachOnField<Func>(F const& index, Func&& func) -> DFView<T, RecI>```
- [ ] ```ForEachOnField<Func>(F const& index, Func&& func) const -> DFView<T const, RecI>```
- [ ] ```ForEachOnRecord<Func>(R const& index, Func&& func) -> DFView<T, FldI>```
- [ ] ```ForEachOnRecord<Func>(R const& index, Func&& func) const -> DFView<T const, FldI>```

### io

- [ ] ```Print() const -> void```
- [ ] ```PrintTo(std::ostream& stream) const -> void```
- [ ] ```PrintCSV(std::ostream& stream, char sep) const -> void```

TODO add other classes e.g., layouts, indices, selectors, views, containers, errors etc.