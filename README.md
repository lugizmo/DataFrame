# DataFrame

**This is work in progress and not ready for production!**

**Everything can change at this point!**

### Dependencies

This library uses **C++23** constructs (like mdspan). At moment of writing this is only available in llvm/clang.

- Compatible compiler 
- Project Options (gets fetched for you)
- GTest/Googletest (gets fetched for you, when using tests)

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

### TODO General

- rework dependency management to allow local versions
- add lldb tooling
- improve clang-format
- improve clang-tidy
- much better testing
- benchmarks
- code documentation
- usage examples
- watch out what gcc is doing and add support if possible
- add more todo's

### TODO Code

- more constexpr
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
- add more todo's