import sys


def read(path : str):
    if (path == '-'):
        return sys.stdin.read()
    else:
        with open(path, 'rb') as file:
            content = file.read()
        return content


if __name__ == "__main__":
    if len(sys.argv) > 1:
        print(read(sys.argv[1]))
    else:
        print(read('-'))

