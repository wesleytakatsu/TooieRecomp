#!/usr/bin/env python3
"""Offline numeric probe of RT64 matrixDecomposeViewProj vs Tooie/Banjo VP forms.

Reimplements:
  RT64::isMatrixViewProj
  RT64::matrixDecomposeViewProj
from BanjoRecomp/lib/rt64/src/common/rt64_math.cpp

and Tooie func_800E443C product:
  VP = T_S * V_rot * P
where T_S is inv-scale + -translation*inv_scale in the last row.

Diagnostic only. Does not modify the game. See
docs/plan/evidence/p08-render-submission-matrix-diagnosis.md.
"""
from __future__ import annotations

import math
from typing import Tuple

import numpy as np


def is_matrix_view_proj(m: np.ndarray) -> bool:
    return abs(m[3, 3]) >= 1e-6 and abs(1.0 - m[3, 3]) >= 1e-6


def matrix_is_nan(m: np.ndarray) -> bool:
    return bool(np.isnan(m).any())


def matrix_decompose_view_proj(vp: np.ndarray) -> Tuple[np.ndarray, np.ndarray]:
    v = np.eye(4, dtype=np.float64)
    p = np.eye(4, dtype=np.float64)

    p[2, 3] = -1.0
    p[3, 3] = 0.0
    v[0, 2] = -vp[0, 3]
    v[1, 2] = -vp[1, 3]
    v[2, 2] = -vp[2, 3]
    v[3, 2] = -vp[3, 3]

    p[2, 2] = vp[0, 2] / v[0, 2]
    p[3, 2] = vp[3, 2] - p[2, 2] * v[3, 2]

    p[0, 0] = math.sqrt(vp[0, 0] ** 2 + vp[1, 0] ** 2 + vp[2, 0] ** 2)
    p[1, 1] = math.sqrt(vp[0, 1] ** 2 + vp[1, 1] ** 2 + vp[2, 1] ** 2)

    v[0, 0] = vp[0, 0] / p[0, 0]
    v[1, 0] = vp[1, 0] / p[0, 0]
    v[2, 0] = vp[2, 0] / p[0, 0]
    v[3, 0] = vp[3, 0] / p[0, 0]

    v[0, 1] = vp[0, 1] / p[1, 1]
    v[1, 1] = vp[1, 1] / p[1, 1]
    v[2, 1] = vp[2, 1] / p[1, 1]
    v[3, 1] = vp[3, 1] / p[1, 1]

    if matrix_is_nan(v) or matrix_is_nan(p):
        return np.eye(4, dtype=np.float64), vp.copy()
    return v, p


def gu_perspective(fovy_deg: float, aspect: float, near: float, far: float, scale: float = 0.5) -> np.ndarray:
    """libultra/Tooie-style row-major perspective matching func_800CB124 layout."""
    yscale = 1.0 / math.tan(math.radians(fovy_deg) * 0.5)
    p = np.zeros((4, 4), dtype=np.float64)
    p[0, 0] = yscale / aspect
    p[1, 1] = yscale
    p[2, 2] = (near + far) / (near - far)
    p[2, 3] = -1.0
    p[3, 2] = (2.0 * near * far) / (near - far)
    p[3, 3] = 0.0
    return p * scale


def affine_from_rot_trans_scale(r3: np.ndarray, t: np.ndarray, s: float) -> np.ndarray:
    """Tooie T_S * V_rot: last-row translation, uniform scale on the 3x3."""
    a = np.eye(4, dtype=np.float64)
    a[:3, :3] = r3 * s
    a[3, :3] = -t * s
    return a


def report(name: str, vp: np.ndarray, intended_view: np.ndarray | None = None) -> None:
    is_vp = is_matrix_view_proj(vp)
    if is_vp:
        v, p = matrix_decompose_view_proj(vp)
        path = "DECOMPOSE"
    else:
        v = np.eye(4, dtype=np.float64)
        p = vp.copy()
        path = "FALLBACK_VIEW=I"

    reconstructed = v @ p
    err = np.max(np.abs(reconstructed - vp))
    print(f"\n=== {name} ===")
    print(f"isMatrixViewProj: {is_vp}  path={path}")
    print(f"VP[3][3]={vp[3,3]:.6f}")
    print(f"max |V*P - VP| = {err:.6e}")
    print("extracted view 3x3:")
    print(np.array2string(v[:3, :3], precision=4, suppress_small=True))
    print("extracted view last row (translation):", np.array2string(v[3], precision=4, suppress_small=True))
    print("extracted proj diag:", [p[0, 0], p[1, 1], p[2, 2], p[2, 3], p[3, 2], p[3, 3]])
    if intended_view is not None:
        rot_err = np.max(np.abs(v[:3, :3] - intended_view[:3, :3]))
        tr_err = np.max(np.abs(v[3, :3] - intended_view[3, :3]))
        print(f"view 3x3 vs intended maxabs={rot_err:.6e}")
        print(f"view trans vs intended maxabs={tr_err:.6e}")
        print("intended view 3x3:")
        print(np.array2string(intended_view[:3, :3], precision=4, suppress_small=True))


def main() -> None:
    # Wooded Hollow numbers from p08-actor-camera-transform-discriminator.
    r = np.array(
        [
            [0.5843, -0.3564, 0.7291],
            [0.0000, 0.8984, 0.4392],
            [-0.8115, -0.2566, 0.5249],
        ],
        dtype=np.float64,
    )
    eye = np.array([2091.67, 313.14, 560.81], dtype=np.float64)
    p = gu_perspective(50.0, 4.0 / 3.0, 10.0, 8000.0, 0.5)

    vp_banjo = np.eye(4)
    vp_banjo[:3, :3] = r
    vp_banjo = vp_banjo @ p
    report("BANJO incremental V_rot * P (translation NOT in projection)", vp_banjo)

    view_tooie = affine_from_rot_trans_scale(r, eye, 1.0)
    vp_tooie = view_tooie @ p
    report("TOOIE combined T(-eye)*V_rot * P, scale=1", vp_tooie, view_tooie)

    for s in (0.1, 0.5, 2.0):
        view_s = affine_from_rot_trans_scale(r, eye, s)
        vp_s = view_s @ p
        report(f"TOOIE combined T_S * V_rot * P, scale={s}", vp_s, view_s)

    vp_pv = p @ view_tooie
    report("COUNTERFACTUAL P * T(-eye)*V_rot (NOT Tooie mlMtx order)", vp_pv, view_tooie)


if __name__ == "__main__":
    main()
