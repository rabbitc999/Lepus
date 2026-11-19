/**
 * @file lepus_json_internal.h
 * @brief JSON/TOML C 内核共享的独占文档，仅供服务内部访问。
 */
#ifndef LEPUS_JSON_INTERNAL_H
#define LEPUS_JSON_INTERNAL_H
#include "lepus_json.h"
#include "yyjson.h"
/** @brief C 文档及其根节点与有界诊断，不向语言层暴露实现。 */
struct LepusJson
{
    yyjson_mut_doc *document;
    yyjson_mut_val *root;
    char error[256];
};
#endif
