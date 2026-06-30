#!/usr/bin/env python3
import argparse
import json
import math


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
    "infty": (0.38, -0.52),
}

HORIZONTAL_SYMBOL_METRICS = {
    "-": (0.52, -0.42),
}

CENTERED_SYMBOL_METRICS = {
    "int": (2.90, -0.42),
}

ASCENDERS = set("bdfhklt")
DESCENDERS = set("gjpqy")
EXCLUDED_SYMBOLS = {"Int"}


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
    return math.hypot(a["x"] - b["x"], a["y"] - b["y"])


def point_line_distance(p, a, b):
    dx = b["x"] - a["x"]
    dy = b["y"] - a["y"]
    denom = dx * dx + dy * dy
    if denom <= 1e-12:
        return distance(p, a)
    t = ((p["x"] - a["x"]) * dx + (p["y"] - a["y"]) * dy) / denom
    t = max(0.0, min(1.0, t))
    q = {"x": a["x"] + t * dx, "y": a["y"] + t * dy}
    return distance(p, q)


def rdp(stroke, eps):
    if len(stroke) <= 2:
        return [dict(p) for p in stroke]
    start = stroke[0]
    end = stroke[-1]
    max_dist = -1.0
    max_i = 0
    for i in range(1, len(stroke) - 1):
        d = point_line_distance(stroke[i], start, end)
        if d > max_dist:
            max_dist = d
            max_i = i
    if max_dist <= eps:
        return [dict(start), dict(end)]
    return rdp(stroke[:max_i + 1], eps)[:-1] + rdp(stroke[max_i:], eps)


def resample_stroke(stroke, spacing):
    if len(stroke) < 2 or spacing <= 0:
        return [dict(p) for p in stroke]
    out = [dict(stroke[0])]
    prev = dict(stroke[0])
    acc = 0.0
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
            prev = dict(cur)
            i += 1
    if distance(out[-1], stroke[-1]) > 1e-6:
        out.append(dict(stroke[-1]))
    return out


def redistribute_stroke(stroke):
    n = len(stroke)
    if n <= 2:
        return [dict(p) for p in stroke]

    lengths = [0.0]
    for i in range(1, n):
        lengths.append(lengths[-1] + distance(stroke[i - 1], stroke[i]))

    total = lengths[-1]
    if total <= 1e-9:
        return [dict(p) for p in stroke]

    out = []
    seg = 1
    for k in range(n):
        target = total * k / (n - 1)
        while seg < n - 1 and lengths[seg] < target:
            seg += 1

        a = stroke[seg - 1]
        b = stroke[seg]
        span = max(lengths[seg] - lengths[seg - 1], 1e-9)
        t = (target - lengths[seg - 1]) / span
        out.append({
            **a,
            "x": a["x"] + (b["x"] - a["x"]) * t,
            "y": a["y"] + (b["y"] - a["y"]) * t,
            "p": a.get("p", 0.5) + (b.get("p", 0.5) - a.get("p", 0.5)) * t,
            "t": a.get("t", 0.0) + (b.get("t", 0.0) - a.get("t", 0.0)) * t,
        })
    return out


def smooth_stroke(stroke, iterations, endpoint_weight=1.0):
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
        if endpoint_weight < 1.0:
            nxt[0]["x"] = endpoint_weight * pts[0]["x"] + (1 - endpoint_weight) * nxt[1]["x"]
            nxt[0]["y"] = endpoint_weight * pts[0]["y"] + (1 - endpoint_weight) * nxt[1]["y"]
            nxt[-1]["x"] = endpoint_weight * pts[-1]["x"] + (1 - endpoint_weight) * nxt[-2]["x"]
            nxt[-1]["y"] = endpoint_weight * pts[-1]["y"] + (1 - endpoint_weight) * nxt[-2]["y"]
        pts = nxt
    return pts


def normalize_capture(capture):
    symbol = capture.get("symbol", "symbol")
    target_height, target_min_y = capture_metrics(symbol)
    pts = [p for stroke in capture["strokes"] for p in stroke]
    min_x = min(p["x"] for p in pts)
    max_x = max(p["x"] for p in pts)
    min_y = min(p["y"] for p in pts)
    max_y = max(p["y"] for p in pts)
    if symbol in CENTERED_SYMBOL_METRICS:
        target_height, target_center_y = CENTERED_SYMBOL_METRICS[symbol]
        scale = target_height / max(max_y - min_y, 1e-6)
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
        scale = target_width / max(max_x - min_x, 1e-6)
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

    h = max(max_y - min_y, 1e-6)
    scale = target_height / h
    strokes = []
    for stroke in capture["strokes"]:
        strokes.append([
            {
                **p,
                "x": (p["x"] - min_x) * scale,
                "y": target_min_y + (p["y"] - min_y) * scale,
            }
            for p in stroke
        ])
    return strokes


def process_capture(capture, args):
    out = dict(capture)
    out["strokes_raw_count"] = [len(s) for s in capture.get("strokes", [])]
    out["strokes"] = normalize_capture(capture) if args.normalize else [[dict(p) for p in s] for s in capture["strokes"]]

    processed = []
    for stroke in out["strokes"]:
        stroke = resample_stroke(stroke, args.resample)
        stroke = smooth_stroke(stroke, args.smooth)
        if args.simplify > 0:
            stroke = rdp(stroke, args.simplify)
        stroke = redistribute_stroke(stroke)
        if len(stroke) >= 2:
            processed.append(stroke)

    out["strokes"] = processed
    out["capture_order"] = list(range(len(processed)))
    out["processed"] = {
        "normalize": args.normalize,
        "resample": args.resample,
        "smooth": args.smooth,
        "simplify": args.simplify,
    }
    return out


def load_captures(path):
    data = json.load(open(path))
    if isinstance(data, dict) and "captures" in data:
        return data, data["captures"]
    if isinstance(data, list):
        return {"created_at": None}, data
    return {"created_at": None}, [data]


def filter_variants(captures, max_variants):
    if max_variants <= 0:
        return captures
    counts = {}
    out = []
    for cap in captures:
        symbol = cap.get("symbol", "symbol")
        if symbol in EXCLUDED_SYMBOLS:
            continue
        n = counts.get(symbol, 0)
        if n >= max_variants:
            continue
        counts[symbol] = n + 1
        out.append(cap)
    return out


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("capture_json")
    parser.add_argument("--out", required=True)
    parser.add_argument("--max-variants", type=int, default=1)
    parser.add_argument("--resample", type=float, default=0.040)
    parser.add_argument("--smooth", type=int, default=6)
    parser.add_argument("--simplify", type=float, default=0.032)
    parser.add_argument("--no-normalize", action="store_false", dest="normalize")
    args = parser.parse_args()

    meta, captures = load_captures(args.capture_json)
    captures = filter_variants(captures, args.max_variants)
    processed = [process_capture(c, args) for c in captures]
    payload = {
        "created_at": meta.get("created_at"),
        "source": args.capture_json,
        "count": len(processed),
        "processed": {
            "max_variants": args.max_variants,
            "normalize": args.normalize,
            "resample": args.resample,
            "smooth": args.smooth,
            "simplify": args.simplify,
        },
        "captures": processed,
    }
    with open(args.out, "w") as f:
        json.dump(payload, f, indent=2)
    print(f"wrote {args.out} ({len(processed)} captures)")


if __name__ == "__main__":
    main()
