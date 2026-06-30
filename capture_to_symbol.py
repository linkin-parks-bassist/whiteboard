#!/usr/bin/env python3
import argparse
import json
import os
import re


def ident(s):
    s = re.sub(r"[^A-Za-z0-9_]", "_", s)
    if not s or s[0].isdigit():
        s = "_" + s
    return s


def custom_symbol_id_expr(name):
    h = 2166136261
    for ch in name:
        h ^= ord(ch) & 0xff
        h = (h * 16777619) & 0xffffffff
    return f"(WB_SYMBOL_CUSTOM_BASE + {h % 100000})"


def symbol_id_expr(symbol):
    if symbol.startswith("text:") and len(symbol) == 6:
        return f"(WB_SYMBOL_ROMAN_BASE + {ord(symbol[-1])})"
    if symbol.startswith("roman:") and len(symbol) == 7:
        return f"(WB_SYMBOL_ROMAN_BASE + {ord(symbol[-1])})"
    if symbol.startswith("math:") and len(symbol) == 6:
        return str(ord(symbol[-1]))
    if symbol.startswith("mathbb:") and len(symbol) == 8:
        return custom_symbol_id_expr(symbol)
    if symbol.startswith("\\"):
        return custom_symbol_id_expr(symbol[1:])
    commands = {
        "alpha": "WB_SYMBOL_ALPHA",
        "beta": "WB_SYMBOL_BETA",
        "gamma": "WB_SYMBOL_GAMMA",
        "delta": "WB_SYMBOL_DELTA",
        "epsilon": "WB_SYMBOL_EPSILON",
        "theta": "WB_SYMBOL_THETA",
        "lambda": "WB_SYMBOL_LAMBDA",
        "mu": "WB_SYMBOL_MU",
        "pi": "WB_SYMBOL_PI",
        "sigma": "WB_SYMBOL_SIGMA",
        "phi": "WB_SYMBOL_PHI",
        "omega": "WB_SYMBOL_OMEGA",
        "varepsilon": custom_symbol_id_expr("varepsilon"),
        "zeta": custom_symbol_id_expr("zeta"),
        "eta": custom_symbol_id_expr("eta"),
        "vartheta": custom_symbol_id_expr("vartheta"),
        "iota": custom_symbol_id_expr("iota"),
        "kappa": custom_symbol_id_expr("kappa"),
        "nu": custom_symbol_id_expr("nu"),
        "xi": custom_symbol_id_expr("xi"),
        "varpi": custom_symbol_id_expr("varpi"),
        "rho": custom_symbol_id_expr("rho"),
        "varrho": custom_symbol_id_expr("varrho"),
        "varsigma": custom_symbol_id_expr("varsigma"),
        "tau": custom_symbol_id_expr("tau"),
        "upsilon": custom_symbol_id_expr("upsilon"),
        "varphi": custom_symbol_id_expr("varphi"),
        "chi": custom_symbol_id_expr("chi"),
        "psi": custom_symbol_id_expr("psi"),
        "Gamma": custom_symbol_id_expr("Gamma"),
        "Delta": custom_symbol_id_expr("Delta"),
        "Theta": custom_symbol_id_expr("Theta"),
        "Lambda": custom_symbol_id_expr("Lambda"),
        "Xi": custom_symbol_id_expr("Xi"),
        "Pi": custom_symbol_id_expr("Pi"),
        "Sigma": custom_symbol_id_expr("Sigma"),
        "Upsilon": custom_symbol_id_expr("Upsilon"),
        "Phi": custom_symbol_id_expr("Phi"),
        "Psi": custom_symbol_id_expr("Psi"),
        "Omega": custom_symbol_id_expr("Omega"),
        "sum": "WB_SYMBOL_SUM",
        "Sum": "WB_SYMBOL_BIG_SUM",
        "prod": "WB_SYMBOL_PROD",
        "Prod": "WB_SYMBOL_BIG_PROD",
        "int": "WB_SYMBOL_BIG_INT",
        "leq": "WB_SYMBOL_LEQ",
        "geq": "WB_SYMBOL_GEQ",
        "neq": "WB_SYMBOL_NEQ",
        "approx": "WB_SYMBOL_APPROX",
        "to": "WB_SYMBOL_TO",
        "arrow": "WB_SYMBOL_TO",
        "infty": "WB_SYMBOL_INFINITY",
        "infinity": "WB_SYMBOL_INFINITY",
        "cdot": "WB_SYMBOL_CDOT",
        "times": "WB_SYMBOL_TIMES",
        "pm": "WB_SYMBOL_PM",
        "sqrt": "WB_SYMBOL_SQRT",
        "fracbar": "WB_SYMBOL_FRACBAR",
        "partial": "WB_SYMBOL_PARTIAL",
        "nabla": "WB_SYMBOL_NABLA",
        "forall": "WB_SYMBOL_FORALL",
        "exists": "WB_SYMBOL_EXISTS",
        "in": "WB_SYMBOL_IN",
        "notin": "WB_SYMBOL_NOTIN",
        "subset": "WB_SYMBOL_SUBSET",
        "subseteq": "WB_SYMBOL_SUBSETEQ",
        "emptyset": "WB_SYMBOL_EMPTYSET",
        "cup": "WB_SYMBOL_CUP",
        "cap": "WB_SYMBOL_CAP",
        "sim": "WB_SYMBOL_SIM",
        "equiv": "WB_SYMBOL_EQUIV",
        "propto": "WB_SYMBOL_PROPORTO",
        "perp": "WB_SYMBOL_PERP",
        "parallel": "WB_SYMBOL_PARALLEL",
    }
    if symbol in commands:
        return commands[symbol]
    if len(symbol) == 1:
        return str(ord(symbol))
    return custom_symbol_id_expr(symbol)


SYMBOL_METRICS = {
    "fracbar": (0.06, -0.56),
    "+": (0.55, -0.62),
    "-": (0.16, -0.42),
    "=": (0.32, -0.48),
    "<": (0.55, -0.62),
    ">": (0.55, -0.62),
    "leq": (0.68, -0.64),
    "geq": (0.68, -0.64),
    "neq": (0.50, -0.58),
    "approx": (0.34, -0.50),
    "cdot": (0.12, -0.42),
    "times": (0.48, -0.58),
    "pm": (0.62, -0.66),
    ".": (0.12, -0.08),
    ",": (0.22, -0.08),
    ":": (0.42, -0.50),
    ";": (0.48, -0.50),
    "(": (0.92, -0.82),
    ")": (0.92, -0.82),
    "[": (0.92, -0.82),
    "]": (0.92, -0.82),
    "sum": (0.92, -0.82),
    "int": (2.90, -2.36),
    "sqrt": (0.95, -0.86),
    "infinity": (0.38, -0.52),
    "arrow": (0.24, -0.42),
    "to": (0.24, -0.42),
}

HORIZONTAL_SYMBOL_METRICS = {
    "-": (0.52, -0.42),
}

CENTERED_SYMBOL_METRICS = {
    "int": (2.90, -0.42),
}

ASCENDERS = set("bdfhklt")
DESCENDERS = set("gjpqy")


def capture_metrics(symbol):
    is_math_letter = symbol.startswith("math:") and len(symbol) == 6
    if symbol.startswith("text:") or symbol.startswith("roman:") or symbol.startswith("math:"):
        symbol = symbol.split(":", 1)[1]
    if symbol.startswith("mathbb:"):
        symbol = symbol.split(":", 1)[1]
    if symbol.startswith("\\"):
        symbol = symbol[1:]
    if symbol in SYMBOL_METRICS:
        return SYMBOL_METRICS[symbol]
    if len(symbol) == 1 and symbol.isdigit():
        return (0.96, -0.88)
    if len(symbol) == 1 and symbol.isupper():
        return (0.82, -0.82)
    if len(symbol) == 1 and symbol.islower():
        if symbol in ASCENDERS:
            return (0.80 if is_math_letter else 0.82, -0.82)
        if symbol in DESCENDERS:
            return (1.65 if is_math_letter else 1.72, -0.58)
        if symbol == "i":
            return (0.62 if is_math_letter else 0.66, -0.66)
        return (0.54 if is_math_letter else 0.58, -0.58)
    return (0.78, -0.76)


def distance(a, b):
    return ((a["x"] - b["x"]) ** 2 + (a["y"] - b["y"]) ** 2) ** 0.5


def resample_stroke(stroke, spacing):
    if len(stroke) < 2 or spacing <= 0:
        return [dict(p) for p in stroke]
    out = [stroke[0]]
    acc = 0.0
    prev = stroke[0]
    i = 1
    while i < len(stroke):
        cur = stroke[i]
        d = distance(prev, cur)
        if d + acc >= spacing and d > 0:
            t = (spacing - acc) / d
            np = {
                "x": prev["x"] + (cur["x"] - prev["x"]) * t,
                "y": prev["y"] + (cur["y"] - prev["y"]) * t,
                "p": prev.get("p", 0.5) + (cur.get("p", 0.5) - prev.get("p", 0.5)) * t,
                "t": prev.get("t", 0.0) + (cur.get("t", 0.0) - prev.get("t", 0.0)) * t,
            }
            out.append(np)
            prev = np
            acc = 0.0
        else:
            acc += d
            prev = cur
            i += 1
    if out[-1] is not stroke[-1]:
        out.append(stroke[-1])
    return out


def smooth_stroke(stroke, iterations):
    pts = [dict(p) for p in stroke]
    if len(pts) < 3:
        return pts
    for _ in range(iterations):
        nxt = [pts[0]]
        for i in range(1, len(pts) - 1):
            q = dict(pts[i])
            q["x"] = 0.25 * pts[i - 1]["x"] + 0.50 * pts[i]["x"] + 0.25 * pts[i + 1]["x"]
            q["y"] = 0.25 * pts[i - 1]["y"] + 0.50 * pts[i]["y"] + 0.25 * pts[i + 1]["y"]
            nxt.append(q)
        nxt.append(pts[-1])
        pts = nxt
    return pts


def catmull_rom_beziers(points, tension=0.85):
    curves = []
    if len(points) < 2:
        return curves
    pts = [(float(p["x"]), float(p["y"])) for p in points]
    for i in range(len(pts) - 1):
        p0 = pts[i - 1] if i > 0 else pts[i]
        p1 = pts[i]
        p2 = pts[i + 1]
        p3 = pts[i + 2] if i + 2 < len(pts) else pts[i + 1]
        c1 = (p1[0] + (p2[0] - p0[0]) * tension / 6.0, p1[1] + (p2[1] - p0[1]) * tension / 6.0)
        c2 = (p2[0] - (p3[0] - p1[0]) * tension / 6.0, p2[1] - (p3[1] - p1[1]) * tension / 6.0)
        curves.append((p1, c1, c2, p2))
    return curves


def normalise_capture(capture):
    symbol = capture.get("symbol", "symbol")
    target_height, target_min_y = capture_metrics(symbol)
    pts = [p for stroke in capture["strokes"] for p in stroke]
    min_x = min(p["x"] for p in pts)
    max_x = max(p["x"] for p in pts)
    min_y = min(p["y"] for p in pts)
    max_y = max(p["y"] for p in pts)
    if symbol in CENTERED_SYMBOL_METRICS:
        target_height, target_center_y = CENTERED_SYMBOL_METRICS[symbol]
        scale = target_height / max(max_y - min_y, 1.0)
        source_center_y = (min_y + max_y) * 0.5
        return [
            [
                {
                    **p,
                    "x": (p["x"] - min_x) * scale,
                    "y": target_center_y + (p["y"] - source_center_y) * scale,
                }
                for p in stroke
            ]
            for stroke in capture["strokes"]
        ]

    if symbol in HORIZONTAL_SYMBOL_METRICS:
        target_width, target_center_y = HORIZONTAL_SYMBOL_METRICS[symbol]
        scale = target_width / max(max_x - min_x, 1.0)
        source_center_y = (min_y + max_y) * 0.5
        return [
            [
                {
                    **p,
                    "x": (p["x"] - min_x) * scale,
                    "y": target_center_y + (p["y"] - source_center_y) * scale,
                }
                for p in stroke
            ]
            for stroke in capture["strokes"]
        ]
    h = max(max_y - min_y, 1.0)
    scale = target_height / h
    norm = []
    for stroke in capture["strokes"]:
        ns = []
        for p in stroke:
            q = dict(p)
            q["x"] = (p["x"] - min_x) * scale
            q["y"] = target_min_y + (p["y"] - min_y) * scale
            ns.append(q)
        norm.append(ns)
    return norm


def capture_strokes(capture, normalize):
    if normalize:
        return normalise_capture(capture)
    return [[dict(p) for p in stroke] for stroke in capture["strokes"]]


def emit_curve(prefix, idx, curve):
    degree = 3
    n = 4
    knots = "0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f"
    xs = ", ".join(f"{p[0]:.6f}f" for p in curve)
    ys = ", ".join(f"{p[1]:.6f}f" for p in curve)
    out = []
    out.append(f"static float {prefix}_x_{idx}[4] = {{ {xs} }};")
    out.append(f"static float {prefix}_y_{idx}[4] = {{ {ys} }};")
    out.append(f"static float {prefix}_w_{idx}[4] = {{ 1.0f, 1.0f, 1.0f, 1.0f }};")
    out.append(f"static float {prefix}_k_{idx}[8] = {{ {knots} }};")
    out.append(f"static wb_nurbs {prefix}_nx_{idx} = {{ {degree}, {n}, 8, {prefix}_k_{idx}, {prefix}_x_{idx}, {prefix}_w_{idx} }};")
    out.append(f"static wb_nurbs {prefix}_ny_{idx} = {{ {degree}, {n}, 8, {prefix}_k_{idx}, {prefix}_y_{idx}, {prefix}_w_{idx} }};")
    out.append(f"static wb_nurbs_pcurve {prefix}_curve_{idx} = {{ &{prefix}_nx_{idx}, &{prefix}_ny_{idx}, 0 }};")
    return "\n".join(out), f"&{prefix}_curve_{idx}"


def emit_capture(path, args, capture_index=None):
    capture = path if isinstance(path, dict) else json.load(open(path))
    default_symbol = "symbol" if isinstance(path, dict) else os.path.splitext(os.path.basename(path))[0]
    symbol = capture.get("symbol", default_symbol)
    variant = int(capture.get("variant", 0))
    suffix = f"_{capture_index}" if capture_index is not None else ""
    prefix = ident(f"captured_{symbol}_{variant}{suffix}")
    strokes = capture_strokes(capture, args.normalize)
    pts = [p for stroke in strokes for p in stroke]
    min_x = min(p["x"] for p in pts)
    max_x = max(p["x"] for p in pts)
    min_y = min(p["y"] for p in pts)
    max_y = max(p["y"] for p in pts)
    advance = max(0.25, max_x + 0.16)
    source = capture.get("_source", path if isinstance(path, str) else "batch")
    out = [f"/* captured symbol {symbol} variant {variant} from {source} */"]
    ptrs = []
    curve_idx = 0
    for stroke in strokes:
        stroke = resample_stroke(stroke, args.spacing)
        stroke = smooth_stroke(stroke, args.smooth)
        for curve in catmull_rom_beziers(stroke, args.tension):
            c, p = emit_curve(prefix, curve_idx, curve)
            out.append(c)
            ptrs.append(p)
            curve_idx += 1
    out.append(f"static wb_nurbs_pcurve *{prefix}_curves[{len(ptrs)}] = {{ {', '.join(ptrs)} }};")
    out.append(f"wb_plane_figure {prefix}_figure = {{ {len(ptrs)}, {prefix}_curves }};")
    out.append(f"static wb_captured_symbol_variant {prefix}_variant = {{ &{prefix}_figure, {advance:.6f}f, {min_x:.6f}f, {min_y:.6f}f, {max_x:.6f}f, {max_y:.6f}f }};")
    return "\n".join(out), prefix, symbol


def emit_registry(entries):
    grouped = {}
    for symbol, prefix in entries:
        grouped.setdefault(symbol, []).append(prefix)

    out = []
    out.append("typedef struct")
    out.append("{")
    out.append("\tint symbol_id;")
    out.append("\tint n_variants;")
    out.append("\twb_captured_symbol_variant **variants;")
    out.append("} wb_captured_symbol_entry;")
    out.append("")

    table_ptrs = []
    for idx, (symbol, prefixes) in enumerate(grouped.items()):
        arr = f"captured_symbol_variants_{idx}"
        table_ptrs.append((symbol, arr, len(prefixes)))
        out.append(f"static wb_captured_symbol_variant *{arr}[{len(prefixes)}] = {{ " + ", ".join(f"&{p}_variant" for p in prefixes) + " };")

    out.append("")
    out.append(f"static wb_captured_symbol_entry captured_symbol_table[{len(table_ptrs)}] = {{")
    for symbol, arr, count in table_ptrs:
        out.append(f"\t{{ {symbol_id_expr(symbol)}, {count}, {arr} }},")
    out.append("};")
    out.append("")
    out.append("const wb_captured_symbol_variant *wb_get_captured_symbol_variant(int symbol_id, int variant_seed)")
    out.append("{")
    out.append("\tint n_symbols = (int)(sizeof(captured_symbol_table) / sizeof(captured_symbol_table[0]));")
    out.append("\tfor (int i = 0; i < n_symbols; i++)")
    out.append("\t{")
    out.append("\t\tif (captured_symbol_table[i].symbol_id != symbol_id || captured_symbol_table[i].n_variants <= 0)")
    out.append("\t\t\tcontinue;")
    out.append("\t\tuint32_t pick = (uint32_t)variant_seed;")
    out.append("\t\tpick ^= pick >> 16;")
    out.append("\t\tpick *= 0x7feb352dU;")
    out.append("\t\tpick ^= pick >> 15;")
    out.append("\t\tpick *= 0x846ca68bU;")
    out.append("\t\tpick ^= pick >> 16;")
    out.append("\t\treturn captured_symbol_table[i].variants[pick % captured_symbol_table[i].n_variants];")
    out.append("\t}")
    out.append("\treturn NULL;")
    out.append("}")
    out.append("")
    out.append("wb_plane_figure *wb_get_captured_symbol_figure(int symbol_id, int variant_seed)")
    out.append("{")
    out.append("\tconst wb_captured_symbol_variant *variant = wb_get_captured_symbol_variant(symbol_id, variant_seed);")
    out.append("\treturn variant ? variant->figure : NULL;")
    out.append("}")
    return "\n".join(out)


def load_captures(path):
    data = json.load(open(path))
    if isinstance(data, dict) and "captures" in data:
        out = []
        for cap in data["captures"]:
            cap["_source"] = path
            out.append(cap)
        return out
    if isinstance(data, list):
        for cap in data:
            if isinstance(cap, dict):
                cap["_source"] = path
        return data
    data["_source"] = path
    return [data]


def filter_captures(captures, max_variants):
    if max_variants <= 0:
        return captures
    counts = {}
    out = []
    for capture in captures:
        symbol = capture.get("symbol", "symbol")
        n = counts.get(symbol, 0)
        if n >= max_variants:
            continue
        counts[symbol] = n + 1
        out.append(capture)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("captures", nargs="+")
    parser.add_argument("--out", default="generated/wb_captured_symbols.c")
    parser.add_argument("--spacing", type=float, default=0.0)
    parser.add_argument("--smooth", type=int, default=0)
    parser.add_argument("--tension", type=float, default=0.85)
    parser.add_argument("--max-variants", type=int, default=1)
    parser.add_argument("--normalize", action="store_true")
    args = parser.parse_args()
    chunks = ['#include "whiteboard.h"', '#include "wb_captured_symbols.h"', ""]
    names = []
    registry = []
    capture_index = 0
    for path in args.captures:
        for capture in filter_captures(load_captures(path), args.max_variants):
            code, name, symbol = emit_capture(capture, args, capture_index)
            chunks.append(code)
            chunks.append("")
            names.append(name)
            registry.append((symbol, name))
            capture_index += 1
    chunks.append(emit_registry(registry))
    chunks.append("")
    with open(args.out, "w") as f:
        f.write("\n".join(chunks))
    print(f"wrote {args.out}")
    for name in names:
        print(f"{name}_figure")


if __name__ == "__main__":
    main()
