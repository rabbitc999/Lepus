#include "lepus_i18n.hpp"
#ifndef PARSE_ENGINE_LEPUS_RUNTIME_ALLOCATOR_HPP
#define PARSE_ENGINE_LEPUS_RUNTIME_ALLOCATOR_HPP
/* Explicit C-backed allocators. No global operator new/delete replacement. */
#ifndef LEPUS_CUSTOM_ALLOCATOR
#define LEPUS_CUSTOM_ALLOCATOR 1
#endif
#ifndef PARSER_MEMORY_NAMESPACE
#define PARSER_MEMORY_NAMESPACE Memory
#endif
#include "allocator/lepus_allocator.hpp"
#endif
