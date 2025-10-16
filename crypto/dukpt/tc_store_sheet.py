
def count_trailing_zeros(n):
    if n == 0:
        return 0

    count = 0
    while n & 1 == 0:
        count += 1
        n >>= 1
    return count


def next_power_of_2(n):
    if n <= 0:
        return 1
    if n & (n - 1) == 0:
        return n

    return 1 << (n).bit_length()


def prev_power_of_2(n):
    if n <= 0:
        return 0  # or raise ValueError, since there's no power of 2 ≤ 0

    return 1 << (n.bit_length() - 1)


def count_ones(n):
    count = 0  # n == 0, return 0
    while n:
        n = prev(n)
        count += 1
    return count


def prev(n):
    if n == 1:
        return 0
    return n & n - 1


def ibdk_pos(n):
    y = prev(n)
    x = count_trailing_zeros(n)
    return x, y


class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    RESET = '\033[0m'

class Special:
    NO_CHANGE = -1
    DELETE = -2




def generate_table(max_x=8, max_y=8, max_n=2**16):
    table = [[Special.NO_CHANGE for _ in range(max_y + 1)] for _ in range(max_x + 1)]

    for n in range(1, max_n + 1):
        x, y = ibdk_pos(n)
        if x <= max_x and y <= max_y:
            if table[x][y] == Special.NO_CHANGE:  # 如果是第一个找到的n
                table[x][y] = n
        if n < max_y:
            table[x][n] = Special.DELETE
    return table


def refill_under_zero_by_search_up(table):
    len_x = len(table)
    len_y = len(table[0])

    for y in range(len_y):
        for x in range(len_x):
            if table[x][y] == Special.NO_CHANGE and y - 1 >= 0:
                table[x][y] = table[x][y - 1]

    return table



def print_table(table, max_x, max_y):
    print("x-y (x: trailing zeros, y: ones count)")
    print("=" * 80)

    # 打印表头
    print("  y\\x|      |", end="")
    for x in range(max_x + 1):
        print(f"{x:6d}", end="")
    print()
    print("-" * 5 + "-" * (8 + 6 * (max_x + 1)))

    # 打印表格内容
    for y in range(max_y + 1):
        print(f"{y:5d}| {y:04x} |", end="")
        for x in range(max_x + 1):
            if table[x][y] == -1:
                print("     -", end="")
            elif table[x][y] == -2:
                # print(f"{Colors.RED}     -{Colors.RESET}", end="")
                print("    <-", end="")
            else:
                print(f"{table[x][y]:6d}", end="")
        print()


if __name__ == '__main__':
    max_x = 21
    max_y = 0xffff
    max_n = max_y
    # print_table(refill_under_zero_by_search_up(generate_table(max_x, max_y, max_n)), max_x, max_y)
    print_table(generate_table(max_x, max_y, max_n), max_x, max_y)
