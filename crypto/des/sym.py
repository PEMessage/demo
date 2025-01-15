import sympy as sp

# 定义符号变量
x, y = sp.symbols('x y')

# 定义 F 函数，这里假设 F 是一个简单的示例函数
F =  sp.Function('F')
dF =  sp.Function('dF')
# def F(a, *args):
#     return a

# 初始化 Left 和 Right 的字典
Left = {1: x }
Right = {1: y }


# 计算前几个 N 的 Left 和 Right 值
N = 5  # 你可以选择其他的 N，来计算更多的项
for n in range(2, N + 1):

    # 使用递推关系更新 Left 和 Right
    # See: https://ctf-wiki.org/crypto/blockcipher/des/
    # Round1, Diagram中每一层上方，左右的值

    Left[n] = Right[n-1]
    Right[n] = Left[n-1] + F(Right[n-1])

left_expr = 'Left[n] = Right[n-1]'
right_expr = 'Right[n] = Left[n-1] + F(Right[n-1])'
print(f'{left_expr:<100} {right_expr}')
# 输出 Left 和 Right 的结果
for n in range(1, N + 1):
    left = f'Left({n}) = {Left[n]}'
    right = f'Right({n}) = {Right[n]}'
    print(f'{left:<100} {right}')
