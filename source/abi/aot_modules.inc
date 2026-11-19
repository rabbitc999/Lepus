/* 模块名与 C 入口表提供函数的统一清单。每项可包含多张入口表。
 * 标准库集中注册排除 demo，BUILD 选择保留 demo；裁剪宏沿用既有约定。 */
LEPUS_BUILD_MODULE(print, abi_print_records)
LEPUS_BUILD_MODULE(collection, abi_collection_records)
LEPUS_BUILD_MODULE(string, abi_string_records)
LEPUS_BUILD_MODULE(big_number, abi_big_number_records)
LEPUS_BUILD_MODULE(cas, abi_cas_records)
LEPUS_BUILD_MODULE(file, abi_file_records)
#ifndef GLUE_NO_STD_FILESYSTEM
LEPUS_BUILD_MODULE(filesystem, abi_filesystem_records)
#endif
#ifndef GLUE_NO_STD_JSON
LEPUS_BUILD_MODULE(json, abi_json_records)
LEPUS_BUILD_MODULE(serialization, abi_serialization_records)
#endif
#ifndef GLUE_NO_STD_TOML
LEPUS_BUILD_MODULE(toml, abi_toml_records)
#endif
#ifndef GLUE_NO_STD_COMPRESS
LEPUS_BUILD_MODULE(compress, abi_compress_records)
#endif
#ifndef GLUE_NO_STD_MATH
LEPUS_BUILD_MODULE(math,
                 abi_math_records, abi_clock_records)
#endif
#ifndef GLUE_NO_STD_RANDOM
LEPUS_BUILD_MODULE(random, abi_random_records)
#endif
#ifndef GLUE_NO_STD_REGEX
LEPUS_BUILD_MODULE(regex, abi_regex_records)
#endif
#ifndef GLUE_NO_STD_LINALG
LEPUS_BUILD_MODULE(linalg, abi_scalar_records, abi_linalg_records)
#endif
#ifndef GLUE_NO_STD_PARSE
LEPUS_BUILD_MODULE(parse, abi_parse_records)
#endif
#ifndef GLUE_NO_STD_PROCESS
LEPUS_BUILD_MODULE(process, abi_process_records)
#endif
#ifndef GLUE_NO_STD_SQL
LEPUS_BUILD_MODULE(sql, abi_sql_records, abi_sql_ipage_records)
#endif
#ifndef GLUE_NO_STD_IMAGE
LEPUS_BUILD_MODULE(image, abi_image_records)
#endif
#ifndef GLUE_NO_STD_NET
LEPUS_BUILD_MODULE(net, abi_net_records)
#endif
#ifndef LEPUS_STANDARD_ONLY
LEPUS_BUILD_MODULE(demo, abi_demo_records)
#endif
