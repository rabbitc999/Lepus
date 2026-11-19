"""有界并行门禁：仅并行明确登记的独立检查，其余任务作为串行屏障。"""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import json
import os
from pathlib import Path
import subprocess
import time

# 已登记测试使用各自的产物路径；网络、打包、生成器和未知测试保持串行。
PARALLEL_TESTS = frozenset("""
bigint_arithmetic bigfloat_math lepus_cas_math lepus_compiler_safety
lepus_print_streams lepus_resource_lifetime lepus_type_memory lepus_lsp_closure_types lepus_parse_selfhost
lepus_array_parameters lepus_string_literals lepus_opcode_declarations
lepus_range_loop_values lepus_opcode_resources lepus_opcode_visibility
opcode_core_isolation lepus_collection_glue lepus_map_pair_iterator
lepus_generic_constructors lepus_borrow_flow lepus_generic_closures
lepus_borrow_move_defer lepus_borrow_contract_stress lepus_nested_result_stress lepus_syntax_semantic_stress
lepus_percent_preprocessing_removed lepus_entry_signature lepus_function_types lepus_analysis_diagnostics
lepus_generic_monomorphization lepus_generic_scope
lepus_language_move lepus_tuple_returns lepus_thread_wait
macro_foundations lepus_macro_editor lepus_ast_macros lepus_macro_branches lepus_macro_groups lepus_macro_generics
lepus_macro_parameters lepus_macro_categories lepus_macro_result_inference lepus_macro_variadics
lepus_macro_patterns lepus_macro_declarations lepus_macro_members lepus_macro_implement lepus_value_derive lepus_debug_derive lepus_json_derive lepus_declaration_sugars lepus_macro_metadata
lepus_derive_contracts
lepus_macro_type_metadata lepus_macro_member_contract lepus_macro_provenance lepus_macro_controls lepus_macro_control_flow lepus_loop_labels lepus_macro_diagnostics
lepus_macro_preview lepus_macro_control_preview lepus_macro_effects_preview lepus_lsp_macro_completion lepus_lsp_macro_highlighting
lepus_reflection_features lepus_reflection_metadata lepus_reflection_declarations lepus_reflection_queries
lepus_reflection_calls lepus_reflection_e2e lepus_reflection_students lepus_reflection_serialization lepus_annotations
check_doc_links export_lepus_ebnf library_reference check_keywords
check_std_naming embed_manual changelog
""".split())


def positive_jobs(value):
    """解析正整数并拒绝零、负数及非整数工作数。"""
    try:
        jobs = int(value)
    except (TypeError, ValueError):
        raise argparse.ArgumentTypeError("并行工作数必须是正整数") from None
    if jobs < 1:
        raise argparse.ArgumentTypeError("并行工作数必须是正整数")
    return jobs


def default_jobs():
    """默认最多四个测试工作线程，环境变量允许显式覆盖。"""
    return positive_jobs(os.environ.get("LEPUS_GATE_JOBS", min(4, os.cpu_count() or 1)))


def independent(command):
    """只允许审查过的独立 Python 与原生检查进入并行组。"""
    return any(Path(str(part)).suffix == ".py" and Path(str(part)).stem in PARALLEL_TESTS
               for part in command[1:2])


def run_tasks(tasks, output, jobs, cwd, env=None, metadata=None, index_offset=0):
    """按屏障顺序运行任务，协调线程统一发布日志与有序结果。"""
    output = Path(output)
    output.mkdir(parents=True, exist_ok=True)
    results = [None] * len(tasks)
    started = time.monotonic()
    environment = dict(os.environ, PYTHONUNBUFFERED="1", PYTHONUTF8="1", PYTHONIOENCODING="utf-8", LEPUS_QUIET="1")
    environment.update(env or {})
    print(f"门禁工作线程：{jobs}；日志：{output}", flush=True)

    def execute(index):
        """执行单个子进程，独立保存完整输出并将启动异常记录为失败。"""
        task = tasks[index]
        path = output / f"{index + index_offset + 1:03d}.log"
        begin = time.monotonic()
        with path.open("wb") as log:
            try:
                code = subprocess.run(task["command"], cwd=cwd, env=environment,
                                      stdout=log, stderr=subprocess.STDOUT).returncode
            except OSError as error:
                log.write(str(error).encode("utf-8", errors="replace"))
                code = 127
        return dict(task, exit_code=code, seconds=round(time.monotonic() - begin, 3),
                    log=str(path), index=index + index_offset + 1)

    def publish(index, result):
        """仅由协调线程更新结果，避免多个工作线程覆盖 JSON 或交错输出。"""
        results[index] = result
        state = "跳过" if result["exit_code"] is None else ("通过" if result["exit_code"] == 0 else "失败")
        print(f"[{index + index_offset + 1:3d}] {state} {result['title']}（{result['seconds']}s）", flush=True)
        if result["exit_code"] not in (0, None):
            print(f"      完整输出：{result['log']}", flush=True)
        document = dict(metadata or {}, jobs=jobs, seconds=round(time.monotonic() - started, 3),
                        results=[item for item in results if item is not None])
        temporary = output / "results.json.tmp"
        temporary.write_text(json.dumps(document, ensure_ascii=False, indent=2), encoding="utf-8")
        temporary.replace(output / "results.json")

    def can_parallel(task):
        """必需生成或构建始终形成屏障，其余检查按显式登记并行。"""
        return not task.get("required") and task.get("parallel", independent(task["command"]))

    index = 0
    blocked = False
    with ThreadPoolExecutor(max_workers=jobs) as executor:
        while index < len(tasks):
            if blocked:
                publish(index, dict(tasks[index], exit_code=None, seconds=0, index=index + index_offset + 1,
                                    reason="生成或构建失败"))
                index += 1
                continue
            if not can_parallel(tasks[index]):
                print(f"[{index + index_offset + 1:3d}] 开始 {tasks[index]['title']}", flush=True)
                result = execute(index)
                publish(index, result)
                blocked = bool(tasks[index].get("required") and result["exit_code"] != 0)
                index += 1
                continue
            pending = {}
            while index < len(tasks) and can_parallel(tasks[index]):
                print(f"[{index + index_offset + 1:3d}] 排队 {tasks[index]['title']}", flush=True)
                pending[executor.submit(execute, index)] = index
                index += 1
            for future in as_completed(pending):
                publish(pending[future], future.result())
    failed = sum(item["exit_code"] not in (0, None) for item in results)
    skipped = sum(item["exit_code"] is None for item in results)
    print(f"步骤：{len(results)}，成功 {len(results) - failed - skipped}，失败 {failed}，跳过 {skipped}；"
          f"总耗时 {time.monotonic() - started:.2f}s", flush=True)
    return results


def main():
    """追加 shell 提供的无损参数，或执行已登记的完整门禁清单。"""
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="mode", required=True)
    append = sub.add_parser("append")
    append.add_argument("--required", action="store_true", help="失败时阻断后续检查的生成或构建任务")
    append.add_argument("manifest", type=Path)
    append.add_argument("title")
    append.add_argument("command", nargs=argparse.REMAINDER)
    run = sub.add_parser("run")
    run.add_argument("manifest", type=Path)
    run.add_argument("--output", type=Path, required=True)
    run.add_argument("--jobs", type=positive_jobs, default=None)
    run.add_argument("--cleanup-manifest", action="store_true", help="读入成功后删除调用方创建的临时清单")
    args = parser.parse_args()
    if args.mode == "append":
        if not args.command:
            parser.error("门禁命令不能为空")
        task = dict(title=args.title, command=args.command,
                    required=args.required)
        with args.manifest.open("a", encoding="utf-8") as stream:
            stream.write(json.dumps(task, ensure_ascii=False) + "\n")
        return 0
    try:
        jobs = args.jobs if args.jobs is not None else default_jobs()
    except argparse.ArgumentTypeError as error:
        parser.error(str(error))
    tasks = [json.loads(line) for line in args.manifest.read_text(encoding="utf-8").splitlines()]
    if args.cleanup_manifest:
        args.manifest.unlink()
    results = run_tasks(tasks, args.output, jobs, Path.cwd())
    return int(any(item["exit_code"] != 0 for item in results))


if __name__ == "__main__":
    raise SystemExit(main())
