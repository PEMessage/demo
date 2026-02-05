import itertools

print(
    [(a, b, c) for a in (1, 2, 3)
        for b in (1, 2, 3)
        for c in (1, 2, 3)
        if a != b and a != c and b != c]
)


print(
    [list(perm) for perm in itertools.permutations((1, 2, 3))]
)

def to_c_expr(data, indent=0):
    """
    Convert Python data to C-style expression string
    """
    indent_str = " " * indent

    if isinstance(data, list):
        if not data:
            return "{}"

        # Check if it's a 2D array
        if all(isinstance(item, list) for item in data):
            items = []
            for i, row in enumerate(data):
                items.append(f"{indent_str}    {{{', '.join(str(x) for x in row)}}}{',' if i < len(data)-1 else ''}")
            return f"{{\n" + "\n".join(items) + f"\n{indent_str}}}"
        else:
            # 1D array
            return f"{{{', '.join(str(x) for x in data)}}}"

    elif isinstance(data, tuple):
        return f"{{{', '.join(str(x) for x in data)}}}"

    elif isinstance(data, dict):
        items = []
        for key, value in data.items():
            items.append(f"{indent_str}    .{key} = {to_c_expr(value, indent+4)},")
        return f"{{\n" + "\n".join(items) + f"\n{indent_str}}}"

    elif isinstance(data, bool):
        return "true" if data else "false"

    elif isinstance(data, str):
        return f'"{data}"'

    elif data is None:
        return "NULL"

    else:
        return str(data)

print(
    to_c_expr(
        [list(perm) for perm in itertools.permutations((1, 2, 3))]
    )
)

