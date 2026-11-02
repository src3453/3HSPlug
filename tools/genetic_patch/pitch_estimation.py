"""Single-note pitch estimation using NumPy and the YIN difference function.

Reference: de Cheveigne and Kawahara, JASA 111 (2002), 1917-1930.
This implementation uses fixed-length difference windows, FFT correlation,
parabolic period refinement, and energy-weighted aggregation over active frames.
It does not call librosa or its probabilistic pYIN tracker.
"""
import math

import numpy as np


def _weighted_median(values, weights):
    order = np.argsort(values, kind="stable")
    return float(values[order[np.searchsorted(np.cumsum(weights[order]), weights.sum()/2)]])


def _frame_pitch(frame, sample_rate, fmin, fmax):
    frame = np.asarray(frame, dtype=np.float64)
    frame = frame - frame.mean()
    max_lag = min(int(np.ceil(sample_rate/fmin)) + 1, len(frame)//2)
    min_lag = max(2, int(np.floor(sample_rate/fmax)))
    if max_lag <= min_lag + 1:
        return None
    width = len(frame) - max_lag
    fft_size = 1 << (len(frame)+width-2).bit_length()
    # Correlate the same prefix window with every shifted window. Fixed width
    # avoids making larger lags look better merely because fewer samples overlap.
    correlation = np.fft.irfft(np.conj(np.fft.rfft(frame[:width], fft_size)) *
                              np.fft.rfft(frame, fft_size), fft_size)[:max_lag+1]
    energy = np.concatenate(([0.0], np.cumsum(frame*frame)))
    lags = np.arange(max_lag+1)
    difference = np.maximum(energy[width] + energy[width+lags]-energy[lags]-2*correlation, 0)
    normalized = np.ones(max_lag+1)
    normalized[1:] = difference[1:] * lags[1:] / np.maximum(np.cumsum(difference[1:]), 1e-20)
    minima = np.flatnonzero((normalized[1:-1] < normalized[:-2]) &
                            (normalized[1:-1] <= normalized[2:])) + 1
    for lag in minima[minima >= min_lag]:
        left, middle, right = normalized[lag-1:lag+2]
        curvature = left - 2*middle + right
        shift = 0.0 if curvature <= 1e-20 else float(np.clip(0.5*(left-right)/curvature, -0.5, 0.5))
        # Refine before thresholding: high notes can have their true minimum
        # between samples, while the double-period minimum falls on a sample.
        trough = middle - 0.25*(left-right)*shift
        frequency = sample_rate / (lag+shift)
        if trough < 0.1 and fmin <= frequency <= fmax:
            return frequency, float(np.clip(1-trough, 0, 1))
    return None  # do not turn noise into a pitch by using a global minimum


def estimate_pitch(audio, sample_rate=48000, fmin=20.0, fmax=5000.0):
    """Return estimated Hz, rounded MIDI note, and diagnostics for one pitched note.

    Ignore quiet tails, analyze at most 64 frames, and reject unstable/nonperiodic
    recordings instead of silently assigning an arbitrary MIDI note.
    """
    failure = "Could not estimate a stable pitch; specify --note explicitly"
    audio = np.asarray(audio, dtype=np.float64)
    if (audio.ndim != 1 or len(audio) < 256 or not np.isfinite(audio).all() or
            not 0 < fmin < fmax < sample_rate/2):
        raise ValueError(failure)
    # Reject DC as well as silence; DC is not a low-frequency musical note.
    audio = audio - audio.mean()
    length = min(8192, len(audio))
    hop = max(1, length//4)
    starts = np.arange(0, len(audio)-length+1, hop)
    energy = np.concatenate(([0.0], np.cumsum(audio*audio)))
    integral = np.concatenate(([0.0], np.cumsum(audio)))
    means = (integral[starts+length]-integral[starts])/length
    rms = np.sqrt(np.maximum((energy[starts+length]-energy[starts])/length-means*means, 0))
    if np.max(rms) < 1e-6:
        raise ValueError(failure)
    active = np.flatnonzero(rms >= np.max(rms)*0.1)
    if len(active) > 64:
        active = active[np.linspace(0, len(active)-1, 64).astype(int)]
    estimates, periodicities, weights = [], [], []
    for index in active:
        frame = audio[starts[index]:starts[index]+length]
        result = _frame_pitch(frame, sample_rate, fmin, fmax)
        if result is not None:
            frequency, periodicity = result
            estimates.append(69 + 12*np.log2(frequency/440))
            periodicities.append(periodicity)
            weights.append(float(rms[index]**2))
    if not estimates:
        raise ValueError(failure)
    estimates, weights = np.array(estimates), np.array(weights)
    periodic_fraction = float(weights.sum()/np.sum(rms[active]**2))
    center = _weighted_median(estimates, weights)
    inliers = np.abs(estimates-center) <= 0.5
    consistency = float(weights[inliers].sum()/weights.sum())
    if periodic_fraction < 0.5 or consistency < 0.65:
        raise ValueError(failure)
    midi = _weighted_median(estimates[inliers], weights[inliers])
    note = int(math.floor(midi+0.5))
    if not 0 <= note <= 127:
        raise ValueError(failure)
    return {"method": "yin_numpy", "frequency_hz": float(440 * 2**((midi-69)/12)),
            "midi_float": midi, "note": note, "cents_from_note": float(100*(midi-note)),
            "periodicity": float(np.average(np.array(periodicities)[inliers], weights=weights[inliers])),
            "periodic_energy_fraction": periodic_fraction, "consistency": consistency,
            "periodic_frames": len(estimates), "analyzed_frames": len(active),
            "range_hz": [fmin, fmax]}
