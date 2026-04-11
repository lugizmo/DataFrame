# DataFrame

**This is work in progress and not ready for production!**

**Everything can change at this point!**

### Dependencies

This library uses **C++23** constructs (like mdspan). At moment of writing this is only available in llvm/clang.

- Compatible compiler 
- Project Options (gets fetched for you)
- GTest/Googletest (gets fetched for you, when using tests)

### Dataframes

Dataframes can store every c++ type as values that are default constructable. The records and field indices must
conform to: TODO add rule

Data is stored as a consecutive block of memory. This is a decision to optimize iterations over memory and having less 
indirection when working with the data. That also means you should try to define the shape of your data at the beginning
if possible. And then optimizing by choosing an appropriate memory layout as described next. 

You can choose between layout_right (row major, default) or layout_left (column major) as the underlying storage memory
order. The interface of the dataframe does not change. Choose layout_right when adding records/rows more frequently, and
layout_left when adding fields/columns is more frequent.

Dataframes use raw memory in the background, and a std::memory_resource is used for allocations. All constructors and
constructor functions optionally accept a shared_memory resource and a capacity that can be used for memory management.

### Design Decisions

- not thread safe
- no exceptions
- iterators/pointers/references are invalidated by mutation of the dataframe
- consecutive memory for storage
- dataframes use field and record lookup but allow for range versions for instant lookup (see examples)

### Fundamental Naming

| Concept | Description                          |
|---------|--------------------------------------|
| Value   | Element type                         |
| Field   | Field/Column type                    |
| Record  | Record/Row type                      |
| View    | Iterator, span, reference or pointer |

### Examples

## Creating

The following lists functions on how to create/initialize a Dataframe. The simplest method is to create an empty 
DataFrame (best with pre-allocated memory) and add fields and records afterward. But you can also initialize all 
at once for optimal initialization and when you know what you're going to add. As mentioned before, capacity and 
memory_resource can be passed to the dataframe on construction.

Default construction of a dataframe. 
```c++
// both with optionally passed memory_resource
auto default  = lugizmo::DataFrame<int, int, int>{};
auto reserved = lugizmo::DataFrame<int, int, int>{50}; 
```

You can create a DataFrame from known fields and records using a span or initializer list of fields and records. 
Again, memory can also be reserved in advance and a memory_resource can be passed.
(TODO add example for FromRecords)
```c++
// create empty dataframe with known fields:
auto dfFromAr = lugizmo::DataFrame<int, int, int>::FromFields(std::array{1, 2, 3, 4});
auto dfFromIn = lugizmo::DataFrame<int, int, int>::FromFields({1, 2, 3, 4});

// also possible to pass values to fill records with:
auto df = lugizmo::DataFrame<int, int, int>::FromFieldsAndRecord({1, 2, 3, 4}, {1, 2, 3, 4}, {10, 20, 30, 40});
// Fields  :   1    2    3    4
// Records :---------------------
//     1   |   10   20   30   40
//     2   |   10   20   30   40
//     3   |   10   20   30   40
//     4   |   10   20   30   40


// you can also create a dataframe with explicitly setting all values:
auto df = lugizmo::DataFrame<int, int, int>::FromFieldsAndRecords({1, 2, 3, 4}, {1, 2, 3, 4}, {
                                                                  {1.0f, 2.0f, 3.0f, 4.0f}, 
                                                                  {1.0f, 2.0f, 3.0f, 4.0f},
                                                                  {1.0f, 2.0f, 3.0f, 4.0f}});
// Fields  :   1    2    3    4
// Records :---------------------
//     1   |   1    2    3    4
//     2   |   1    2    3    4
//     3   |   1    2    3    4
//     4   |   1    2    3    4
```

## Accessing, View, and Iteration

The access API keeps the public return types small and predictable.

Plain pointer return types are non-null by contract. When an access operation may not yield a pointer, the nullable form
is expressed explicitly as `std::optional<Value*>`.  
Multi-value access uses standard library view types when the selected access pattern is contiguous and `ValueView` based
types when iteration is non-contiguous or index-aware.

TODO this needs an update see DataFrame.h for available types
| Concept                     | Spelling                  | Description                                                                               |
|-----------------------------|---------------------------|-------------------------------------------------------------------------------------------|
| value type                  | `Value`                   | Copy of the underlying stored value.                                                      |
| pointer                     | `Value*`                  | Non-null pointer to an underlying stored value.                                           |
| maybe value                 | `std::optional<Value>`    | Optional copy of a single value.                                                          |
| maybe pointer               | `std::optional<Value*>`   | Optional non-null pointer to an underlying stored value.                                  |
| contiguous many             | `std::span<Value>`        | View over a contiguous sequence of values when the selected access pattern is contiguous. |
| many / generic view         | `ValueView`               | Iterator over a sequence of values.                                                       |
| many / generic indexed view | `IndexedValueView`        | Iterator over a sequence of values including the index (zip view).                        |
| raw 2D                      | `std::mdspan<Value, ...>` | View over underlying data blob.                                                           |

### DataFrame targeted features

- Storage is consecutive in memory
- Records (Rows) and Fields (Columns) can be accessed via an key
- Exception free

Views:
- views should be compatible with ranges
- generally more ranges support
- add (indexed) views of (indexed) views (traversing set-of/all fields/records)

Functional:
- transform functions -> creating new DF
- apply functions on fields/records
- transform and apply on field-ranges & column-ranges

### DataFrame currently not planned features

- thread safe mutation/access

### TODO Crucial

- check for gtest version if gtest is already present does not work!

### TODO General

- put all types and functions etc. not intended for the user in a separate namespace
- rework dependency management to allow local versions
- add lldb tooling
- improve clang-format
- improve clang-tidy
- much better testing
- benchmarks
- code documentation
- usage examples
- watch out what gcc is doing and add support if possible
- handling user types that might throw in any of it's constructors
- when looking at a view of record or field enable not just iterate but also doing lookups by using operator[] or .Get or something like this
- shrink memory to current capacity
- allow moving in data via a pointer T* (user is responsible the data is of field x record size)
  - adding a overload taking fields and records + std::container were I shrink it or use it's capacity so that there is no leak
- add more todo's

### TODO Code

- more constexpr
- add merge of two dataframes of the same type
- add multi type version or specialization for single type
- todo thing about using lugizmo::df to drop the DF names everywhere
  - DFView -> lugizmo::df::View -> df::View
  - DFHashIndex -> lugizmo::df::IndexHash -> df::IndexHash
- Index types should have an hint on which search algorithm to use
  - default binary
  - interpolation search
  - exponential search
  - jump-search
- rethink the naming of DFView and DFViewIndexed
  - views are non owning lazy ranges without creating any copys
  - ranges are abstraction of anything having a begin() and end() and can be immediate or lazy
- ~~indexed iterator does not have the same usage like iterating over an map.~~
- add reversed (index) iterator  
- make views not eagerly so that they fit better with ranges.
- add test for mixed dataframe index types
- handling of exception enabled code. E.g. std::uninitialized_fill_n will handle ctor's etc. and therefor exceptions might happen.
  - think about using memcpy instead of std::uninitialized_fill_n or making it an option
- add more todo's

### TODO Benchmarks

- Manipulating "big" dataframes, so not testing creating but really just adding columns/row with row-/col major layouts when already big 
