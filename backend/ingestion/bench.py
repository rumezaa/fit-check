"""What the ingestion pipeline costs on this machine.

Standalone on purpose: numpy and onnxruntime are the only hard requirements, so
this file can be copied to a bare pi and run before the rest of the backend is
installed. That is the point - it measures the floor we would be left with if
we dropped rembg, not what the current venv happens to weigh.

    python3 bench.py                    # inference + the viewfinder proxy
    python3 bench.py --photo shirt.png  # also time one real end-to-end capture
    python3 bench.py --model u2netp.onnx

Pillow is optional and only needed for --photo.
"""

import argparse
import os
import platform
import resource
import sys
import time
import urllib.request
from pathlib import Path

import numpy as np

try:
    import onnxruntime as ort
except ImportError:
    sys.exit("onnxruntime is required: pip install onnxruntime")

MODEL_URL = "https://github.com/danielgatis/rembg/releases/download/v0.0.0/u2netp.onnx"
# where rembg keeps it, so a machine that has already run the app reuses it
CACHED = Path.home() / ".rembg" / "models" / "u2netp" / "u2netp.onnx"
SIZE = 320  # u2netp's onnx input is fixed at 1x3x320x320 - smaller is rejected
REPEATS = 10


def peak_rss_mb():
    """ru_maxrss is bytes on macOS and kilobytes on linux."""
    raw = resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
    return raw / 1048576 if sys.platform == "darwin" else raw / 1024


def find_model(explicit):
    if explicit:
        path = Path(explicit).expanduser()
        if not path.exists():
            sys.exit(f"no model at {path}")
        return path
    if CACHED.exists():
        return CACHED
    print(f"fetching u2netp (4.6MB) -> {CACHED}")
    CACHED.parent.mkdir(parents=True, exist_ok=True)
    urllib.request.urlretrieve(MODEL_URL, CACHED)  # noqa: S310 - pinned url
    return CACHED


def session(model, threads):
    opts = ort.SessionOptions()
    opts.intra_op_num_threads = threads
    opts.inter_op_num_threads = 1
    return ort.InferenceSession(
        str(model), sess_options=opts, providers=["CPUExecutionProvider"]
    )


def median(times):
    return sorted(times)[len(times) // 2]


def time_it(fn, repeats=REPEATS):
    fn()  # discard the first, which pays for lazily allocated arenas
    return median([_one(fn) for _ in range(repeats)])


def _one(fn):
    start = time.perf_counter()
    fn()
    return time.perf_counter() - start


def bench_inference(model):
    """The cost of one u2netp pass, against the thread count it is given.

    Swept rather than measured once because it is what says whether a
    viewfinder could run the net per frame and still leave a core for the
    camera and the ui.
    """
    cores = os.cpu_count() or 1
    counts = sorted({1, 2, 4, cores})
    x = np.random.rand(1, 3, SIZE, SIZE).astype(np.float32)

    print(f"\ninference - u2netp @ {SIZE}x{SIZE}")
    print(f"  {'threads':>8}  {'per frame':>11}  {'fps':>6}")
    results = {}
    for n in counts:
        if n > cores:
            continue
        sess = session(model, n)
        name = sess.get_inputs()[0].name
        secs = time_it(lambda: sess.run(None, {name: x}))
        results[n] = secs
        print(f"  {n:>8}  {secs * 1000:>8.1f} ms  {1 / secs:>6.1f}")
    return results


def bench_proxy():
    """The cheap in-view signal: edge density and centre-vs-border contrast.

    No model, no matte - just enough to tell the user something garment shaped
    is filling the frame, which is all the viewfinder actually has to say.
    """
    frame = (np.random.rand(480, 640, 3) * 255).astype(np.uint8).astype(np.int16)

    def proxy():
        grey = frame.mean(2)
        edges = np.abs(np.diff(grey, axis=1)).mean() + np.abs(np.diff(grey, axis=0)).mean()
        h, w = grey.shape
        border = np.concatenate([frame[: h // 8].reshape(-1, 3), frame[-h // 8 :].reshape(-1, 3)])
        centre = frame[h // 4 : 3 * h // 4, w // 4 : 3 * w // 4].reshape(-1, 3)
        return edges, np.abs(centre.mean(0) - border.mean(0)).sum()

    secs = time_it(proxy, repeats=50)
    print("\nviewfinder proxy - 640x480 frame, no model")
    print(f"  {secs * 1000:>8.2f} ms  {1 / secs:>6.0f} fps headroom")
    return secs


def bench_capture(model, photo):
    """One whole capture: decode, shrink, segment, composite, shrink again.

    This is what the user waits for after pressing the shutter, and it is
    usually dominated by decoding the camera's full resolution still rather
    than by the net.
    """
    try:
        from PIL import Image, ImageOps
    except ImportError:
        print("\ncapture - skipped, pillow not installed")
        return

    mean = np.array([0.485, 0.456, 0.406])
    std = np.array([0.229, 0.224, 0.225])
    sess = session(model, os.cpu_count() or 1)
    name = sess.get_inputs()[0].name

    def capture():
        img = ImageOps.exif_transpose(Image.open(photo)).convert("RGB")
        img.thumbnail((1536, 1536))
        small = np.asarray(img.resize((SIZE, SIZE), Image.Resampling.LANCZOS), dtype=np.float64)
        small /= max(small.max(), 1e-6)  # u2net normalises by the frame's own peak
        x = ((small - mean) / std).transpose(2, 0, 1)[None].astype(np.float32)
        pred = sess.run(None, {name: x})[0][:, 0, :, :]
        pred = (pred - pred.min()) / (pred.max() - pred.min())
        mask = Image.fromarray((np.squeeze(pred) * 255).astype("uint8"), "L")
        cut = Image.composite(img, Image.new("RGBA", img.size, 0), mask.resize(img.size, Image.Resampling.LANCZOS))
        cut.thumbnail((768, 768))
        return cut

    with Image.open(photo) as probe:
        source = probe.size
    secs = time_it(capture, repeats=5)
    print(f"\ncapture - one shutter press, {source[0]}x{source[1]} source")
    print(f"  {secs * 1000:>8.0f} ms")
    return secs


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--model", help="path to u2netp.onnx (downloaded if absent)")
    ap.add_argument("--photo", help="a real photo to time one full capture against")
    args = ap.parse_args()

    print(f"{platform.platform()}")
    print(f"{platform.processor() or platform.machine()} - {os.cpu_count()} cores")
    print(f"python {platform.python_version()}  onnxruntime {ort.__version__}")

    model = find_model(args.model)
    print(f"model {model} ({model.stat().st_size / 1e6:.1f} MB)")

    inference = bench_inference(model)
    bench_proxy()
    if args.photo:
        bench_capture(model, args.photo)

    print(f"\npeak rss {peak_rss_mb():.0f} MB")

    cores = os.cpu_count() or 1
    best = inference.get(min(4, cores)) or min(inference.values())
    print(
        f"\nper-frame segmentation would run at {1 / best:.1f} fps on {min(4, cores)} "
        "cores, and would hold them all.\nthe proxy leaves the cpu free, so a live "
        "indicator wants the proxy and the shutter wants the net."
    )


if __name__ == "__main__":
    main()
