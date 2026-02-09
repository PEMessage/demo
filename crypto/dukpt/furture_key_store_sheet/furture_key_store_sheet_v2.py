
class Colors:
    RED = '\033[91m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    MAGENTA = '\033[95m'
    CYAN = '\033[96m'
    RESET = '\033[0m'


class Special:
    NULL = -1
    DELETE = -2


def func(y, x):
    if x < 0 or x > 16:
        return Special.NULL

    # (y & -y): the least significant bit
    #           Eg: 0b110100 -> 0b00100
    if (1 << x) == (y & -y):
        return Special.DELETE

    if y & (1 << x) != 0:
        return Special.NULL

    return (1 << x) * ((y >> x) + 1)


def generate_table(max_x=21, max_y=16):
    table = [[Special.NULL for _ in range(max_y + 1)] for _ in range(max_x + 1)]

    for y in range(max_y + 1):
        for x in range(max_x + 1):
            table[x][y] = func(y, x)

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


def print_table_color(table, max_x, max_y):

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
            current_val = table[x][y]

            # 检查与上一行的差异
            is_different_from_previous = False
            if y > 0:
                previous_val = table[x][y - 1]
                is_different_from_previous = (current_val != previous_val)

            # 处理特殊值
            if current_val == -1:
                if is_different_from_previous:
                    print(f"{Colors.YELLOW}     -{Colors.RESET}", end="")
                else:
                    print("     -", end="")
            elif current_val == -2:
                if is_different_from_previous:
                    print(f"{Colors.RED}    <-{Colors.RESET}", end="")
                else:
                    print("    <-", end="")
            else:
                # 正常值，检查是否与上一行不同
                if is_different_from_previous:
                    print(f"{Colors.GREEN}{current_val:6d}{Colors.RESET}", end="")
                else:
                    print(f"{current_val:6d}", end="")
        print()


# Use python3 tc_store_sheet.py | less -R
if __name__ == '__main__':
    max_x = 21
    max_y = 0xffff
    print_table_color(generate_table(max_x, max_y), max_x, max_y)
