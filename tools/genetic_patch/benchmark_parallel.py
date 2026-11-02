"""Compare candidate evaluation backends without changing the GA search path.

All backends evaluate the same patches in input order. Process workers initialize
their own renderer/objective once, then return only three scores per candidate.
Threads share a renderer and read-only target features, with candidate-local audio.
"""
import argparse
from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor
import hashlib
import json
import multiprocessing
import os
from pathlib import Path
import platform
import statistics
import time

import numpy as np
import genetic_patch as gp

_context = None


def init_worker(renderer_path, target, note, off):
    global _context
    renderer = gp.Renderer(renderer_path)
    objective = gp.Objective(target)
    for group in [objective.target] + objective.reference:
        for array in group:
            array.flags.writeable = False
    _context = renderer, objective, note, len(target), off


def evaluate(genes):
    renderer, objective, note, count, off = _context
    return objective.scores(renderer.render(gp.decode(genes), note, count, off))


def render_digest(genes):
    renderer, _, note, count, off = _context
    audio = renderer.render(gp.decode(genes), note, count, off)
    return hashlib.sha256(audio.tobytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("target")
    parser.add_argument("--renderer", default=str(gp.HERE / "build/Release/s3hs_patch_renderer.dll"))
    parser.add_argument("--note", type=int, required=True)
    parser.add_argument("--note-off", type=float)
    parser.add_argument("--candidates", type=int, default=24)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--thread-workers", default="2,4,8,16")
    parser.add_argument("--process-workers", default="4,8", help="empty string disables processes")
    parser.add_argument("--seed", type=int, default=11)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    if args.candidates < 1 or args.repeats < 1:
        parser.error("candidates and repeats must be positive")
    configurations = [("serial", 1)]
    for backend, counts in (("threads", args.thread_workers), ("processes", args.process_workers)):
        for count in filter(None, counts.split(",")):
            workers = int(count)
            if workers < 1:
                parser.error("worker counts must be positive")
            configurations.append((backend, workers))
    target = gp.read_audio(args.target)
    off = len(target) if args.note_off is None else round(args.note_off*gp.RATE)
    if not 1 <= off <= len(target):
        parser.error("note-off must be within the sample or at its end")
    renderer_path = str(Path(args.renderer).resolve())
    init_args = renderer_path, target, args.note, off
    init_worker(*init_args)
    rng = np.random.default_rng(args.seed)
    population = [gp.random_genes(rng, gp.FM_MODES) for _ in range(args.candidates)]
    # Exercise different feedback histories, destinations and the seeded noise wave.
    for i, genes in enumerate(population):
        genes[1] = int(rng.integers(0, 256))
        genes[-1] = i % 8
    population[0][8] = 9  # OP1 noise, independent seeded wave table in each render
    report = {"target": str(Path(args.target).resolve()), "sample_seconds": len(target)/gp.RATE,
              "note": args.note, "note_off_seconds": off/gp.RATE,
              "candidates": args.candidates, "repeats": args.repeats, "seed": args.seed,
              "python": platform.python_version(), "numpy": np.__version__,
              "logical_cpus": os.cpu_count(), "processor": os.environ.get("PROCESSOR_IDENTIFIER", ""),
              "renderer": _context[0].info, "results": []}
    reference = None
    reference_hashes = None
    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    hash_inputs = population[:min(4, len(population))]
    for backend, workers in configurations:
        started = time.perf_counter()
        pool = None
        if backend == "threads":
            pool = ThreadPoolExecutor(max_workers=workers)
        elif backend == "processes":
            pool = ProcessPoolExecutor(max_workers=workers,
                mp_context=multiprocessing.get_context("spawn"),
                initializer=init_worker, initargs=init_args)
        try:
            def run_batch():
                return np.array(list(map(evaluate, population)) if pool is None else
                                list(pool.map(evaluate, population)))
            cold = run_batch()
            first_seconds = time.perf_counter() - started
            if reference is None:
                reference = cold
            np.testing.assert_array_equal(cold, reference)
            samples = []
            for _ in range(args.repeats):
                tick = time.perf_counter()
                result = run_batch()
                samples.append(time.perf_counter()-tick)
                np.testing.assert_array_equal(result, reference)
            # Check native sample identity separately from the timed evaluation.
            hashes = list(map(render_digest, hash_inputs)) if pool is None else list(pool.map(render_digest, hash_inputs))
            if reference_hashes is None:
                reference_hashes = hashes
            if hashes != reference_hashes:
                raise AssertionError("parallel render samples differ from serial")
            median = statistics.median(samples)
            baseline = median if backend == "serial" else report["results"][0]["warm_median_seconds"]
            row = {"backend": backend, "workers": workers,
                   "first_batch_seconds_including_pool_startup": first_seconds,
                   "warm_seconds": samples, "warm_median_seconds": median,
                   "speedup_over_serial": baseline/median,
                   "all_candidate_scores_bit_identical": True,
                   "verified_waveform_sha256": hashes}
            report["results"].append(row)
            output.write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
            print(f"{backend:9s} workers={workers:2d} first={first_seconds:.3f}s "
                  f"warm_median={median:.3f}s speedup={baseline/median:.2f}x identical", flush=True)
        finally:
            if pool is not None:
                pool.shutdown(wait=True, cancel_futures=True)
    print(f"Saved {output.resolve()}", flush=True)


if __name__ == "__main__":
    multiprocessing.freeze_support()
    main()
