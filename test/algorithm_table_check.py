"""Check the plugin's copies of the 3HS816 algorithm routing against the core.

1. The routing diagram (src/ui/AlgorithmDiagram.h) keeps its own copy of the
   core's kAlgorithms, because the core declares it in an anonymous namespace.
   The two must match.
2. Velocity scaling (src/VolumeScalingMap.h) must scale the operators that
   reach the output: exactly those when the groups add, and a non-empty subset
   of them when the groups multiply (scaling one factor of a product is enough).

Run this after copying a new core from CPT32.
"""
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
CORE = ROOT / "src/s3hs_core/3hs/3hs816/src/channel.cpp"
UI = ROOT / "src/ui/AlgorithmDiagram.h"
SCALING = ROOT / "src/VolumeScalingMap.h"


def table_text(text):
    start = text.index("constexpr Group kAllAdditive")
    end = text.index("};", text.index("kAlgorithms[kAlgorithmCount]")) + 2
    body = re.sub(r"/\*.*?\*/", "", text[start:end])  # 行ごとの説明は比べない
    return re.sub(r"\s+", "", body)


def algorithms(text):
    """[(multiply, [[stage masks] per group])] for the 13 modes."""
    body = table_text(text)
    groups = dict(re.findall(r"constexprGroup(\w+)=(\{\d+,\{[^}]*\}\});", body))
    rows = body[body.index("kAlgorithms[kAlgorithmCount]={") + len("kAlgorithms[kAlgorithmCount]={"):-2]
    for name, value in groups.items():
        rows = re.sub(rf"\b{name}\b", value, rows)
    parsed = eval("[" + rows.replace("{", "[").replace("}", "]").replace("true", "True").replace("false", "False") + "]")
    return [(multiply, [stages[:count] for count, stages in grps[:n]]) for multiply, _direct, n, grps in parsed]


core_text = CORE.read_text(encoding="utf-8")
if table_text(core_text) != table_text(UI.read_text(encoding="utf-8")):
    print("FAIL: src/ui/AlgorithmDiagram.h no longer matches the core's kAlgorithms")
    sys.exit(1)
print("PASS: algorithm table matches the 3HS816 core")

scaling = []
for row in re.findall(r"\{([y_,]+)\},\s*//", SCALING.read_text(encoding="utf-8")):
    scaling.append(sum(1 << i for i, c in enumerate(row.split(",")) if c == "y"))

modes = algorithms(core_text)
failed = len(modes) != 13 or len(scaling) != 13
for mode, (multiply, grps) in enumerate(modes):
    outputs = 0
    for stages in grps:
        outputs |= stages[-1]
    scaled = scaling[mode]
    ok = (scaled != 0 and scaled & ~outputs == 0) if multiply else scaled == outputs
    if not ok:
        print(f"FAIL: mode {mode} scales operators {scaled:#04x}, but the outputs are {outputs:#04x}")
        failed = True
if failed:
    sys.exit(1)
print("PASS: velocity scaling follows the output operators in all 13 modes")
