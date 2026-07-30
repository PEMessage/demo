#!/usr/bin/env python3
"""mod_matrix.py x n [mode]

Show x×n matrix of b = a*x mod n for different a-indexing schemes.

Modes:
  row      a = i*n + j          (row-major, default)
  col      a = i + j*x          (column-major)
  inv      a = (i*inv_x%n) + j*n   (b depends only on i)
  ask      show all three
"""

import math
import sys


def inv_mod(x: int, n: int) -> int:
    """modular inverse of x modulo n (x,n coprime)"""
    return pow(x, -1, n)


def make_bijection(x: int, n: int, index: tuple[int, int], scheme: str) -> int:
    i, j = index
    total = x * n
    if scheme == "row":
        return (i * n + j) % total
    elif scheme == "col":
        return (i + j * x) % total
    elif scheme == "inv":
        inv_x = inv_mod(x, n)
        return ((i * inv_x) % n + j * n) % total
    else:
        raise ValueError(f"unknown scheme: {scheme}")


def generate_matrices(x: int, n: int, scheme: str):
    a_mat = []
    b_mat = []
    for i in range(x):
        a_row = []
        b_row = []
        for j in range(n):
            a = make_bijection(x, n, (i, j), scheme)
            a_row.append(a)
            b_row.append((a * x) % n)
        a_mat.append(a_row)
        b_mat.append(b_row)
    return a_mat, b_mat


def print_matrices(x: int, n: int, a_mat: list[list[int]], b_mat: list[list[int]], scheme: str):
    desc = {"row": "a = i*n + j      (行优先)", "col": "a = i + j*x      (列优先)",
            "inv": f"a = i*{inv_mod(x,n)}%{n} + j*{n}  (b仅随i变)"}
    print(f"\n{'='*50}")
    print(f"  scheme: {desc[scheme]}")
    print(f"  x={x}, n={n}   b = a * x mod n")
    print()

    header = " i\\j".ljust(5) + "|" + "".join(f"{j:4d}" for j in range(n))
    sep = "-" * len(header)

    print("-- a --")
    print(header)
    print(sep)
    for i in range(x):
        line = "".join(f"{v:4d}" for v in a_mat[i])
        print(f"{i:4d} |{line}")
    print(sep)

    print("-- b --")
    print(header)
    print(sep)
    for i in range(x):
        line = "".join(f"{v:4d}" for v in b_mat[i])
        print(f"{i:4d} |{line}")
    print(sep)


def main():
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} x n [row|col|inv|all]", file=sys.stderr)
        sys.exit(1)

    x, n = int(sys.argv[1]), int(sys.argv[2])
    mode = sys.argv[3] if len(sys.argv) > 3 else "row"

    if math.gcd(x, n) != 1:
        print(f"Error: {x} and {n} must be coprime", file=sys.stderr)
        sys.exit(1)

    schemes = ["row", "col", "inv"] if mode == "all" else [mode]
    for s in schemes:
        a_mat, b_mat = generate_matrices(x, n, s)
        print_matrices(x, n, a_mat, b_mat, s)


if __name__ == "__main__":
    main()
