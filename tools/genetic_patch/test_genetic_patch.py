"""Behavioral tests; build the native renderer before running unittest."""
import argparse
import copy
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import threading
import time
import unittest
from unittest import mock
import wave
import struct

import numpy as np
import genetic_patch as gp
import s3hs_fui as fui
from pitch_estimation import estimate_pitch
import atomic_file


@contextlib.contextmanager
def deny_windows_delete(path):
    """Keep a real Windows handle open without FILE_SHARE_DELETE."""
    import ctypes
    from ctypes import wintypes
    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.CreateFileW.argtypes = [wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.LPVOID, wintypes.DWORD, wintypes.DWORD,
                                  wintypes.HANDLE]
    kernel.CreateFileW.restype = wintypes.HANDLE
    kernel.CloseHandle.argtypes = [wintypes.HANDLE]
    kernel.CloseHandle.restype = wintypes.BOOL
    handle = kernel.CreateFileW(str(path), 0x80000000, 3, None, 3, 0, None)
    if handle == ctypes.c_void_p(-1).value:
        raise ctypes.WinError(ctypes.get_last_error())
    try:
        yield
    finally:
        kernel.CloseHandle(handle)


class AtomicSaveTests(unittest.TestCase):
    @unittest.skipUnless(os.name == "nt", "requires real Windows sharing locks")
    def test_locked_json_wav_and_fui_recover_after_reader_closes(self):
        patch = gp.decode(gp.random_genes(np.random.default_rng(1), [4]))
        writers = {
            "best_patch.json": lambda p: atomic_file.atomic_write_bytes(p, b'{"new": true}\n'),
            "best.wav": lambda p: gp.write_wav(p, .2*np.sin(np.arange(4800)*2*np.pi*440/gp.RATE)),
            "best_patch.fui": lambda p: fui.write_fui(p, patch, "locked test"),
        }
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            for name, write in writers.items():
                with self.subTest(name=name):
                    path = Path(tmp) / name
                    path.write_bytes(b"old checkpoint")
                    blocked = threading.Event()
                    failures = []
                    replace = os.replace

                    def observed_replace(src, dst):
                        try:
                            return replace(src, dst)
                        except OSError:
                            blocked.set()
                            raise

                    def save():
                        try:
                            write(path)
                        except BaseException as exc:
                            failures.append(exc)

                    with mock.patch.object(atomic_file.os, "replace", observed_replace):
                        with deny_windows_delete(path):
                            worker = threading.Thread(target=save)
                            worker.start()
                            was_blocked = blocked.wait(2)
                            old_data = path.read_bytes()
                        worker.join(5)
                    self.assertTrue(was_blocked, "replacement must encounter the real lock")
                    self.assertEqual(old_data, b"old checkpoint")
                    self.assertFalse(worker.is_alive())
                    self.assertEqual(failures, [])
                    self.assertNotEqual(path.read_bytes(), old_data)
                    if name.endswith(".wav"):
                        self.assertEqual(len(gp.read_wav(path)), 4800)
                    elif name.endswith(".fui"):
                        self.assertEqual(fui.decode_fui(path.read_bytes())[1], "locked test")
                    else:
                        self.assertEqual(json.loads(path.read_bytes()), {"new": True})
                    self.assertEqual(list(Path(tmp).glob(".*.tmp")), [])

    @unittest.skipUnless(os.name == "nt", "requires real Windows sharing locks")
    def test_persistent_lock_is_bounded_and_retains_both_versions(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            path = Path(tmp) / "best_patch.json"
            path.write_bytes(b"old")
            start = time.monotonic()
            with deny_windows_delete(path):
                with self.assertRaisesRegex(OSError, "Completed data retained at") as error:
                    atomic_file.atomic_write_bytes(path, b"new", retry_seconds=0.1)
            self.assertLess(time.monotonic()-start, 2)
            self.assertGreaterEqual(time.monotonic()-start, 0.1)
            self.assertIn(str(path), str(error.exception))
            self.assertEqual(path.read_bytes(), b"old")
            pending, = Path(tmp).glob(".*.tmp")
            self.assertEqual(pending.read_bytes(), b"new")

    def test_unique_staging_and_nonlocking_failures(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            path = Path(tmp) / "best_patch.json"
            path.write_bytes(b"old")
            legacy_temp = Path(tmp) / "best_patch.json.tmp"
            legacy_temp.write_bytes(b"another writer")
            atomic_file.atomic_write_bytes(path, b"new")
            self.assertEqual(legacy_temp.read_bytes(), b"another writer")
            # Unrelated IO errors must fail immediately, preserving completed data.
            with mock.patch.object(atomic_file.os, "replace", side_effect=OSError(28, "disk full")) as replace:
                with self.assertRaisesRegex(OSError, "disk full"):
                    atomic_file.atomic_write_bytes(path, b"pending")
            self.assertEqual(replace.call_count, 1)
            self.assertEqual(path.read_bytes(), b"new")
            pending, = Path(tmp).glob(".*.tmp")
            self.assertEqual(pending.read_bytes(), b"pending")

    def test_partial_write_does_not_replace_previous_checkpoint(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            path = Path(tmp) / "best.wav"
            path.write_bytes(b"old")

            def broken_write(stream):
                stream.write(b"partial")
                raise ValueError("writer failed")

            with self.assertRaisesRegex(ValueError, "writer failed"):
                atomic_file.atomic_write(path, broken_write)
            self.assertEqual(path.read_bytes(), b"old")
            self.assertEqual(list(Path(tmp).glob(".*.tmp")), [])


class PitchTests(unittest.TestCase):
    def test_notes_with_strong_second_harmonic_and_detuning(self):
        t = np.arange(24000)/gp.RATE
        for note in (21, 33, 48, 60, 69, 84, 96, 108):
            with self.subTest(note=note):
                f = 440*2**((note-69)/12)
                x = .15*np.sin(2*np.pi*f*t)+.35*np.sin(4*np.pi*f*t)+.1*np.sin(6*np.pi*f*t)
                result = estimate_pitch(x)
                self.assertEqual(result["note"], note)
                self.assertLess(abs(result["frequency_hz"]/f-1), 0.005)
        f = 440*2**(23/1200)
        x = np.sin(2*np.pi*f*t)*np.exp(-t*4)
        x = np.concatenate((np.zeros(12000), x, np.zeros(24000)))+0.2
        result = estimate_pitch(x)
        self.assertEqual(result["note"], 69)
        self.assertAlmostEqual(result["cents_from_note"], 23, delta=2)

    def test_nonperiodic_and_invalid_samples_are_rejected(self):
        rng = np.random.default_rng(7)
        for x in (np.zeros(48000), np.ones(48000), rng.normal(0, .1, 48000),
                  np.full(48000, np.nan), np.zeros(100)):
            with self.assertRaisesRegex(ValueError, "specify --note"):
                estimate_pitch(x)


class FuiTests(unittest.TestCase):
    def setUp(self):
        self.patch = {"program": 9, "modmode": 12, "feedback": 0, "keyShift": 0,
            "operators": [{"frequency": 0 if i == 0 else 0x1000+i*0x101,
                "waveform": i, "volume": 255-i, "attack": 10+i, "decay": 20+i,
                "sustain": 30+i, "release": 40+i} for i in range(8)]}

    def test_furnace_layout_and_roundtrip(self):
        # Explicit offsets from Furnace writeFeature3H, independent of our decoder.
        data = fui.encode_s3hs_feature(self.patch)
        self.assertEqual(len(data), 64)
        self.assertEqual(data[:7], bytes(range(0x11, 0x18)))
        self.assertEqual(data[7:14], bytes(range(1, 8)))
        self.assertEqual(data[14:22], bytes(range(8)))
        self.assertEqual(data[22:27], bytes([255, 10, 20, 30, 40]))
        self.assertEqual(data[57:64], bytes([248, 17, 27, 37, 47, 12, 128]))
        for feedback in (0, 127, 128, 255):
            self.patch["feedback"] = feedback
            decoded, name = fui.decode_fui(fui.encode_fui(self.patch, "ピアノ 3H@"))
            self.assertEqual(name, "ピアノ 3H@")
            decoded["program"] = 9  # MIDI program numbers are external to FUI.
            self.assertEqual(decoded, self.patch)
        modern = fui.encode_s3hs_feature(self.patch) + b"\0"
        self.assertEqual(fui.decode_s3hs_feature(modern)["operators"], self.patch["operators"])
        self.patch["fbDest"] = 7
        decoded, _ = fui.decode_fui(fui.encode_fui(self.patch))
        decoded["program"] = 9
        self.assertEqual(decoded, self.patch)
        self.assertEqual(len(fui.encode_s3hs_feature(self.patch)), 65)
        # EOF-terminated files are produced by Furnace when there are no assets.
        self.assertEqual(fui.decode_fui(fui.encode_fui(self.patch)[:-2])[0],
                         fui.decode_fui(fui.encode_fui(self.patch))[0])

    def test_feature_boundaries_and_export_restrictions(self):
        data = fui.encode_fui(self.patch)
        with self.assertRaises(ValueError):
            fui.decode_fui(data[:-3])
        with self.assertRaisesRegex(ValueError, "not S3HS"):
            fui.decode_fui(data[:6] + struct.pack("<H", 1) + data[8:])
        self.patch["keyShift"] = 1
        with self.assertRaisesRegex(ValueError, "keyShift"):
            fui.encode_fui(self.patch)


class AudioInputTests(unittest.TestCase):
    def test_native_wav_needs_no_ffmpeg_and_silence_is_rejected(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            path = Path(tmp) / "native.wav"
            x = (np.sin(np.arange(4800) * 2*np.pi*440/gp.RATE) * 0.2).astype(np.float32)
            gp.write_wav(path, x)
            missing_ffmpeg = str(Path(tmp) / "missing-ffmpeg.exe")
            np.testing.assert_array_equal(gp.read_audio(path, missing_ffmpeg), gp.read_wav(path))
            gp.write_wav(path, np.zeros_like(x))
            with self.assertRaisesRegex(ValueError, "silent"):
                gp.read_audio(path, missing_ffmpeg)

    @unittest.skipUnless(shutil.which("ffmpeg"), "ffmpeg not on PATH")
    def test_real_conversion_and_search(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            source = root / "入力 音源 44100.wav"
            samples = np.sin(np.arange(4410) * 2*np.pi*440/44100) * 0.2
            with wave.open(str(source), "wb") as f:
                f.setparams((1, 2, 44100, 0, "NONE", "not compressed"))
                f.writeframes(np.rint(samples*32767).astype("<i2").tobytes())
            original = source.read_bytes()
            flac = root / "入力 音源.flac"
            subprocess.run([shutil.which("ffmpeg"), "-nostdin", "-v", "error",
                            "-i", str(source), str(flac)], check=True, capture_output=True)
            a, b = gp.read_audio(source), gp.read_audio(flac)
            np.testing.assert_array_equal(a, b)
            self.assertEqual(len(a), 4800)
            self.assertAlmostEqual(float(np.sqrt(np.mean(a*a))), 0.2/np.sqrt(2), places=4)
            stereo = root / "stereo.wav"
            stereo_data = np.column_stack((samples, samples*0.5))
            with wave.open(str(stereo), "wb") as f:
                f.setparams((2, 2, 44100, 0, "NONE", "not compressed"))
                f.writeframes(np.rint(stereo_data*32767).astype("<i2").tobytes())
            mixed = gp.read_audio(stereo)
            self.assertAlmostEqual(float(np.sqrt(np.mean(mixed*mixed))),
                                   0.15/np.sqrt(2), places=4)
            self.assertEqual(source.read_bytes(), original)
            # Exercise conversion through the real search entry point, including export.
            args = argparse.Namespace(target=str(flac), note_off=None, note=None,
                seed=1, modes="6", population=4, generations=1, initial=None,
                program=0, waveforms=False, output=str(root / "result"), ffmpeg="ffmpeg")
            renderer = gp.Renderer(gp.HERE / "build/Release/s3hs_patch_renderer.dll")
            with contextlib.redirect_stdout(io.StringIO()):
                gp.search(args, renderer)
            self.assertEqual(len(gp.read_wav(root / "result/best.wav")), 4800)
            metadata = json.loads((root / "result/search.json").read_text())
            self.assertEqual(metadata["note_off_seconds"], 0.1)
            self.assertEqual(metadata["note"], 69)
            self.assertEqual(metadata["pitch"]["method"], "yin_numpy")

    def test_conversion_errors_are_actionable(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            bad = Path(tmp) / "invalid.wav"
            bad.write_bytes(b"not audio")
            with self.assertRaisesRegex(RuntimeError, "ffmpeg not found"):
                gp.read_audio(bad, str(Path(tmp) / "missing-ffmpeg.exe"))
            if shutil.which("ffmpeg"):
                with self.assertRaisesRegex(RuntimeError, "ffmpeg conversion failed"):
                    gp.read_audio(bad)
            with self.assertRaises(FileNotFoundError):
                gp.read_audio(Path(tmp) / "missing.wav")


class GeneticPatchTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.renderer = gp.Renderer(gp.HERE / "build/Release/s3hs_patch_renderer.dll")
        cls.patch = gp.decode(gp.random_genes(np.random.default_rng(4), (6,)))
        cls.patch["fbDest"] = 0
        for op in cls.patch["operators"]:
            op.update(frequency=4096, attack=0, decay=5, sustain=140, release=2, volume=0)
        cls.patch["operators"][0]["frequency"] = 0
        cls.patch["operators"][0]["volume"] = 220
        cls.patch["operators"][1]["volume"] = 35
        cls.audio = cls.renderer.render(cls.patch, 69, 7680, 4800)

    def test_native_repeatability_gate_and_pitch(self):
        repeated = self.renderer.render(self.patch, 69, 7680, 4800)
        np.testing.assert_array_equal(self.audio, repeated)
        self.assertGreater(float(np.max(np.abs(self.audio))), 0.001)
        # Release=2 -> 46.875 ms: the tail must finish before this recording ends.
        self.assertEqual(float(np.max(np.abs(self.audio[-480:]))), 0)
        held = self.renderer.render(self.patch, 69, 7680, 7680)
        self.assertGreater(float(np.max(np.abs(held[-480:]))), 0.001)
        shifted = self.renderer.render(self.patch, 81, 7680, 4800)
        self.assertFalse(np.array_equal(self.audio, shifted))

    def test_every_feedback_destination_affects_only_the_selected_operator(self):
        for dest in range(8):
            with self.subTest(destination=dest):
                patch = copy.deepcopy(self.patch)
                patch.update(modmode=0, feedback=128, fbDest=dest)
                for i, op in enumerate(patch["operators"]):
                    op["volume"] = 220 if i == dest else 0
                baseline = self.renderer.render(patch, 69, 2048, 2048)
                patch["feedback"] = 224
                active = self.renderer.render(patch, 69, 2048, 2048)
                self.assertGreater(float(np.linalg.norm(active-baseline)), 0.01)
                patch["fbDest"] = (dest+1) % 8
                inactive = self.renderer.render(patch, 69, 2048, 2048)
                np.testing.assert_array_equal(inactive, baseline)
                patch["fbDest"] = dest
                # Other renders must not leave feedback history in this candidate.
                np.testing.assert_array_equal(self.renderer.render(patch, 69, 2048, 2048), active)

    def test_old_json_defaults_to_op1_and_stale_renderer_is_rejected(self):
        legacy = copy.deepcopy(self.patch)
        legacy.pop("fbDest")
        np.testing.assert_array_equal(gp.encode(legacy), gp.encode(self.patch))
        legacy["feedback"] = 224
        explicit = dict(legacy, fbDest=0)
        np.testing.assert_array_equal(self.renderer.render(legacy, 69, 2048, 2048),
                                      self.renderer.render(explicit, 69, 2048, 2048))
        with mock.patch.object(gp, "renderer_source_id", return_value="0"*64):
            with self.assertRaisesRegex(RuntimeError, "rebuild with"):
                gp.Renderer(gp.HERE / "build/Release/s3hs_patch_renderer.dll")

    def test_initial_patch_with_nonzero_destination_survives_search_and_export(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            patch = copy.deepcopy(self.patch)
            patch.update(feedback=224, fbDest=1)
            target = self.renderer.render(patch, 69, 7680, 4800)
            gp.write_wav(root / "target.wav", target)
            (root / "initial.json").write_text(json.dumps({"patches": [patch]}))
            args = argparse.Namespace(target=str(root / "target.wav"), note_off=0.1, note=69,
                seed=3, modes="6", population=4, generations=1, initial=str(root / "initial.json"),
                program=0, waveforms=False, output=str(root / "result"))
            with contextlib.redirect_stdout(io.StringIO()):
                with mock.patch.object(gp, "estimate_pitch", side_effect=AssertionError("manual note must win")):
                    gp.search(args, self.renderer)
            best = gp.load_patch(root / "result/best_patch.json", 0)
            self.assertEqual(best, patch)
            imported, _ = fui.decode_fui((root / "result/best_patch.fui").read_bytes())
            imported["program"] = 0
            self.assertEqual(imported, patch)
            np.testing.assert_array_equal(self.renderer.render(imported, 69, 7680, 4800), target)
            metadata = json.loads((root / "result/search.json").read_text())
            self.assertEqual(metadata["renderer"]["source_id"], gp.renderer_source_id())

    def test_objective_and_pcm_roundtrip(self):
        loss = gp.Objective(self.audio)
        np.testing.assert_array_equal(loss.scores(self.audio), np.zeros(3))
        self.assertTrue(np.all(loss.scores(np.zeros_like(self.audio)) > 0.9))
        self.assertTrue(np.all(loss.scores(self.audio * 0.1) > 0))
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            path = Path(tmp) / "target.wav"
            gp.write_wav(path, self.audio)
            np.testing.assert_allclose(gp.read_wav(path), self.audio, atol=1/32768)

    def test_search_improves_and_exports_reproducible_patch(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            target = root / "target.wav"
            gp.write_wav(target, self.audio)
            def run(output, workers):
                args = argparse.Namespace(target=str(target), note_off=0.1, note=69,
                    seed=7, modes="6", population=8, generations=9, initial=None,
                    program=0, waveforms=False, output=str(output), workers=workers)
                with contextlib.redirect_stdout(io.StringIO()):
                    return gp.search(args, self.renderer)
            history = run(root / "first", 1)
            self.assertEqual({h["stage"] for h in history}, {0, 1, 2})
            losses = [h["best_fine_loss"] for h in history]
            self.assertTrue(all(b <= a for a, b in zip(losses, losses[1:])))
            self.assertLess(losses[-1], losses[0])
            output_patch = gp.load_patch(root / "first/best_patch.json", 0)
            gp.validate_patch(output_patch)
            fui_patch, name = fui.decode_fui((root / "first/best_patch.fui").read_bytes())
            fui_patch["program"] = 0
            self.assertEqual(fui_patch, output_patch)
            self.assertEqual(name, "target GA")
            output_audio = self.renderer.render(output_patch, 69, 7680, 4800)
            np.testing.assert_allclose(gp.read_wav(root / "first/best.wav"), output_audio, atol=1/32768)
            np.testing.assert_array_equal(self.renderer.render(fui_patch, 69, 7680, 4800), output_audio)
            expected = gp.Objective(gp.read_wav(target)).scores(output_audio)[2]
            self.assertAlmostEqual(expected, losses[-1])
            for workers in (2, 8):
                parallel = root / f"workers-{workers}"
                self.assertEqual(history, run(parallel, workers))
                for name in ("best_patch.json", "best.wav", "best_patch.fui"):
                    self.assertEqual((root / "first" / name).read_bytes(),
                                     (parallel / name).read_bytes())
                self.assertEqual(json.loads((parallel / "search.json").read_text())["workers"], workers)
            metadata = json.loads((root / "first/search.json").read_text())
            self.assertEqual(metadata["history"], history)
            print(f"GA regression: fine loss {losses[0]:.6f} -> {losses[-1]:.6f}")

    def test_parallel_cache_and_failure_cleanup(self):
        a, b = np.array([1]), np.array([2])
        old_score, new_score = np.array([1., 2., 3.]), np.array([4., 5., 6.])
        evaluate = mock.Mock(return_value=new_score)
        with gp.ThreadPoolExecutor(max_workers=2) as pool:
            result = gp.evaluate_population([a, b, b.copy(), a.copy()],
                                            {tuple(a): old_score}, evaluate, pool)
        evaluate.assert_called_once()
        self.assertIs(result[tuple(a)], old_score)
        self.assertIs(result[tuple(b)], new_score)
        with self.assertRaisesRegex(ValueError, "workers"):
            gp.search(argparse.Namespace(workers=0), self.renderer)
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            gp.write_wav(root / "target.wav", self.audio)
            args = argparse.Namespace(target=str(root / "target.wav"), note_off=0.1, note=69,
                seed=1, modes="6", population=8, generations=2, initial=None,
                program=0, waveforms=False, output=str(root / "result"), workers=2)
            with mock.patch.object(self.renderer, "render", side_effect=RuntimeError("worker failed")):
                with self.assertRaisesRegex(RuntimeError, "worker failed"):
                    gp.search(args, self.renderer)
            self.assertFalse(any(t.name.startswith("s3hs-search") for t in threading.enumerate()))

    def test_progress_display_and_opt_out_preserve_search_outputs(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            target = root / "target.wav"
            gp.write_wav(target, self.audio)
            histories = []
            for name, tty, disabled in (("bars", True, False), ("disabled", True, True),
                                         ("redirected", False, False)):
                args = argparse.Namespace(target=str(target), note_off=0.1, note=69,
                    seed=1, modes="6", population=4, generations=2, initial=None,
                    program=0, waveforms=False, output=str(root / name), workers=2,
                    no_progress=disabled)
                stdout, stderr = io.StringIO(), io.StringIO()
                with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
                    with mock.patch.object(stderr, "isatty", return_value=tty):
                        histories.append(gp.search(args, self.renderer))
                if name == "bars":
                    self.assertIn("Generations", stderr.getvalue())
                    self.assertIn("Candidates", stderr.getvalue())
                    self.assertIn("100%", stderr.getvalue())
                    self.assertNotIn("generation=", stdout.getvalue())
                else:
                    self.assertEqual(stderr.getvalue(), "")
                    self.assertIn("generation=0001", stdout.getvalue())
            self.assertEqual(histories[0], histories[1])
            self.assertEqual(histories[0], histories[2])
            for name in ("best_patch.json", "best.wav", "best_patch.fui", "search.json"):
                expected = (root / "bars" / name).read_bytes()
                for other in ("disabled", "redirected"):
                    self.assertEqual((root / other / name).read_bytes(), expected)

    def test_bad_inputs_and_gene_bounds(self):
        rng = np.random.default_rng(12)
        a = gp.random_genes(rng, gp.FM_MODES)
        destinations = set()
        for _ in range(100):
            a = gp.breed(rng, a, gp.random_genes(rng, gp.FM_MODES), gp.FM_MODES, True)
            gp.validate_patch(gp.decode(a))
            self.assertEqual(a[2], 0)
            destinations.add(int(a[-1]))
        self.assertEqual(destinations, set(range(8)))
        with self.assertRaises(RuntimeError):
            self.renderer.render(self.patch, 69, 100, 101)

    def test_existing_abbreviated_bank(self):
        patch = gp.load_patch(gp.HERE / "../../assets/patches/0.json", 0)
        self.assertEqual(len(patch["operators"]), 8)
        self.assertEqual(patch["operators"][7]["volume"], 0)
        rendered = self.renderer.render(patch, 69, 12000, 7200)
        self.assertGreater(float(np.max(np.abs(rendered))), 0.001)

    def test_cli_default_note_off_matches_explicit_sample_end(self):
        with tempfile.TemporaryDirectory(dir=gp.HERE / "build") as tmp:
            root = Path(tmp)
            target = root / "target.wav"
            gp.write_wav(target, self.audio)
            base = [sys.executable, str(gp.HERE / "genetic_patch.py"), "search", str(target),
                    "--population", "4", "--generations", "1", "--modes", "6"]
            for name, options in (("default", []), ("explicit", ["--note", "69", "--note-off", "0.16", "--no-progress"])):
                result = subprocess.run(base + ["--output", str(root / name)] + options,
                                        capture_output=True, text=True)
                self.assertEqual(result.returncode, 0, result.stderr)
                metadata = json.loads((root / name / "search.json").read_text())
                self.assertEqual(metadata["note_off_seconds"], len(self.audio)/gp.RATE)
                self.assertEqual(metadata["workers"], 8)
                self.assertEqual(metadata["note"], 69)
                self.assertEqual(metadata["pitch"]["method"], "yin_numpy" if name == "default" else "manual")
            self.assertEqual((root / "default/best.wav").read_bytes(),
                             (root / "explicit/best.wav").read_bytes())
            result = subprocess.run([sys.executable, str(gp.HERE / "genetic_patch.py"),
                "--renderer", str(root / "absent.dll"), "export-fui",
                str(root / "default/best_patch.json"), "--output", str(root / "export.fui")],
                capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(fui.decode_fui((root / "export.fui").read_bytes())[0],
                             fui.decode_fui((root / "default/best_patch.fui").read_bytes())[0])


if __name__ == "__main__":
    unittest.main()
