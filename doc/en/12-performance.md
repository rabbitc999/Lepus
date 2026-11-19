# Lepus 1.0.0 performance measurements

<!-- LANGUAGE NAVIGATION -->
**English** · [简体中文](../12-performance.md)
<!-- END LANGUAGE NAVIGATION -->

All values below were collected in this run on **2026-11-17**, from 18 workloads and eight languages: **144 combinations, 3600 timed observations**. No earlier timings are reused. Each cell is the median of 25 intervals, in **milliseconds**.

## Environment and identity

Host: **AMD Ryzen 5 5600X 6-Core Processor**, 12 logical CPUs; `Windows-11-10.0.26200-SP0`. Sampling started at `2026-11-17T14:51:54.231871+08:00` and finished at `2026-11-17T14:55:17.664578+08:00` (UTC+08:00).

Measured Lepus compiler source: `575dfb26339e72032e8424b93c44583e2de3fc28`; executable SHA-256: `0f2c143bafb4b74896d4fc1e2e69288966ff7fcebdccaded53f88bf22dd94a9b`. This identifies the measured executable; subsequent documentation builds may have different bytes. Workload sources, generated wrappers, tool versions and artifact hashes are retained with the raw records.

### Toolchain

| Language | Actual tool version |
| --- | --- |
| Lepus | Lepus 1.0.0 / static LLVM 23.1.4 |
| C | gcc.exe (Rev6, Built by MSYS2 project) 16.2.0 |
| C++ | g++.exe (Rev6, Built by MSYS2 project) 16.2.0 |
| Rust | rustc 1.100.0 (0ef1bc386 2026-11-09) |
| Java | java version "22.0.1" 2024-04-16 |
| Lua | Lua 5.5.1 (official VM, monotonic clock host) |
| Python | Python 3.12.3 |
| Go | go version go1.27.1 windows/amd64 |

## Method

Build and linking finish before sampling. Every combination runs in five independent processes. Each process records five intervals using seeds 0–4; native and interpreted implementations warm once beforehand. Java uses default tiered compilation and GC with `-Xbatch`, warming for at least 32 calls and at least one second per process. Warmup counts and durations remain in the raw output. This does not prove that every method has reached a final compilation tier.

Lepus uses `build --shared --unchecked --allocator malloc`, with default LLVM O3/LTO. C/C++ use GCC O3/LTO; Rust uses optimization level 3, fat LTO and one codegen unit. Go uses its default optimizer. Lua and CPython use their normal interpreters and memory management. No optimization or implementation is changed after inspecting timings.

All workloads and languages are rerun. Five-seed integer results must match exactly; floating-point results are checked with relative and absolute tolerances of 1e-12. Compilation, process startup, global setup, warmup and checksum aggregation are outside each interval. Local allocation, growth, copying and cleanup performed by the work function are inside it. GC outside an interval is not charged back to that interval.

Sampling is serial; workload and language order are shuffled reproducibly with seed 20261106 on each round. All valid observations are retained, including slow samples. CPU frequency and affinity are not fixed and the operating system and other applications remain active. The ranges below show variation in this environment. No overall language score is calculated.

## Numeric and container workloads

| Workload | Lepus | C | C++ | Rust | Java | Lua | Python | Go |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `int_chain` | 0.8650 | 4.3203 | 4.3274 | 0.8755 | 4.9766 | 28.8422 | 332.1939 | 4.5364 |
| `float_chain` | 1.2963 | 1.3035 | 1.2990 | 1.2975 | 1.2980 | 5.6551 | 31.3853 | 1.3026 |
| `call` | 0.2155 | 0.8674 | 0.8677 | 1.0839 | 1.0835 | 17.8962 | 117.8942 | 1.0942 |
| `array` | 10.2056 | 7.0379 | 6.9417 | 6.2335 | 6.1583 | 42.6954 | 205.2060 | 7.3182 |
| `array_rw` | 24.7831 | 8.6540 | 8.6569 | 6.9762 | 8.6962 | 68.9648 | 511.5395 | 8.7053 |
| `list_growth` | 5.9256 | 1.7808 | 2.1084 | 1.8585 | 2.9190 | 12.0088 | 56.9180 | 2.8916 |
| `map_growth` | 58.8178 | 1.6981 | 1.6751 | 2.0525 | 3.6058 | 4.1881 | 20.3026 | 2.5545 |
| `set_growth` | 62.2301 | 1.7019 | 1.6743 | 1.5560 | 3.4721 | 4.0800 | 17.4583 | 2.5639 |

Sizes: integer chain and fixed-array tasks 4,000,000 iterations; float chain and small-function task 1,000,000; List 400,000 appends; Map/Set 20,000 dispersed keys with insert/overwrite or duplicate insert, hit/miss queries, deletion and reinsertion. All growth workloads start empty without reserving capacity. The previous mixed preallocation `list` case is retired in favor of `list_growth`.

## Payloads, method dispatch, structures and multidimensional arrays

| Workload | Lepus | C | C++ | Rust | Java | Lua | Python | Go |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `enum_payload` | 0.8626 | 0.8598 | 0.8101 | 1.2214 | 0.8487 | 10.1157 | 53.5000 | 0.6713 |
| `interface_mono` | 10.7148 | 0.3143 | 0.2891 | 0.0565 | 0.1950 | 6.3055 | 32.8008 | 0.4239 |
| `interface_poly` | 97.6147 | 0.4096 | 0.2959 | 0.4187 | 2.0480 | 9.7684 | 42.3919 | 0.5905 |
| `struct_nested` | 1.8012 | 1.1289 | 1.1283 | 1.0649 | 2.6851 | 23.8392 | 76.3463 | 1.2445 |
| `array_2d` | 0.6660 | 0.5879 | 0.6324 | 0.6300 | 0.8435 | 6.6608 | 39.5465 | 0.6391 |
| `array_3d` | 0.7451 | 0.6834 | 0.6966 | 0.6164 | 1.1896 | 8.5685 | 45.4049 | 0.6705 |

Each work function executes 262,144 iterations. Enum and nested-structure inputs contain 256 entries; arrays are 64×64 and 16×16×16. Inputs are built before timing. The method tasks use one implementation or a 2:1:1 mix of three implementations. Normal inlining and devirtualization remain allowed.

## UTF-8 byte tasks

| Workload | Lepus | C | C++ | Rust | Java | Lua | Python | Go |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `string_read` | 2.5197 | 0.5416 | 0.5400 | 0.5368 | 0.5405 | 5.8885 | 25.2156 | 0.5746 |
| `string_concat` | 13.0475 | 6.5745 | 0.1186 | 0.0670 | 6.2898 | 26.6039 | 6.8641 | 6.8563 |
| `string_slice` | 5.2537 | 0.1941 | 0.2234 | 0.0694 | 0.0172 | 0.2997 | 0.4265 | 0.1327 |
| `string_find` | 3.2133 | 0.0172 | 0.0603 | 0.0397 | 0.3830 | 0.0761 | 0.0990 | 0.0744 |

The input contains 8448 UTF-8 bytes. Workloads perform 131,072 dependent byte reads, 2048 successive concatenations followed by complete-byte reduction, 1024 pairs of owning slices and comparisons, or 256 searches with present and absent needles. Positions and results are byte-based.

## Implementation differences

Container, object representation, library algorithm and runtime costs are part of these implementations. C uses an explicit open-addressed hash reference; C++ and Rust use standard containers. Go uses slices and maps; Java uses ArrayList/HashMap/HashSet with Long boxing; Python uses list/dict/set; Lua uses tables. C models payloads and dispatch with tags and function pointers, Rust with enum/dyn trait, and dynamic languages with objects and method lookup. These are task comparisons, not identical ABIs or storage layouts.

String rows compare UTF-8 byte tasks, not all languages’ Unicode String APIs. C++ and Rust append to growable strings and can reuse capacity; the C, Lepus, Lua, Python and Go/Java byte implementations create successive concatenation results. Python uses bytes; Go and Java use byte sequences with explicit owned slice copies. Java byte search is an explicit loop; other implementations may use optimized library searches. Allocation strategies and library algorithms differ; these rows do not isolate language-compiler costs.

Single-thread kernels, library operations and warmed method loops do not represent application startup, concurrent servers, I/O, build time or peak memory. No language-wide conclusion follows from one row. Read each result together with its source, representation and full range.

## Location, range and quantiles

Every row uses all 25 observations. Time values are in milliseconds. Four quantile points are P25, P50, P75 and P100; P50 is the median and P100 is the maximum. R7 linearly interpolates the sorted sample at zero-based position (n−1)p. With n=25 these are the 7th, 13th, 19th and 25th ordered values. See the [R quantile definition](https://stat.ethz.ch/R-manual/R-devel/library/stats/html/quantile.html).

| Workload | Language | Minimum | Mean | P25 | Median / P50 | P75 | Maximum / P100 |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `int_chain` | Lepus | 0.8618 | 0.8977 | 0.8623 | 0.8650 | 0.8853 | 1.0958 |
| `int_chain` | C | 4.3147 | 4.3561 | 4.3166 | 4.3203 | 4.3291 | 5.0864 |
| `int_chain` | C++ | 4.3146 | 4.3881 | 4.3192 | 4.3274 | 4.3431 | 5.0002 |
| `int_chain` | Rust | 0.8618 | 0.9225 | 0.8643 | 0.8755 | 0.9820 | 1.1149 |
| `int_chain` | Java | 4.9607 | 5.0460 | 4.9666 | 4.9766 | 5.0112 | 5.8120 |
| `int_chain` | Lua | 28.5830 | 29.1938 | 28.6925 | 28.8422 | 29.3062 | 32.4919 |
| `int_chain` | Python | 313.8524 | 349.0646 | 321.8156 | 332.1939 | 369.2366 | 501.3194 |
| `int_chain` | Go | 4.3129 | 4.6670 | 4.4024 | 4.5364 | 4.8720 | 5.4725 |
| `float_chain` | Lepus | 1.2928 | 1.3023 | 1.2947 | 1.2963 | 1.2987 | 1.3779 |
| `float_chain` | C | 1.2927 | 1.3622 | 1.2977 | 1.3035 | 1.3816 | 1.6459 |
| `float_chain` | C++ | 1.2927 | 1.3625 | 1.2961 | 1.2990 | 1.3332 | 1.7601 |
| `float_chain` | Rust | 1.2932 | 1.3300 | 1.2962 | 1.2975 | 1.3007 | 1.8208 |
| `float_chain` | Java | 1.2928 | 1.3511 | 1.2958 | 1.2980 | 1.3080 | 1.6367 |
| `float_chain` | Lua | 5.4636 | 5.7610 | 5.6173 | 5.6551 | 5.7420 | 6.8102 |
| `float_chain` | Python | 29.7582 | 32.8298 | 30.5419 | 31.3853 | 34.3830 | 45.5743 |
| `float_chain` | Go | 1.2929 | 1.3210 | 1.2963 | 1.3026 | 1.3151 | 1.6591 |
| `call` | Lepus | 0.2154 | 0.2169 | 0.2155 | 0.2155 | 0.2190 | 0.2215 |
| `call` | C | 0.8618 | 0.9200 | 0.8638 | 0.8674 | 0.9597 | 1.0935 |
| `call` | C++ | 0.8618 | 0.9233 | 0.8637 | 0.8677 | 0.9831 | 1.1020 |
| `call` | Rust | 1.0773 | 1.1345 | 1.0798 | 1.0839 | 1.1067 | 1.3770 |
| `call` | Java | 1.0774 | 1.1384 | 1.0795 | 1.0835 | 1.1892 | 1.3615 |
| `call` | Lua | 16.6989 | 18.0284 | 17.4201 | 17.8962 | 18.5085 | 20.7187 |
| `call` | Python | 112.3987 | 122.4565 | 114.8132 | 117.8942 | 123.7914 | 161.1259 |
| `call` | Go | 1.0774 | 1.1530 | 1.0830 | 1.0942 | 1.2029 | 1.3751 |
| `array` | Lepus | 9.8086 | 11.0953 | 9.9715 | 10.2056 | 11.0682 | 17.1363 |
| `array` | C | 6.9016 | 7.3257 | 6.9132 | 7.0379 | 7.4559 | 9.2070 |
| `array` | C++ | 6.9038 | 7.6046 | 6.9148 | 6.9417 | 8.3979 | 10.1053 |
| `array` | Rust | 6.2127 | 6.2758 | 6.2191 | 6.2335 | 6.2496 | 6.6993 |
| `array` | Java | 6.1484 | 6.2353 | 6.1526 | 6.1583 | 6.1769 | 7.1658 |
| `array` | Lua | 34.4787 | 41.3941 | 39.9596 | 42.6954 | 43.2857 | 46.2597 |
| `array` | Python | 195.4597 | 218.1656 | 199.2158 | 205.2060 | 224.6694 | 323.8104 |
| `array` | Go | 6.9109 | 7.4447 | 7.0780 | 7.3182 | 7.6386 | 10.1879 |
| `array_rw` | Lepus | 24.0851 | 25.3837 | 24.4037 | 24.7831 | 25.3447 | 33.7246 |
| `array_rw` | C | 8.6299 | 8.7188 | 8.6392 | 8.6540 | 8.6871 | 9.3215 |
| `array_rw` | C++ | 8.6303 | 8.6974 | 8.6405 | 8.6569 | 8.6776 | 9.4211 |
| `array_rw` | Rust | 6.9486 | 6.9797 | 6.9654 | 6.9762 | 6.9892 | 7.0287 |
| `array_rw` | Java | 8.6334 | 8.8225 | 8.6519 | 8.6962 | 8.9259 | 9.9287 |
| `array_rw` | Lua | 62.7965 | 70.0178 | 67.2269 | 68.9648 | 74.3616 | 76.8938 |
| `array_rw` | Python | 491.1568 | 525.5681 | 500.1405 | 511.5395 | 526.3614 | 789.7872 |
| `array_rw` | Go | 8.6320 | 9.0988 | 8.6739 | 8.7053 | 9.5442 | 10.2579 |
| `list_growth` | Lepus | 5.3972 | 7.7498 | 5.5998 | 5.9256 | 9.7562 | 13.1898 |
| `list_growth` | C | 1.5447 | 1.7983 | 1.7277 | 1.7808 | 1.8852 | 2.0272 |
| `list_growth` | C++ | 1.8429 | 2.1611 | 2.0030 | 2.1084 | 2.2514 | 3.1960 |
| `list_growth` | Rust | 1.6487 | 1.9145 | 1.7563 | 1.8585 | 1.9706 | 2.4867 |
| `list_growth` | Java | 2.4881 | 3.3881 | 2.7263 | 2.9190 | 3.6698 | 6.7337 |
| `list_growth` | Lua | 10.9803 | 12.7812 | 11.7532 | 12.0088 | 13.2749 | 16.5354 |
| `list_growth` | Python | 55.5622 | 57.3316 | 56.4223 | 56.9180 | 57.9822 | 61.1514 |
| `list_growth` | Go | 2.1715 | 3.2485 | 2.5580 | 2.8916 | 3.0487 | 11.2271 |
| `map_growth` | Lepus | 55.8172 | 59.3298 | 57.9105 | 58.8178 | 59.7689 | 69.5974 |
| `map_growth` | C | 1.6665 | 1.7393 | 1.6881 | 1.6981 | 1.7430 | 2.1331 |
| `map_growth` | C++ | 1.6225 | 1.8247 | 1.6545 | 1.6751 | 1.7775 | 3.6089 |
| `map_growth` | Rust | 1.9730 | 2.2191 | 2.0089 | 2.0525 | 2.0855 | 6.2822 |
| `map_growth` | Java | 3.3523 | 3.8048 | 3.5314 | 3.6058 | 3.8613 | 5.4248 |
| `map_growth` | Lua | 3.9712 | 4.5033 | 4.1289 | 4.1881 | 4.4048 | 8.2998 |
| `map_growth` | Python | 19.4381 | 20.6353 | 20.0640 | 20.3026 | 20.8461 | 23.7426 |
| `map_growth` | Go | 2.1671 | 2.7565 | 2.3785 | 2.5545 | 2.9720 | 5.1491 |
| `set_growth` | Lepus | 58.3519 | 63.3251 | 60.2536 | 62.2301 | 64.5449 | 72.9791 |
| `set_growth` | C | 1.5925 | 1.7736 | 1.6432 | 1.7019 | 1.7702 | 2.5584 |
| `set_growth` | C++ | 1.5961 | 1.8587 | 1.6439 | 1.6743 | 1.7738 | 2.9230 |
| `set_growth` | Rust | 1.5066 | 1.6879 | 1.5417 | 1.5560 | 1.7173 | 2.9121 |
| `set_growth` | Java | 3.0931 | 3.5433 | 3.2642 | 3.4721 | 3.8291 | 4.4015 |
| `set_growth` | Lua | 3.8294 | 4.1863 | 4.0100 | 4.0800 | 4.1824 | 5.3007 |
| `set_growth` | Python | 16.8523 | 17.8296 | 17.3131 | 17.4583 | 18.2043 | 19.1250 |
| `set_growth` | Go | 1.9455 | 2.5477 | 2.2010 | 2.5639 | 2.7577 | 3.6068 |
| `enum_payload` | Lepus | 0.8534 | 0.9448 | 0.8569 | 0.8626 | 0.9693 | 1.5070 |
| `enum_payload` | C | 0.8529 | 0.8707 | 0.8567 | 0.8598 | 0.8623 | 1.0526 |
| `enum_payload` | C++ | 0.8071 | 0.8196 | 0.8093 | 0.8101 | 0.8126 | 0.9817 |
| `enum_payload` | Rust | 1.1923 | 1.2523 | 1.2098 | 1.2214 | 1.2993 | 1.4911 |
| `enum_payload` | Java | 0.8432 | 0.8859 | 0.8460 | 0.8487 | 0.8764 | 1.0784 |
| `enum_payload` | Lua | 9.1753 | 10.2478 | 9.6128 | 10.1157 | 10.6597 | 12.5574 |
| `enum_payload` | Python | 51.1378 | 55.2903 | 52.2348 | 53.5000 | 56.7159 | 70.0281 |
| `enum_payload` | Go | 0.6487 | 0.7241 | 0.6628 | 0.6713 | 0.8115 | 0.8296 |
| `interface_mono` | Lepus | 10.3332 | 10.9658 | 10.5678 | 10.7148 | 11.0613 | 12.9418 |
| `interface_mono` | C | 0.2824 | 0.3194 | 0.2837 | 0.3143 | 0.3530 | 0.3686 |
| `interface_mono` | C++ | 0.2824 | 0.2965 | 0.2825 | 0.2891 | 0.2923 | 0.3646 |
| `interface_mono` | Rust | 0.0564 | 0.0568 | 0.0565 | 0.0565 | 0.0565 | 0.0612 |
| `interface_mono` | Java | 0.1695 | 0.1917 | 0.1696 | 0.1950 | 0.2119 | 0.2122 |
| `interface_mono` | Lua | 6.1394 | 6.4036 | 6.2098 | 6.3055 | 6.4311 | 7.4263 |
| `interface_mono` | Python | 31.6281 | 33.7675 | 32.3816 | 32.8008 | 35.6132 | 37.0895 |
| `interface_mono` | Go | 0.3390 | 0.4152 | 0.4083 | 0.4239 | 0.4307 | 0.5650 |
| `interface_poly` | Lepus | 90.8902 | 98.9562 | 95.2884 | 97.6147 | 101.6594 | 122.4229 |
| `interface_poly` | C | 0.4095 | 0.4140 | 0.4095 | 0.4096 | 0.4156 | 0.4542 |
| `interface_poly` | C++ | 0.2824 | 0.3385 | 0.2825 | 0.2959 | 0.3873 | 0.4709 |
| `interface_poly` | Rust | 0.3672 | 0.4547 | 0.4096 | 0.4187 | 0.5154 | 0.5791 |
| `interface_poly` | Java | 1.9426 | 2.1184 | 2.0127 | 2.0480 | 2.1735 | 2.5642 |
| `interface_poly` | Lua | 9.4604 | 9.9530 | 9.6002 | 9.7684 | 10.2068 | 11.2768 |
| `interface_poly` | Python | 41.4216 | 45.3267 | 41.8565 | 42.3919 | 47.3022 | 57.9405 |
| `interface_poly` | Go | 0.5189 | 0.5979 | 0.5325 | 0.5905 | 0.6644 | 0.7409 |
| `struct_nested` | Lepus | 1.7060 | 1.8605 | 1.7570 | 1.8012 | 1.8713 | 2.3211 |
| `struct_nested` | C | 0.9842 | 1.1072 | 1.0011 | 1.1289 | 1.1684 | 1.4021 |
| `struct_nested` | C++ | 0.9920 | 1.1327 | 1.0282 | 1.1283 | 1.1720 | 1.4615 |
| `struct_nested` | Rust | 0.9647 | 1.0294 | 0.9686 | 1.0649 | 1.0691 | 1.0808 |
| `struct_nested` | Java | 1.7845 | 2.4111 | 2.0063 | 2.6851 | 2.7035 | 3.0773 |
| `struct_nested` | Lua | 21.4467 | 24.9234 | 22.5238 | 23.8392 | 25.1875 | 38.2326 |
| `struct_nested` | Python | 73.8165 | 77.7061 | 75.1195 | 76.3463 | 79.4121 | 92.6439 |
| `struct_nested` | Go | 1.0519 | 1.2649 | 1.1193 | 1.2445 | 1.3234 | 1.6505 |
| `array_2d` | Lepus | 0.6498 | 0.7131 | 0.6545 | 0.6660 | 0.7357 | 0.9561 |
| `array_2d` | C | 0.5650 | 0.6001 | 0.5669 | 0.5879 | 0.6045 | 0.7600 |
| `array_2d` | C++ | 0.5650 | 0.6401 | 0.5724 | 0.6324 | 0.7111 | 0.7532 |
| `array_2d` | Rust | 0.5371 | 0.6317 | 0.5447 | 0.6300 | 0.6843 | 0.9306 |
| `array_2d` | Java | 0.8369 | 0.8825 | 0.8403 | 0.8435 | 0.8636 | 1.1354 |
| `array_2d` | Lua | 5.8711 | 6.5536 | 6.6287 | 6.6608 | 6.6906 | 7.5049 |
| `array_2d` | Python | 37.6316 | 39.5276 | 38.8325 | 39.5465 | 39.8569 | 44.6666 |
| `array_2d` | Go | 0.5658 | 0.6624 | 0.6156 | 0.6391 | 0.7264 | 0.7876 |
| `array_3d` | Lepus | 0.7345 | 0.7639 | 0.7373 | 0.7451 | 0.7631 | 0.9338 |
| `array_3d` | C | 0.6780 | 0.7007 | 0.6801 | 0.6834 | 0.6919 | 0.8254 |
| `array_3d` | C++ | 0.6780 | 0.7360 | 0.6839 | 0.6966 | 0.8059 | 0.8651 |
| `array_3d` | Rust | 0.6101 | 0.6224 | 0.6135 | 0.6164 | 0.6232 | 0.6968 |
| `array_3d` | Java | 1.1808 | 1.2360 | 1.1850 | 1.1896 | 1.2237 | 1.5569 |
| `array_3d` | Lua | 7.7315 | 8.6714 | 8.2317 | 8.5685 | 8.8305 | 11.4074 |
| `array_3d` | Python | 43.9655 | 46.0031 | 44.7020 | 45.4049 | 46.1910 | 52.5253 |
| `array_3d` | Go | 0.6360 | 0.7119 | 0.6373 | 0.6705 | 0.7983 | 0.8568 |
| `string_read` | Lepus | 2.4780 | 2.6155 | 2.5006 | 2.5197 | 2.5689 | 3.6950 |
| `string_read` | C | 0.5365 | 0.5662 | 0.5366 | 0.5416 | 0.5924 | 0.6708 |
| `string_read` | C++ | 0.5365 | 0.5501 | 0.5366 | 0.5400 | 0.5514 | 0.6758 |
| `string_read` | Rust | 0.5366 | 0.5585 | 0.5367 | 0.5368 | 0.5452 | 0.6776 |
| `string_read` | Java | 0.5368 | 0.5466 | 0.5370 | 0.5405 | 0.5515 | 0.6284 |
| `string_read` | Lua | 5.5464 | 6.0126 | 5.8135 | 5.8885 | 5.9669 | 7.4684 |
| `string_read` | Python | 24.5470 | 25.5516 | 24.7637 | 25.2156 | 25.8903 | 29.5073 |
| `string_read` | Go | 0.5650 | 0.5984 | 0.5651 | 0.5746 | 0.5855 | 0.7311 |
| `string_concat` | Lepus | 8.8775 | 14.8835 | 11.2344 | 13.0475 | 17.3508 | 26.8180 |
| `string_concat` | C | 2.7230 | 8.7336 | 4.2536 | 6.5745 | 12.1459 | 18.8106 |
| `string_concat` | C++ | 0.0991 | 0.1217 | 0.1114 | 0.1186 | 0.1351 | 0.1487 |
| `string_concat` | Rust | 0.0513 | 0.0696 | 0.0627 | 0.0670 | 0.0800 | 0.0885 |
| `string_concat` | Java | 5.6962 | 6.5426 | 6.0647 | 6.2898 | 7.0683 | 8.8602 |
| `string_concat` | Lua | 12.2532 | 24.6981 | 19.1539 | 26.6039 | 30.7335 | 36.1190 |
| `string_concat` | Python | 6.3308 | 7.0443 | 6.4746 | 6.8641 | 7.2387 | 9.2991 |
| `string_concat` | Go | 5.4501 | 9.0145 | 5.9483 | 6.8563 | 12.2725 | 21.2815 |
| `string_slice` | Lepus | 4.9401 | 5.3821 | 5.0903 | 5.2537 | 5.7166 | 6.1731 |
| `string_slice` | C | 0.0625 | 0.1640 | 0.0798 | 0.1941 | 0.2056 | 0.3023 |
| `string_slice` | C++ | 0.0673 | 0.1823 | 0.0712 | 0.2234 | 0.2732 | 0.2998 |
| `string_slice` | Rust | 0.0618 | 0.0831 | 0.0628 | 0.0694 | 0.1003 | 0.1621 |
| `string_slice` | Java | 0.0144 | 0.0172 | 0.0150 | 0.0172 | 0.0180 | 0.0292 |
| `string_slice` | Lua | 0.2863 | 0.3090 | 0.2932 | 0.2997 | 0.3190 | 0.3528 |
| `string_slice` | Python | 0.4068 | 0.4711 | 0.4213 | 0.4265 | 0.4401 | 0.7217 |
| `string_slice` | Go | 0.1069 | 0.1301 | 0.1216 | 0.1327 | 0.1374 | 0.1573 |
| `string_find` | Lepus | 2.9630 | 3.4990 | 3.1651 | 3.2133 | 3.5323 | 5.5483 |
| `string_find` | C | 0.0138 | 0.0172 | 0.0151 | 0.0172 | 0.0190 | 0.0220 |
| `string_find` | C++ | 0.0542 | 0.0691 | 0.0596 | 0.0603 | 0.0752 | 0.1374 |
| `string_find` | Rust | 0.0364 | 0.0430 | 0.0392 | 0.0397 | 0.0459 | 0.0692 |
| `string_find` | Java | 0.3476 | 0.3944 | 0.3792 | 0.3830 | 0.4160 | 0.4742 |
| `string_find` | Lua | 0.0695 | 0.0816 | 0.0717 | 0.0761 | 0.0874 | 0.1095 |
| `string_find` | Python | 0.0956 | 0.1010 | 0.0977 | 0.0990 | 0.1022 | 0.1254 |
| `string_find` | Go | 0.0593 | 0.0730 | 0.0651 | 0.0744 | 0.0815 | 0.0850 |

## Dispersion and distribution shape

Sample variance is s² = sum((x−mean)²)/(n−1), in ms²; sample SD is its square root, in ms. CV is 100s/mean, expressed as a percentage ([NIST definition](https://www.itl.nist.gov/div898/software/dataplot/refman2/auxillar/coefvari.htm)). Adjusted Fisher–Pearson sample skewness is G1 = n/((n−1)(n−2)) × sum(((x−mean)/s)³), a dimensionless value ([NIST skewness definitions](https://www.itl.nist.gov/div898/handbook/eda/section3/eda35b.htm)). When s=0, skewness is undefined rather than zero.

These are descriptive statistics of the 25 intervals, including differences between seeds and execution environments. Five intervals share each process; they are not 25 independent processes and are not used here for significance claims. Positive skewness describes a longer right tail, not a speed-based judgment. No observations are removed.

| Workload | Language | Sample variance (ms²) | SD (ms) | CV (%) | Skewness G1 |
| --- | --- | ---: | ---: | ---: | ---: |
| `int_chain` | Lepus | 0.00495252 | 0.0703742 | 7.83963 | 2.12774 |
| `int_chain` | C | 0.0233383 | 0.152769 | 3.50704 | 4.93637 |
| `int_chain` | C++ | 0.0273514 | 0.165383 | 3.76887 | 3.01228 |
| `int_chain` | Rust | 0.00678714 | 0.0823841 | 8.93025 | 1.23759 |
| `int_chain` | Java | 0.0339788 | 0.184333 | 3.65307 | 3.46358 |
| `int_chain` | Lua | 0.710978 | 0.843195 | 2.88826 | 2.8702 |
| `int_chain` | Python | 1657.74 | 40.7154 | 11.6641 | 2.36426 |
| `int_chain` | Go | 0.115356 | 0.33964 | 7.27748 | 1.00116 |
| `float_chain` | Lepus | 0.000329459 | 0.018151 | 1.39375 | 3.45918 |
| `float_chain` | C | 0.0106146 | 0.103027 | 7.56318 | 1.72092 |
| `float_chain` | C++ | 0.0166534 | 0.129048 | 9.47163 | 2.07803 |
| `float_chain` | Rust | 0.0119918 | 0.109507 | 8.23369 | 4.17704 |
| `float_chain` | Java | 0.012844 | 0.113332 | 8.38819 | 1.87843 |
| `float_chain` | Lua | 0.0873378 | 0.29553 | 5.12988 | 2.33284 |
| `float_chain` | Python | 13.2892 | 3.64543 | 11.104 | 2.0598 |
| `float_chain` | Go | 0.00518704 | 0.0720211 | 5.45213 | 4.66815 |
| `call` | Lepus | 4.59843e-06 | 0.0021444 | 0.988438 | 1.03036 |
| `call` | C | 0.00701045 | 0.0837285 | 9.1008 | 1.25726 |
| `call` | C++ | 0.00845298 | 0.0919401 | 9.95794 | 1.1641 |
| `call` | Rust | 0.00860693 | 0.0927735 | 8.17722 | 1.59907 |
| `call` | Java | 0.00882068 | 0.0939185 | 8.25019 | 1.54271 |
| `call` | Lua | 0.811224 | 0.90068 | 4.99588 | 1.10834 |
| `call` | Python | 137.211 | 11.7137 | 9.5656 | 1.929 |
| `call` | Go | 0.0115564 | 0.107501 | 9.32318 | 1.26448 |
| `array` | Lepus | 4.15531 | 2.03846 | 18.3722 | 2.19682 |
| `array` | C | 0.418918 | 0.647239 | 8.8352 | 1.94509 |
| `array` | C++ | 0.971319 | 0.985555 | 12.96 | 1.25061 |
| `array` | Rust | 0.0144117 | 0.120049 | 1.91289 | 2.69583 |
| `array` | Java | 0.0467828 | 0.216293 | 3.46883 | 3.74705 |
| `array` | Lua | 11.1416 | 3.3379 | 8.06371 | -0.919426 |
| `array` | Python | 1008.58 | 31.7582 | 14.5569 | 2.19497 |
| `array` | Go | 0.427409 | 0.653765 | 8.78165 | 3.23871 |
| `array_rw` | Lepus | 3.94571 | 1.98638 | 7.82541 | 3.4521 |
| `array_rw` | C | 0.0302205 | 0.17384 | 1.99385 | 2.81602 |
| `array_rw` | C++ | 0.0241048 | 0.155257 | 1.7851 | 4.56643 |
| `array_rw` | Rust | 0.000319991 | 0.0178883 | 0.256292 | 0.773198 |
| `array_rw` | Java | 0.0885925 | 0.297645 | 3.37369 | 2.48737 |
| `array_rw` | Lua | 16.9227 | 4.11372 | 5.87525 | -0.0021934 |
| `array_rw` | Python | 3294.47 | 57.3975 | 10.921 | 4.37482 |
| `array_rw` | Go | 0.274952 | 0.524359 | 5.76297 | 0.735569 |
| `list_growth` | Lepus | 7.39164 | 2.71876 | 35.0816 | 0.752074 |
| `list_growth` | C | 0.0145298 | 0.120539 | 6.70279 | 0.167976 |
| `list_growth` | C++ | 0.0841787 | 0.290136 | 13.4256 | 2.09487 |
| `list_growth` | Rust | 0.0497636 | 0.223078 | 11.6523 | 1.32455 |
| `list_growth` | Java | 1.11621 | 1.05651 | 31.1829 | 1.73712 |
| `list_growth` | Lua | 2.73818 | 1.65474 | 12.9467 | 1.26487 |
| `list_growth` | Python | 2.30482 | 1.51816 | 2.64804 | 1.21313 |
| `list_growth` | Go | 3.01182 | 1.73546 | 53.424 | 4.35882 |
| `map_growth` | Lepus | 8.14682 | 2.85426 | 4.81084 | 2.08838 |
| `map_growth` | C | 0.0100426 | 0.100213 | 5.76154 | 2.89813 |
| `map_growth` | C++ | 0.165386 | 0.406677 | 22.2872 | 3.85277 |
| `map_growth` | Rust | 0.719793 | 0.848406 | 38.2327 | 4.96349 |
| `map_growth` | Java | 0.26782 | 0.517514 | 13.6015 | 2.22284 |
| `map_growth` | Lua | 0.83015 | 0.911126 | 20.2326 | 3.47568 |
| `map_growth` | Python | 0.940193 | 0.969635 | 4.6989 | 1.89891 |
| `map_growth` | Go | 0.407745 | 0.638549 | 23.1649 | 2.41378 |
| `set_growth` | Lepus | 16.0821 | 4.01024 | 6.33279 | 1.06054 |
| `set_growth` | C | 0.0497147 | 0.222968 | 12.5718 | 2.39182 |
| `set_growth` | C++ | 0.172543 | 0.415382 | 22.3476 | 1.91317 |
| `set_growth` | Rust | 0.0938199 | 0.3063 | 18.147 | 3.0767 |
| `set_growth` | Java | 0.117103 | 0.342202 | 9.6576 | 0.641983 |
| `set_growth` | Lua | 0.134722 | 0.367045 | 8.76782 | 1.97089 |
| `set_growth` | Python | 0.530878 | 0.728614 | 4.08655 | 0.724601 |
| `set_growth` | Go | 0.178921 | 0.422991 | 16.6026 | 0.603343 |
| `enum_payload` | Lepus | 0.0321518 | 0.179309 | 18.9785 | 2.5632 |
| `enum_payload` | C | 0.00154658 | 0.0393266 | 4.5165 | 4.46307 |
| `enum_payload` | C++ | 0.00120538 | 0.0347186 | 4.23607 | 4.60243 |
| `enum_payload` | Rust | 0.00483485 | 0.0695331 | 5.55256 | 2.11078 |
| `enum_payload` | Java | 0.00529222 | 0.0727476 | 8.21161 | 1.71961 |
| `enum_payload` | Lua | 0.712632 | 0.844175 | 8.23761 | 0.952566 |
| `enum_payload` | Python | 22.3564 | 4.72825 | 8.55169 | 1.95323 |
| `enum_payload` | Go | 0.00540472 | 0.0735168 | 10.1525 | 0.353547 |
| `interface_mono` | Lepus | 0.376722 | 0.613777 | 5.59718 | 1.64916 |
| `interface_mono` | C | 0.00114788 | 0.0338804 | 10.6075 | 0.108318 |
| `interface_mono` | C++ | 0.000580825 | 0.0241003 | 8.12816 | 2.20542 |
| `interface_mono` | Rust | 1.02027e-06 | 0.00101008 | 1.77794 | 3.85045 |
| `interface_mono` | Java | 0.000398062 | 0.0199515 | 10.4051 | -0.110984 |
| `interface_mono` | Lua | 0.0840961 | 0.289993 | 4.52857 | 2.22285 |
| `interface_mono` | Python | 3.36331 | 1.83393 | 5.43106 | 0.622534 |
| `interface_mono` | Go | 0.00214352 | 0.0462981 | 11.1498 | 0.750543 |
| `interface_poly` | Lepus | 46.4013 | 6.81185 | 6.8837 | 1.74765 |
| `interface_poly` | C | 9.01669e-05 | 0.00949563 | 2.29356 | 3.47564 |
| `interface_poly` | C++ | 0.00403927 | 0.0635553 | 18.776 | 0.697115 |
| `interface_poly` | Rust | 0.00424326 | 0.0651403 | 14.3245 | 0.430247 |
| `interface_poly` | Java | 0.029441 | 0.171584 | 8.09978 | 1.24361 |
| `interface_poly` | Lua | 0.26398 | 0.513789 | 5.16218 | 1.50735 |
| `interface_poly` | Python | 25.8017 | 5.07954 | 11.2065 | 1.29797 |
| `interface_poly` | Go | 0.00564015 | 0.0751009 | 12.5602 | 0.41862 |
| `struct_nested` | Lepus | 0.0302067 | 0.173801 | 9.34158 | 1.69027 |
| `struct_nested` | C | 0.0108149 | 0.103995 | 9.39226 | 0.821086 |
| `struct_nested` | C++ | 0.014219 | 0.119244 | 10.5275 | 1.1133 |
| `struct_nested` | Rust | 0.00255109 | 0.0505083 | 4.9066 | -0.421052 |
| `struct_nested` | Java | 0.185966 | 0.431238 | 17.8855 | -0.376123 |
| `struct_nested` | Lua | 15.0907 | 3.88468 | 15.5865 | 2.11721 |
| `struct_nested` | Python | 19.0457 | 4.36414 | 5.61621 | 2.22909 |
| `struct_nested` | Go | 0.0301616 | 0.173671 | 13.7297 | 0.726637 |
| `array_2d` | Lepus | 0.00771023 | 0.0878079 | 12.3132 | 1.48763 |
| `array_2d` | C | 0.00227124 | 0.0476575 | 7.94122 | 2.20957 |
| `array_2d` | C++ | 0.00523393 | 0.0723459 | 11.3021 | 0.250247 |
| `array_2d` | Rust | 0.00919731 | 0.0959026 | 15.1822 | 1.21792 |
| `array_2d` | Java | 0.00744899 | 0.0863075 | 9.78007 | 2.10315 |
| `array_2d` | Lua | 0.144825 | 0.380559 | 5.80689 | -0.253129 |
| `array_2d` | Python | 1.97231 | 1.40439 | 3.55294 | 2.04729 |
| `array_2d` | Go | 0.00478575 | 0.0691791 | 10.4443 | 0.329791 |
| `array_3d` | Lepus | 0.00227796 | 0.047728 | 6.24768 | 2.6218 |
| `array_3d` | C | 0.00182831 | 0.0427588 | 6.10264 | 2.24882 |
| `array_3d` | C++ | 0.00486859 | 0.0697753 | 9.48064 | 0.922579 |
| `array_3d` | Rust | 0.000333354 | 0.018258 | 2.93367 | 3.22744 |
| `array_3d` | Java | 0.0116975 | 0.108155 | 8.75031 | 2.40083 |
| `array_3d` | Lua | 0.53558 | 0.731833 | 8.43962 | 2.2988 |
| `array_3d` | Python | 4.93927 | 2.22245 | 4.83108 | 2.11449 |
| `array_3d` | Go | 0.00652679 | 0.0807885 | 11.3485 | 0.450873 |
| `string_read` | Lepus | 0.0840953 | 0.289992 | 11.0875 | 3.25563 |
| `string_read` | C | 0.00208112 | 0.0456193 | 8.05749 | 1.33365 |
| `string_read` | C++ | 0.000830496 | 0.0288183 | 5.23882 | 3.80479 |
| `string_read` | Rust | 0.00212667 | 0.0461158 | 8.25697 | 2.11001 |
| `string_read` | Java | 0.000339183 | 0.0184169 | 3.36909 | 3.91389 |
| `string_read` | Lua | 0.173972 | 0.4171 | 6.93711 | 2.50861 |
| `string_read` | Python | 1.20652 | 1.09842 | 4.29882 | 2.17082 |
| `string_read` | Go | 0.00304573 | 0.0551882 | 9.22194 | 1.67528 |
| `string_concat` | Lepus | 26.9708 | 5.19335 | 34.8933 | 0.943162 |
| `string_concat` | C | 30.4833 | 5.52117 | 63.2172 | 0.692643 |
| `string_concat` | C++ | 0.000252909 | 0.0159031 | 13.0632 | 0.25634 |
| `string_concat` | Rust | 0.000128828 | 0.0113503 | 16.3032 | 0.116125 |
| `string_concat` | Java | 0.574065 | 0.757671 | 11.5805 | 1.32877 |
| `string_concat` | Lua | 58.2607 | 7.63287 | 30.9047 | -0.360997 |
| `string_concat` | Python | 0.591941 | 0.769377 | 10.9219 | 1.61969 |
| `string_concat` | Go | 18.395 | 4.28894 | 47.5785 | 1.33724 |
| `string_slice` | Lepus | 0.13715 | 0.370337 | 6.88086 | 0.852335 |
| `string_slice` | C | 0.00664369 | 0.0815088 | 49.6957 | -0.00312329 |
| `string_slice` | C++ | 0.00878333 | 0.0937194 | 51.4038 | -0.235135 |
| `string_slice` | Rust | 0.000731434 | 0.027045 | 32.5405 | 1.46118 |
| `string_slice` | Java | 9.17293e-06 | 0.00302869 | 17.6373 | 2.72478 |
| `string_slice` | Lua | 0.000448194 | 0.0211706 | 6.85079 | 0.909435 |
| `string_slice` | Python | 0.00946686 | 0.0972978 | 20.6552 | 1.76156 |
| `string_slice` | Go | 0.000175216 | 0.0132369 | 10.1719 | -0.406169 |
| `string_find` | Lepus | 0.467808 | 0.683965 | 19.5475 | 2.12706 |
| `string_find` | C | 5.73417e-06 | 0.00239461 | 13.9222 | 0.415314 |
| `string_find` | C++ | 0.000417464 | 0.0204319 | 29.5755 | 2.27052 |
| `string_find` | Rust | 5.37194e-05 | 0.00732935 | 17.0498 | 2.01535 |
| `string_find` | Java | 0.00131658 | 0.0362847 | 9.19942 | 0.734192 |
| `string_find` | Lua | 0.000121919 | 0.0110417 | 13.5354 | 0.831082 |
| `string_find` | Python | 3.63007e-05 | 0.00602501 | 5.96323 | 2.92292 |
| `string_find` | Go | 8.48206e-05 | 0.00920981 | 12.6189 | -0.239363 |

## Reproduction and raw records

See [the driver](../../benchmarks/current/run.py), [implementations and method](../../benchmarks/current/README.md), [identity audit](../../benchmarks/current/audit.py), and [report generator](../../benchmarks/current/report.py). Raw JSON contains commands, output, seed results, all samples, summaries, order and SHA-256 identities. The exported data archive also retains measured artifacts and source snapshots.

```powershell
python benchmarks/current/run.py --lepus PATH --compiler-source-commit COMMIT --out build/validation/performance-current/run --build-only
python benchmarks/current/audit.py build/validation/performance-current/run/results.json
python benchmarks/current/run.py --lepus PATH --compiler-source-commit COMMIT --out build/validation/performance-current/run --measure-only
python benchmarks/current/audit.py build/validation/performance-current/run/results.json --after
python benchmarks/current/report.py build/validation/performance-current/run/results.json --write-docs
```
