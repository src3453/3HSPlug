"""S3HS Furnace instrument codec shared by the patch editor and patch search.

Layout: Furnace papers/newIns.md, instrument.cpp writeFeature3H/readFeature3H.
Legacy 64-byte payloads default to feedback operator 1; a 65th byte stores fbDest.
"""
import struct
from atomic_file import atomic_write_bytes

FURNACE_VERSION = 249
S3HS_TYPE = 68
OP_FIELDS = ("volume", "attack", "decay", "sustain", "release")


def _integer(value, lo, hi, name):
    if type(value) is not int or not lo <= value <= hi:
        raise ValueError(f"{name} must be an integer in {lo}..{hi}")
    return value


def encode_s3hs_feature(patch):
    if patch.get("keyShift", 0) != 0:
        raise ValueError("3H FUI export requires keyShift=0")
    ops = patch["operators"]
    if len(ops) != 8:
        raise ValueError("S3HS FUI requires exactly 8 operators")
    frequencies = [_integer(op.get("frequency", 0), 0, 65535, "frequency") for op in ops[1:]]
    data = bytearray(f >> 8 for f in frequencies)
    data.extend(f & 255 for f in frequencies)
    data.extend(_integer(op["waveform"], 0, 15, "waveform") for op in ops)
    for op in ops:
        data.extend(_integer(op[field], 0, 255, field) for field in OP_FIELDS)
    data.append(_integer(patch["modmode"], 0, 12, "modmode"))
    # Plugin 0..255 -> Furnace signed -128..127 (two's-complement byte).
    data.append((_integer(patch["feedback"], 0, 255, "feedback") - 128) & 255)
    if "fbDest" in patch:
        data.append(_integer(patch["fbDest"], 0, 7, "fbDest"))
    return bytes(data)


def decode_s3hs_feature(data):
    if len(data) not in (64, 65):
        raise ValueError("S3HS 3H feature must contain 64 or 65 bytes")
    ops = []
    for i in range(8):
        op = dict(zip(OP_FIELDS, data[22+i*5:27+i*5]))
        op["waveform"] = data[14+i]
        op["frequency"] = 0 if i == 0 else (data[i-1] << 8) | data[i+6]
        ops.append(op)
    signed_feedback = struct.unpack_from("b", data, 63)[0]
    patch = {"program": -1, "modmode": data[62], "feedback": signed_feedback + 128,
             "keyShift": 0, "operators": ops}
    if len(data) == 65:
        patch["fbDest"] = data[64]
    encode_s3hs_feature(patch)  # validate ranges before passing to the editor
    return patch


def encode_fui(patch, name="3HS patch"):
    if not isinstance(name, str) or "\0" in name:
        raise ValueError("instrument name must be text without a NUL character")
    name_data = name.encode("utf-8") + b"\0"
    if len(name_data) > 65535:
        raise ValueError("instrument name is too long")
    payload = encode_s3hs_feature(patch)
    return (b"FINS" + struct.pack("<HH", FURNACE_VERSION, S3HS_TYPE)
            + b"NA" + struct.pack("<H", len(name_data)) + name_data
            + b"3H" + struct.pack("<H", len(payload)) + payload + b"EN")


def decode_fui(data):
    if len(data) < 8 or data[:4] not in (b"FINS", b"FINB"):
        raise ValueError("expected a Furnace FINS instrument")
    if struct.unpack_from("<H", data, 6)[0] != S3HS_TYPE:
        raise ValueError("FUI instrument is not S3HS (type 68)")
    pos, payload, name = 8, None, ""
    while pos + 2 <= len(data):
        code = data[pos:pos+2]
        pos += 2
        if code == b"EN":
            if payload is None:
                raise ValueError("FUI has no S3HS 3H feature")
            return decode_s3hs_feature(payload), name
        if pos + 2 > len(data):
            raise ValueError("truncated FUI feature length")
        length = struct.unpack_from("<H", data, pos)[0]
        pos += 2
        if pos + length > len(data):
            raise ValueError("truncated FUI feature data")
        feature = data[pos:pos+length]
        pos += length
        if code == b"3H":
            payload = feature
        elif code == b"NA":
            name = feature.split(b"\0", 1)[0].decode("utf-8", errors="replace")
    # Furnace also terminates standalone instruments at EOF when no assets follow.
    if pos == len(data) and payload is not None:
        return decode_s3hs_feature(payload), name
    raise ValueError("truncated FUI or missing S3HS feature")


def write_fui(path, patch, name="3HS patch"):
    atomic_write_bytes(path, encode_fui(patch, name))
