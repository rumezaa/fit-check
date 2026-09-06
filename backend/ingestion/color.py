from typing import NamedTuple

import numpy as np
from skimage import color
from sklearn.cluster import KMeans

ALPHA_MIN = 250  # anything softer is an anti-aliased edge blended with the background
L_MIN, L_MAX = (
    10.0,
    95.0,
)  # deep folds and specular highlights are lighting, not material
MAX_SAMPLES = 50_000  # kmeans over every pixel of a 24MP photo is needlessly slow
SEED = 0


class LCh(NamedTuple):
    l: float
    c: float
    h: float


def lab_to_lch(lab):
    l, a, b = lab
    c = np.sqrt(a * a + b * b)
    h = np.degrees(np.arctan2(b, a)) % 360  # % 360 - always 0-360, never negative
    # plain floats, not np.float64 — these get json-serialized downstream
    return LCh(float(l), float(c), float(h))


def lch_to_hex(lch):
    rad = np.radians(lch.h)
    lab = np.array([[[lch.l, lch.c * np.cos(rad), lch.c * np.sin(rad)]]])
    rgb = np.clip(color.lab2rgb(lab).reshape(3), 0, 1)
    r, g, b = (rgb * 255).round().astype(int)
    return f"#{r:02x}{g:02x}{b:02x}"


def extract_dominant_lch(cutout):
    pixels = np.array(cutout.convert("RGBA"))
    mask = pixels[:, :, 3] >= ALPHA_MIN
    garment = pixels[mask][:, :3]  # rgb of garment pixels

    if len(garment) < 3:
        raise ValueError(
            "fewer than 3 garment pixels — check the cutout's alpha channel"
        )

    lab = color.rgb2lab(garment.reshape(-1, 1, 3) / 255.0).reshape(-1, 3)

    # drop the lighting extremes, but only if enough of the garment survives
    lit = lab[(lab[:, 0] > L_MIN) & (lab[:, 0] < L_MAX)]
    if len(lit) >= 3:
        lab = lit

    rng = np.random.default_rng(SEED)
    if len(lab) > MAX_SAMPLES:
        lab = lab[rng.choice(len(lab), MAX_SAMPLES, replace=False)]

    km = KMeans(n_clusters=3, n_init=10, random_state=SEED).fit(lab)

    # we pick the dominant cluster - this gives us the color of the materal
    labels, counts = np.unique(km.labels_, return_counts=True)

    dom_label = labels[counts.argmax()]  # gets index of the largest cluster
    dom_lab = km.cluster_centers_[dom_label]

    return lab_to_lch(dom_lab)
