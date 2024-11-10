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

- indexed iterator does not have the same usage like iterating over an map.
  - ``auto const& [val, idx]: dfView`` would still allow to mutate val if dfView is not const.
  - when doing the same e.g. std::unordered_map val would be constant.
- add more todo's