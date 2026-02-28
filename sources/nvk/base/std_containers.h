#ifndef NV_STD_CONTAINERS_
#define NV_STD_CONTAINERS_

#include <nvk_common.h>

#if !NV_USE_STD_MEMORY
#include <nvk/base/memory/MemoryManager.h>
#include <nvk/base/memory/STLAllocator.h>
#endif

namespace nv {

#if NV_USE_STD_MEMORY
using String = std::string;

using OStringStream = std::ostringstream;
using IStringStream = std::istringstream;

template <typename T> using List = std::list<T>;

template <typename T, typename Comp = std::less<T>>
using Set = std::set<T, Comp>;
template <typename T> using UnorderedSet = std::unordered_set<T>;

template <typename Key, typename T> using Map = std::map<Key, T>;

template <typename Key, typename T>
using UnorderedMap = std::unordered_map<Key, T>;

// Note: our vector should by default use the DefaultRootAllocator, because it
// could get significantly big:
template <typename T> using Vector = std::vector<T>;

template <typename T> using Deque = std::deque<T>;
#else
// template <typename T=char, typename MemAlloc=DefaultPoolAllocator>
using String = std::basic_string<char, std::char_traits<char>,
                                 STLAllocator<char, DefaultPoolAllocator>>;

using OStringStream =
    std::basic_ostringstream<char, std::char_traits<char>,
                             STLAllocator<char, DefaultPoolAllocator>>;
using IStringStream =
    std::basic_istringstream<char, std::char_traits<char>,
                             STLAllocator<char, DefaultPoolAllocator>>;

template <typename T, typename MemAlloc = DefaultPoolAllocator>
using List = std::list<T, STLAllocator<T, MemAlloc>>;

template <typename T, typename Comp = std::less<T>,
          typename MemAlloc = DefaultPoolAllocator>
using Set = std::set<T, Comp, STLAllocator<T, MemAlloc>>;

template <typename T, typename MemAlloc = DefaultPoolAllocator>
using UnorderedSet =
    std::unordered_set<T, std::less<T>, STLAllocator<T, MemAlloc>>;

template <typename Key, typename T, typename MemAlloc = DefaultPoolAllocator>
using Map = std::map<Key, T, std::less<Key>,
                     STLAllocator<std::pair<const Key, T>, MemAlloc>>;

template <typename Key, typename T, typename MemAlloc = DefaultPoolAllocator,
          typename Hash = std::hash<Key>>
using UnorderedMap =
    std::unordered_map<Key, T, Hash, std::equal_to<Key>,
                       STLAllocator<std::pair<const Key, T>, MemAlloc>>;

// Note: our vector should by default use the DefaultRootAllocator, because it
// could get significantly big:
template <typename T, typename MemAlloc = DefaultRootAllocator>
using Vector = std::vector<T, STLAllocator<T, MemAlloc>>;

template <typename T, typename MemAlloc = DefaultRootAllocator>
using Deque = std::deque<T, STLAllocator<T, MemAlloc>>;
#endif

template <typename T> using Queue = std::queue<T, Deque<T>>;

#define DEFINE_VECTOR(tname) using tname##Vector = Vector<nv::tname>;

DEFINE_VECTOR(VoidPtr)
DEFINE_VECTOR(Bool);
DEFINE_VECTOR(I8);
DEFINE_VECTOR(U8);
DEFINE_VECTOR(I16);
DEFINE_VECTOR(U16);
DEFINE_VECTOR(I32);
DEFINE_VECTOR(U32);
DEFINE_VECTOR(I64);
DEFINE_VECTOR(U64);
DEFINE_VECTOR(F32);
DEFINE_VECTOR(F64);
DEFINE_VECTOR(String);

#undef DEFINE_VECTOR

} // namespace nv

namespace std {
#if !NV_USE_STD_MEMORY
template <> struct hash<nv::String> {
    auto operator()(const nv::String& s) const -> std::size_t {
        return std::hash<std::string>()(s.c_str());
    }
};
#endif
} // namespace std

#endif
