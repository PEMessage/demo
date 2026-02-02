# /// script
# requires-python = ">=3.12"
# dependencies = [
#     "sympy",
# ]
# ///

import sys
from textwrap import dedent

import sympy as sp


F =  sp.Function('F')

def encrypto(x, y, N=5):

    Left = {1: x }
    Right = {1: y }


    for n in range(2, N + 1):

        # 使用递推关系更新 Left 和 Right
        # See: https://ctf-wiki.org/crypto/blockcipher/des/
        # Round1, Diagram中每一层上方，左右的值

        Left[n] = Right[n-1]
        Right[n] = Left[n-1] + F(Right[n-1])

    return Left[N], Right[N]

def decrypto(x, y, N=5):

    Left = {1: x}
    Right = {1: y}


    for n in range(2, N + 1):
        Right[n] = Left[n-1]
        Left[n] = Right[n-1] - F(Right[n])

    return Left[N], Right[N]


def verify_correctness(N=1):
    x, y = sp.symbols('x y')

    # 原始明文
    print(f"plaintext: L1 = {x}, R1 = {y}")

    # 加密过程
    for n in range(1, N+1):
        L_enc, R_enc = encrypto(x, y, n)

        left = f'L{n} = {L_enc}'
        right = f'R{n} = {R_enc}'
        print(f'{left:<40} {right}')

    # 解密过程（将加密结果作为输入）
    L_dec, R_dec = decrypto(L_enc, R_enc, N)
    print(f"\ndecrypto {N} times:")
    print(f"L{N} = {L_dec}")
    print(f"R{N} = {R_dec}")

    # 验证解密结果是否等于原始明文
    print(f"\ncheck :")
    print(f"DL{N} == L1: {sp.simplify(L_dec - x) == 0}")
    print(f"DR{N} == R1: {sp.simplify(R_dec - y) == 0}")

    if sp.simplify(L_dec - x) == 0 and sp.simplify(R_dec - y) == 0:
        print(f"Yes, OK")
    else:
        print(f"Fail, Bed")

    return L_enc, R_enc, L_dec, R_dec

if __name__ == "__main__":
    print("----------------" * 3)
    print(dedent(f'''
    HELP:
        {sys.argv[0]} [N]
    EXAMPLE:
        {sys.argv[0]} 5
        '''))
    print("----------------" * 3)
    verify_correctness( int(sys.argv[1]) if sys.argv[1] else 5)

