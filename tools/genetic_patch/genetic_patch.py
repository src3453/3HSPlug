"""Fit 3HSPlug patches to isolated notes using the real C++ engine and a GA."""
import argparse
from concurrent.futures import ThreadPoolExecutor, as_completed
import ctypes
import hashlib
import json
import math
from pathlib import Path
import subprocess
import sys
import tempfile
import wave

import numpy as np
from tqdm import tqdm
from pitch_estimation import estimate_pitch

RATE = 48000
FIELDS = ("frequency", "attack", "decay", "sustain", "release", "volume", "waveform")
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))
from s3hs_fui import write_fui
from atomic_file import atomic_write, atomic_write_bytes

# mode, feedback, seven parameters per operator, then feedback destination.
# OP1 pitch is not a gene. Keep operator indices stable by appending fbDest.
LIMITS = np.array([12, 255] + [65535, 255, 255, 255, 255, 255, 15] * 8 + [7])
FM_MODES = tuple(range(13))  # 0..12 inclusive; reusable by CLI and the GA.


def validate_patch(patch):
    def integer(value, lo, hi):
        if type(value) is not int or not lo <= value <= hi:
            raise ValueError(f"expected integer {lo}..{hi}, got {value!r}")
        return value
    integer(patch["modmode"], 0, 12)
    integer(patch["feedback"], 0, 255)
    integer(patch.get("keyShift", 0), -24, 24)
    integer(patch.get("fbDest", 0), 0, 7)
    if len(patch["operators"]) != 8:
        raise ValueError("patch must contain exactly 8 operators")
    for op in patch["operators"]:
        for name, limit in zip(FIELDS, LIMITS[2:9]):
            integer(op[name], 0, int(limit))
    return patch


def load_patch(path, program):
    bank = json.loads(Path(path).read_text(encoding="utf-8-sig"))
    matches = [p for p in bank["patches"] if p["program"] == program]
    if len(matches) != 1:
        raise ValueError(f"expected one patch with program {program}")
    source = matches[0]
    operators = source.get("operators", [])
    if not isinstance(operators, list) or len(operators) > 8:
        raise ValueError("patch operators must be an array of at most 8 entries")
    # The plugin loader accepts abbreviated banks: omitted OPs/fields are zero.
    padded = [dict.fromkeys(FIELDS, 0) for _ in range(8)]
    for i, op in enumerate(operators):
        padded[i].update({f: op.get(f, 0) for f in FIELDS})
    patch = {"program": program, "modmode": source.get("modmode", 4),
                           "feedback": source.get("feedback", 128),
                           "keyShift": source.get("keyShift", 0), "operators": padded}
    patch["fbDest"] = source.get("fbDest", 0)
    return validate_patch(patch)


def encode(patch):
    return np.array([patch["modmode"], patch["feedback"]] +
                    [op[f] for op in patch["operators"] for f in FIELDS] +
                    [patch.get("fbDest", 0)], dtype=np.int64)


def decode(genes, program=0):
    ops = [dict(zip(FIELDS, map(int, row))) for row in genes[2:-1].reshape(8, 7)]
    ops[0]["frequency"] = 0
    return {"program": program, "modmode": int(genes[0]),
            "feedback": int(genes[1]), "fbDest": int(genes[-1]), "keyShift": 0, "operators": ops}


def renderer_source_id():
    # Same ordered inputs as CMakeLists.txt; reject a DLL built from other sources.
    core = HERE.parent.parent / "src/s3hs_core"
    vendored = core / "3hs"
    sources = sorted([*vendored.glob("3hs816/src/*.cpp"), *vendored.glob("3hs88pwn4/src/*.cpp")],
                     key=lambda path: path.as_posix())
    headers = sorted([*vendored.glob("3hs816/include/hs816/*.hpp"), *vendored.glob("3hs816/src/*.hpp"),
                      *vendored.glob("3hs88pwn4/include/hs88/*.hpp"), *vendored.glob("3hs88pwn4/src/*.hpp")],
                     key=lambda path: path.as_posix())
    files = ([core / name for name in ("sound.cpp", "ram.cpp", "header/spec.hpp")] +
             sources + headers + [HERE / "renderer.cpp"])
    hashes = "".join(hashlib.sha256(path.read_bytes()).hexdigest() for path in files)
    return hashlib.sha256(hashes.encode("ascii")).hexdigest()


class Renderer:
    def __init__(self, path):
        path = Path(path).resolve()
        self.lib = ctypes.CDLL(str(path))
        rebuild = ("Renderer DLL is outdated or incompatible; rebuild with: "
                   "cmake --build tools/genetic_patch/build --config Release")
        try:
            abi = self.lib.s3hs_renderer_abi_version
            abi.argtypes, abi.restype = [], ctypes.c_int
            source_id = self.lib.s3hs_renderer_source_id
            source_id.argtypes, source_id.restype = [], ctypes.c_char_p
            self.call = self.lib.s3hs_render_v2
        except AttributeError as exc:
            raise RuntimeError(rebuild) from exc
        if abi() != 2:
            raise RuntimeError(rebuild)
        built_id = source_id()
        if built_id != renderer_source_id().encode("ascii"):
            raise RuntimeError(rebuild)
        self.info = {"abi": 2, "source_id": built_id.decode("ascii"), "path": str(path)}
        self.call.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_int, ctypes.c_int,
                              ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_float)]
        self.call.restype = ctypes.c_int

    def render(self, patch, note, count, note_off):
        validate_patch(patch)
        regs = np.zeros(64, dtype=np.uint8)
        for i, op in enumerate(patch["operators"]):
            freq = op["frequency"]
            regs[2*i:2*i+2] = [freq >> 8, freq & 255]
            regs[0x10+i] = op["volume"]
            regs[0x20+4*i:0x24+4*i] = [op[f] for f in FIELDS[1:5]]
            regs[0x18+i//2] |= op["waveform"] << (4 if i % 2 == 0 else 0)
        # Same integer-Hz pitch mapping as PluginProcessor, neutral MIDI controls.
        hz = int(440 * 2 ** ((note + patch.get("keyShift", 0) - 69) / 12))
        regs[0:2] = [hz >> 8, hz & 255]
        regs[0x1c], regs[0x1f] = patch["modmode"], patch["feedback"]
        out = np.empty(count, dtype=np.float32)
        result = self.call(regs.ctypes.data_as(ctypes.POINTER(ctypes.c_uint8)), patch.get("fbDest", 0), RATE,
                           count, note_off, out.ctypes.data_as(ctypes.POINTER(ctypes.c_float)))
        if result or not np.isfinite(out).all():
            raise RuntimeError(f"native render failed: {result}")
        return out


class UnsupportedWavFormat(ValueError):
    pass


def read_wav(path):
    """Read native PCM16 WAV; distinguish format errors from invalid audio."""
    with wave.open(str(path), "rb") as f:
        if f.getframerate() != RATE or f.getsampwidth() != 2 or f.getnchannels() not in (1, 2):
            raise UnsupportedWavFormat("target must be 48000 Hz, 16-bit PCM mono/stereo WAV")
        x = np.frombuffer(f.readframes(f.getnframes()), dtype="<i2").astype(np.float32)
        x = x.reshape(-1, f.getnchannels()).mean(axis=1) / 32768
    if len(x) < 256 or not np.isfinite(x).all() or np.max(np.abs(x)) < 1e-5:
        raise ValueError("target is empty, too short, or silent")
    return x


def read_audio(path, ffmpeg="ffmpeg"):
    """Use native WAV directly, otherwise decode to a temporary PCM16 WAV."""
    source = Path(path).resolve(strict=True)
    try:
        return read_wav(source)
    except (UnsupportedWavFormat, wave.Error, EOFError):
        pass
    print(f"Converting input with ffmpeg: {source}", file=sys.stderr)
    with tempfile.TemporaryDirectory(prefix="s3hs-audio-") as temp:
        converted = Path(temp) / "converted.wav"
        command = [str(ffmpeg), "-nostdin", "-hide_banner", "-loglevel", "error",
                   "-i", str(source), "-map", "0:a:0", "-vn", "-ar", str(RATE),
                   "-af", "aformat=channel_layouts=mono|stereo",
                   "-c:a", "pcm_s16le", "-f", "wav", str(converted)]
        try:
            result = subprocess.run(command, capture_output=True, text=True,
                                    encoding="utf-8", errors="replace",
                                    creationflags=getattr(subprocess, "CREATE_NO_WINDOW", 0))
        except FileNotFoundError as exc:
            raise RuntimeError("ffmpeg not found; add it to PATH or use --ffmpeg PATH") from exc
        if result.returncode:
            raise RuntimeError(f"ffmpeg conversion failed (exit {result.returncode}): "
                               f"{result.stderr.strip()[-3000:]}")
        return read_wav(converted)


def write_wav(path, x):
    def write(stream):
        with wave.open(stream, "wb") as f:
            f.setparams((1, 2, RATE, 0, "NONE", "not compressed"))
            f.writeframes(np.rint(np.clip(x, -1, 1) * 32767).astype("<i2").tobytes())
    atomic_write(path, write)


class Objective:
    """Time-aligned STFT magnitude loss, with frequency pooling annealed by stage.

    A single target-derived scale is used for all candidates: normalizing each
    candidate separately would hide level/envelope errors and favor quiet sounds.
    """
    def __init__(self, target):
        self.scale = max(float(np.max(np.abs(target))), 1e-5)
        self.windows = (512, 2048)
        self.target = self.features(target)
        self.reference = [self.pool(self.target, width) for width in (16, 4, 1)]
        for group in [self.target] + self.reference:
            for array in group:
                array.flags.writeable = False

    def features(self, x):
        output = []
        for size in self.windows:
            hop = size // 4
            padded = np.pad(x / self.scale, (size//2, size//2))
            frames = np.lib.stride_tricks.sliding_window_view(padded, size)[::hop]
            output.append(np.abs(np.fft.rfft(frames * np.hanning(size), axis=1)) / size)
        return output

    @staticmethod
    def pool(features, width):
        result = []
        for spec in features:
            padded = np.pad(spec, ((0, 0), (0, (-spec.shape[1]) % width)))
            bands = padded.reshape(len(spec), -1, width).sum(axis=-1)
            result.append(bands)
        return result

    def scores(self, x):
        features = self.features(x)
        scores = []
        for stage, width in enumerate((16, 4, 1)):
            losses = []
            for candidate, target in zip(self.pool(features, width), self.reference[stage]):
                linear = np.sum(np.abs(candidate-target)) / max(np.sum(target), 1e-12)
                log = np.mean(np.abs(np.log1p(100*candidate)-np.log1p(100*target)))
                losses.append(linear + log)
            scores.append(float(np.mean(losses)))
        return np.array(scores)


def random_genes(rng, modes):
    genes = rng.integers(0, LIMITS + 1)
    genes[0] = rng.choice(modes)
    genes[1] = 128
    ops = genes[2:-1].reshape(8, 7)
    ops[:, 0] = rng.choice([2048, 4096, 8192, 12288, 16384, 20480, 24576, 32768], 8)
    ops[:, 1] = rng.integers(0, 24, 8)
    ops[:, 2] = rng.integers(0, 128, 8)
    ops[:, 4] = rng.integers(0, 64, 8)
    ops[:, 5] = rng.integers(0, 192, 8)
    ops[:, 6] = 0  # start with sine operators; optional waveform mutations
    genes[2] = 0
    return genes


def breed(rng, a, b, modes, waveforms):
    child = np.where(rng.random(len(a)) < 0.5, a, b).copy()
    mask = rng.random(len(a)) < 0.12
    steps = np.array([1, 16] + [512, 12, 24, 24, 16, 24, 1] * 8 + [1])
    child += np.rint(rng.normal(size=len(a)) * steps).astype(np.int64) * mask
    child = np.clip(child, 0, LIMITS)
    if rng.random() < 0.1:
        child[0] = rng.choice(modes)
    if child[0] not in modes:
        child[0] = rng.choice(modes)
    if rng.random() < 0.1:
        child[-1] = rng.integers(0, 8)
    for i in range(8):
        pos = 2 + i*7
        if rng.random() < 0.08:
            child[pos] = int(rng.integers(1, 16)) * 4096
        if not waveforms:
            child[pos+6] = 0
    child[2] = 0
    return child


def evaluate_population(population, cache, evaluate, executor=None, *, progress=False):
    """Evaluate each uncached genotype once; only the controller writes the cache."""
    pending = {}
    next_cache = {}
    for genes in population:
        key = tuple(genes)
        if key in cache:
            next_cache[key] = cache[key]
        elif key not in pending:
            pending[key] = genes
    with tqdm(total=len(pending), desc="Candidates", unit="patch", position=1,
              leave=False, dynamic_ncols=True, ascii=True, miniters=1,
              disable=not progress) as bar:
        if executor is None:
            for key, genes in pending.items():
                next_cache[key] = evaluate(genes)
                bar.update()
        else:
            futures = {executor.submit(evaluate, genes): key for key, genes in pending.items()}
            for future in as_completed(futures):
                next_cache[futures[future]] = future.result()
                bar.update()
    return next_cache


def search(args, renderer):
    workers = getattr(args, "workers", 8)
    if workers < 1:
        raise ValueError("workers must be >=1")
    target = read_audio(args.target, getattr(args, "ffmpeg", "ffmpeg"))
    count = len(target)
    note = args.note
    if note is None:
        pitch = estimate_pitch(target, RATE)
        note = pitch["note"]
        print(f"Estimated pitch: {pitch['frequency_hz']:.2f} Hz -> MIDI {note} "
              f"({pitch['cents_from_note']:+.1f} cents)", flush=True)
    else:
        if not 0 <= note <= 127:
            raise ValueError("note must be in 0..127")
        pitch = {"method": "manual", "note": note}
    if args.note_off is None:
        off = count
    else:
        if not math.isfinite(args.note_off) or not 0 < args.note_off <= count / RATE:
            raise ValueError("--note-off must be positive and no later than the target end")
        off = round(args.note_off * RATE)
    if not 1 <= off <= count:
        raise ValueError("--note-off must be within the target recording or at its end")
    objective = Objective(target)
    rng = np.random.default_rng(args.seed)
    modes = tuple(map(int, args.modes.split(",")))
    if not modes or any(m < 0 or m > 12 for m in modes):
        raise ValueError("--modes must contain values 0..12")
    population = [random_genes(rng, modes) for _ in range(args.population)]
    if args.initial:
        patch = load_patch(args.initial, args.program)
        if patch.get("keyShift", 0) != 0 or patch["modmode"] not in modes:
            raise ValueError("initial patch requires keyShift=0 and an allowed mode")
        if not args.waveforms and any(op["waveform"] for op in patch["operators"]):
            raise ValueError("initial patch with non-sine waves requires --waveforms")
        population[0] = encode(patch)
        population[0][2] = 0
    output = Path(args.output)
    output.mkdir(parents=True, exist_ok=True)
    best, best_score = None, math.inf
    history = []
    # Only retain scores, never rendered audio/STFTs. Bound the cache per generation.
    cache = {}
    def evaluate(genes):
        return objective.scores(renderer.render(decode(genes), note, count, off))
    pool = (ThreadPoolExecutor(max_workers=workers, thread_name_prefix="s3hs-search")
            if workers > 1 else None)
    progress = not getattr(args, "no_progress", False) and sys.stderr.isatty()
    generations = tqdm(total=args.generations, desc="Generations", unit="gen", position=0,
                       dynamic_ncols=True, ascii=True, disable=not progress)
    try:
        for generation in range(args.generations):
            stage = min(2, 3 * generation // args.generations)
            cache = evaluate_population(population, cache, evaluate, pool, progress=progress)
            evaluated = []
            # Apply scores in original population order, independent of completion order.
            for genes in population:
                scores = cache[tuple(genes)]
                evaluated.append((float(scores[stage]), genes))
                if scores[2] < best_score:
                    best, best_score = genes.copy(), float(scores[2])
            evaluated.sort(key=lambda item: item[0])
            history.append({"generation": generation, "stage": stage,
                            "selection_loss": evaluated[0][0], "best_fine_loss": best_score})
            save_result(output, best, best_score, history, args, renderer, count, off, note, pitch)
            generations.set_postfix(stage=stage, loss=f"{best_score:.6f}", refresh=False)
            generations.update()
            if not progress:
                print(f"generation={generation:04d} stage={stage} best_fine_loss={best_score:.6f}", flush=True)
            # Keep a champion measured at full resolution across every stage transition.
            population = [best.copy()] + [g.copy() for _, g in evaluated[:max(1, args.population//8)]]
            def tournament():
                indices = rng.integers(0, len(evaluated), 3)
                return evaluated[min(indices)][1]
            while len(population) < args.population:
                population.append(breed(rng, tournament(), tournament(), modes, args.waveforms))
            population = population[:args.population]
    finally:
        generations.close()
        if pool is not None:
            pool.shutdown(wait=True, cancel_futures=True)
    return history


def save_result(output, best, score, history, args, renderer, count, off, note, pitch):
    patch = decode(best, args.program)
    metadata = {"target": str(Path(args.target).resolve()), "note": note, "pitch": pitch,
                "note_off_seconds": off / RATE, "sample_rate": RATE,
                "seed": args.seed, "population": args.population, "generations": args.generations,
                "workers": getattr(args, "workers", 8),
                "modes": args.modes, "waveforms": args.waveforms,
                "ffmpeg": getattr(args, "ffmpeg", "ffmpeg"),
                "renderer": renderer.info,
                "initial": args.initial, "best_fine_loss": score, "history": history}
    # Publish each complete file; retry brief Windows locks without rerendering.
    for name, data in (("best_patch.json", {"patches": [patch]}), ("search.json", metadata)):
        atomic_write_bytes(output / name, (json.dumps(data, indent=2) + "\n").encode("utf-8"))
    write_wav(output / "best.wav", renderer.render(patch, note, count, off))
    write_fui(output / "best_patch.fui", patch, f"{Path(args.target).stem} GA")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--renderer", default=str(HERE / "build/Release/s3hs_patch_renderer.dll"))
    sub = parser.add_subparsers(dest="command", required=True)
    fit = sub.add_parser("search", help="fit an isolated note; convert other audio with ffmpeg")
    fit.add_argument("target")
    fit.add_argument("--ffmpeg", default="ffmpeg", help="ffmpeg executable (default: PATH)")
    fit.add_argument("--output", required=True)
    fit.add_argument("--note", type=int, help="target MIDI note number (default: estimate pitch with YIN)")
    fit.add_argument("--note-off", type=float,
                     help="seconds from decoded audio start (default: end of sample)")
    fit.add_argument("--population", type=int, default=48)
    fit.add_argument("--generations", type=int, default=100)
    fit.add_argument("--workers", type=int, default=8,
                     help="candidate evaluation threads (default: 8; 1 runs serially)")
    fit.add_argument("--no-progress", action="store_true",
                     help="disable tqdm bars and use per-generation log lines (also automatic off-TTY)")
    fit.add_argument("--seed", type=int, default=1)
    fit.add_argument("--modes", default=",".join(map(str, FM_MODES)))
    fit.add_argument("--waveforms", action="store_true", help="also search non-sine waveforms")
    fit.add_argument("--initial", help="seed from an existing patch bank JSON")
    fit.add_argument("--program", type=int, default=0)
    render = sub.add_parser("render", help="render an existing patch as a test/reference WAV")
    render.add_argument("patch")
    render.add_argument("--program", type=int, default=0)
    render.add_argument("--note", type=int, default=69)
    render.add_argument("--duration", type=float, default=1.0)
    render.add_argument("--note-off", type=float, default=0.7)
    render.add_argument("--output", required=True)
    export = sub.add_parser("export-fui", help="export a JSON bank patch as a Furnace S3HS instrument")
    export.add_argument("patch")
    export.add_argument("--program", type=int, default=0)
    export.add_argument("--name", help="instrument name (default: JSON filename)")
    export.add_argument("--output", required=True)
    args = parser.parse_args()
    try:
        if args.command == "export-fui":
            if not 0 <= args.program <= 127:
                raise ValueError("program must be in 0..127")
            write_fui(args.output, load_patch(args.patch, args.program),
                      args.name if args.name is not None else Path(args.patch).stem)
            return
        if (args.note is not None and not 0 <= args.note <= 127) or not 0 <= args.program <= 127:
            raise ValueError("note and program must be in 0..127")
        if args.note_off is not None and (not math.isfinite(args.note_off) or args.note_off <= 0):
            raise ValueError("note-off must be positive and finite")
        renderer = Renderer(args.renderer)
        if args.command == "search":
            if args.population < 4 or args.generations < 1:
                raise ValueError("population must be >=4 and generations >=1")
            search(args, renderer)
        else:
            if not math.isfinite(args.duration) or not 0 < args.note_off <= args.duration:
                raise ValueError("require 0 < note-off <= duration, both finite")
            x = renderer.render(load_patch(args.patch, args.program), args.note,
                                round(args.duration * RATE), round(args.note_off * RATE))
            write_wav(args.output, x)
    except (ValueError, OSError, KeyError, TypeError, RuntimeError, wave.Error) as exc:
        parser.exit(2, f"error: {exc}\n")


if __name__ == "__main__":
    main()
