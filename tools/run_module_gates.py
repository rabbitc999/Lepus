"""按模块执行门禁，并以可复现随机种子抽样复测已通过的门禁。"""
from compiler_path import compiler_path
import argparse
import json
import os
from pathlib import Path
import random
import sys
from project_temp import tempfile
import time

from parallel_gates import default_jobs, positive_jobs, run_tasks

ROOT = Path(__file__).resolve().parents[1]
MODULES = {
    "moves": ["lepus_language_move", "lepus_resource_lifetime", "lepus_native_callbacks", "lepus_backend_ownership"],
    "debug": ["lepus_native_debug", "lepus_debug_optimization"],
    "debug_remote": ["lepus_native_remote_debug"],
    "editor_host": ["lepus_vscode_extension_host"],
    "libraries": ["lepus_digest", "lepus_library_package", "lepus_library_bitcode_header", "lepus_generator_assets"],
    "defaults": ["lepus_default_arguments", "lepus_default_ambiguity", "lepus_default_module"],
    "interfaces": ["lepus_interface_values", "lepus_interface_binding_move", "lepus_copy_interface", "lepus_interface_copy_scalar"],
    "abi": ["lepus_link_abi", "lepus_abi_values", "lepus_abi_contracts", "lepus_c_api_contract"],
    "toolchain": ["lepus_link_abi", "lepus_linker", "lepus_assembler", "lepus_aot_assembly", "lepus_package", "lepus_aot_arguments", "lepus_aot_project_config", "lepus_default_outputs", "lepus_completion_notice"],
    "warnings": ["lepus_warnings", "lepus_module_naming_pragma", "lepus_analysis_diagnostics", "lepus_match_overlap", "lepus_lsp_naming_scopes"],
    "stdlib": ["lepus_standard_unit", "lepus_precompiled", "lepus_generic_scope", "lepus_macro_generics"]
              + (["lepus_precompiled_readonly"] if os.name == "nt" else []),
    "sql": ["lepus_sqlite", "lepus_sql_connections", "lepus_sql_backends", "lepus_reflection_serialization"],
    "net": ["lepus_net_c_api", "lepus_opcode_std_storage"],
    "image": ["lepus_image", "lepus_opcode_std_storage"],
    "collections": ["lepus_c_runtime_storage", "lepus_collection_glue", "lepus_numeric_list_storage", "lepus_numeric_list_append", "lepus_map_pair_iterator"],
    "scalar": ["lepus_scalar_c_api", "lepus_linalg_column", "lepus_linalg_solve"],
    "linalg": ["lepus_linalg_column", "lepus_linalg_solve", "lepus_linalg_decomposition"],
    "file": ["lepus_file_c_api", "lepus_filesystem_c_api", "lepus_opcode_resources", "lepus_print_streams"],
    "filesystem": ["lepus_filesystem_c_api"],
    "regex": ["lepus_regex_c_api"],
    "binary": ["lepus_binary"],
    "signal": ["lepus_signal"],
    "process": ["lepus_process_c_api"],
    "compress": ["lepus_compress_c_api"],
    "concurrency": ["lepus_shared_selfhost", "lepus_thread_wait", "lepus_thread_diagnostics"],
    "serialization": ["lepus_serialization_selfhost", "lepus_reflection_serialization", "lepus_structured_data", "lepus_json_derive"],
    "parse": ["lepus_parse_selfhost", "lepus_precompiled", "lepus_glue_edges"],
    "cas": ["lepus_cas_c_api", "lepus_cas_math", "lepus_big_number_c_api", "lepus_thread_wait"],
    "big_number": ["lepus_big_number_c_api", "lepus_cas_math", "lepus_reflection_serialization"],
    "demo": ["lepus_demo_c_api", "lepus_reference_values", "lepus_reference_combinations"],
    "c_api": ["lepus_native_marshal", "lepus_c_api_contract", "lepus_generator_assets", "lepus_c_api_export", "lepus_c_api_registry", "lepus_native_callbacks", "lepus_abi_contracts", "lepus_abi_directives", "lepus_abi_values", "lepus_glue_edges"],
    "macros": ["lepus_ast_macros", "lepus_macro_parameters", "lepus_macro_categories",
               "lepus_macro_branches", "lepus_macro_groups", "lepus_macro_generics",
               "lepus_macro_result_inference", "lepus_macro_variadics", "lepus_macro_patterns",
               "lepus_macro_declarations", "lepus_macro_members", "lepus_macro_implement", "lepus_value_derive", "lepus_debug_derive", "lepus_json_derive", "lepus_declaration_sugars", "lepus_macro_metadata",
               "lepus_derive_contracts", "lepus_macro_type_metadata", "lepus_macro_member_contract", "lepus_macro_provenance", "lepus_macro_controls", "lepus_macro_control_flow", "lepus_loop_labels", "lepus_macro_else_if", "lepus_macro_type_patterns",
               "lepus_macro_diagnostics", "lepus_macro_preview", "lepus_macro_control_preview", "lepus_macro_effects_preview", "lepus_macro_editor",
               "lepus_lsp_macro_completion", "lepus_lsp_macro_highlighting", "lepus_lsp_generated_symbols"],
    "language": ["lepus_percent_preprocessing_removed", "lepus_entry_signature", "lepus_function_types", "lepus_lambdas", "lepus_ast_macros", "lepus_macro_branches", "lepus_macro_groups", "lepus_macro_generics"],
    "borrow": ["lepus_borrow_flow", "lepus_analysis_diagnostics", "lepus_reference_values"],
    "memory": ["lepus_type_memory", "lepus_resource_lifetime"],
    "generics": ["lepus_generic_monomorphization", "lepus_generic_scope", "lepus_language_move", "lepus_tuple_returns"],
    "reflection": ["lepus_json_derive", "lepus_reflection_features", "lepus_reflection_metadata", "lepus_reflection_declarations",
                   "lepus_reflection_queries", "lepus_reflection_calls", "lepus_reflection_e2e", "lepus_reflection_students",
                   "lepus_reflection_serialization", "lepus_annotations"],
    "samples": [],
    "tools": ["lepus_tool", "lepus_package", "lepus_map_pair_iterator", "lepus_generic_constructors"],
    "lsp": ["lepus_lsp_actions", "lepus_lsp_editor", "lepus_lsp_scheduler", "lepus_lsp_pipeline", "lepus_lsp_snapshots", "lepus_lsp_closure_types", "lepus_lsp_risks", "lepus_lsp_receiver_types",
            "lepus_lsp_overloads", "lepus_lsp_naming_scopes", "lepus_lsp_symbols",
            "lepus_lsp_lambda_capture", "lepus_lsp_semantic_features",
            "lepus_lsp_macro_completion", "lepus_lsp_macro_highlighting", "lepus_lsp_generated_symbols",
            "lepus_native_lsp_parity", "lepus_lsp_type_aliases", "lepus_lsp_fn_named_highlighting", "lepus_lsp_interpolation", "lepus_lsp_recovery", "lepus_lsp_performance", "lepus_lsp_highlight_speed"],
}


def main():
    """运行指定模块，逐项保存日志与结果，失败时返回非零状态。"""
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("modules", nargs="+", choices=[*MODULES, "docs"])
    parser.add_argument("--lepus", type=Path, default=compiler_path())
    parser.add_argument("--random-recheck", type=int, default=0, help="从已通过门禁抽样复测的次数")
    parser.add_argument("--seed", type=int, default=20261013)
    parser.add_argument("--jobs", type=positive_jobs, default=None, help="并行工作数，默认最多四个；1 为串行")
    parser.add_argument("--build-root", type=Path, help="需要链接前端库的模块门禁使用的隔离构建目录")
    parser.add_argument("--cxx", help="模块 C++ 单元使用的编译器")
    parser.add_argument("--sysroot", type=Path, help="模块 C++ 单元使用的工具链 sysroot")
    args = parser.parse_args()
    try:
        jobs = args.jobs if args.jobs is not None else default_jobs()
    except argparse.ArgumentTypeError as error:
        parser.error(str(error))
    if args.random_recheck < 0:
        parser.error("随机复测次数必须非负")
    base = ROOT / "tmp" / "module-gates"
    base.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix=time.strftime("%Y%m%d-%H%M%S-"), dir=base))
    commands = {}
    for module in args.modules:
        if module == "samples":
            commands["samples"] = ["sh", "check.sh"]
        elif module == "docs":
            for script, flags in (("check_doc_links", []), ("export_lepus_ebnf", ["--check"]),
                                  ("library_reference", ["--check"])):
                commands[script] = [sys.executable, str(ROOT / "tools" / (script + ".py")), *flags]
            commands["lepus_book_library"] = [sys.executable, str(ROOT / "tests/lepus_book_library.py"),
                                              "--lepus", str(args.lepus.resolve())]
        else:
            for script in MODULES[module]:
                commands[script] = [sys.executable, str(ROOT / "tests" / (script + ".py")),
                                    "--lepus", str(args.lepus.resolve())]
                if module == "lsp" and script == "lepus_lsp_closure_types":
                    commands[script].append("--lsp-only")
                if script == "lepus_lsp_scheduler":
                    if args.cxx: commands[script] += ["--cxx", args.cxx]
                    if args.sysroot: commands[script] += ["--sysroot", str(args.sysroot.resolve())]
                if module == "macros" and script == "lepus_ast_macros":
                    commands[script].append("--default")
                if script in ("lepus_annotations", "lepus_c_api_registry", "lepus_numeric_list_storage", "lepus_generator_assets", "lepus_library_bitcode_header", "lepus_standard_unit", "lepus_backend_ownership", "lepus_function_types"):
                    if args.build_root: commands[script] += ["--build-root", str(args.build_root.resolve())]
                    if args.cxx: commands[script] += ["--cxx", args.cxx]
                    if args.sysroot: commands[script] += ["--sysroot", str(args.sysroot.resolve())]
            if module == "lsp":
                for script in ("lspCancellation", "semanticProviders", "editorIntegration", "macroHighlighting"):
                    commands[script] = ["node", str(ROOT / "vscode" / "test" / (script + ".js"))]
    environment = dict(DEMO=args.lepus.resolve().as_posix(), LEPUS_COMPILER=str(args.lepus.resolve()),
                       LEPUS_TEST_COMPILER=str(args.lepus.resolve()))
    tasks = [dict(title=name, name=name, command=command, recheck=False)
             for name, command in commands.items()]
    results = run_tasks(tasks, output, jobs, ROOT, environment, dict(seed=args.seed))
    # 按登记顺序抽样，不让完成顺序改变相同种子的复测选择。
    passed = [item["name"] for item in results if item["exit_code"] == 0]
    rng = random.Random(args.seed)
    rechecks = []
    for _ in range(args.random_recheck):
        if not passed:
            break
        name = rng.choice(passed)
        # 同名脚本可能使用固定工作目录，复测必须串行。
        rechecks.append(dict(title="随机复测 " + name, name=name, command=commands[name],
                             recheck=True, parallel=False))
    if rechecks:
        repeated = run_tasks(rechecks, output / "rechecks", 1, ROOT, environment, dict(seed=args.seed),
                             index_offset=len(results))
        results.extend(repeated)
        (output / "results.json").write_text(json.dumps(dict(seed=args.seed, jobs=jobs, results=results),
                                                       ensure_ascii=False, indent=2), encoding="utf-8")
    return int(any(result["exit_code"] != 0 for result in results))


if __name__ == "__main__":
    sys.exit(main())
