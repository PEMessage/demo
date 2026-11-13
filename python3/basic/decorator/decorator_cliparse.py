#!/usr/bin/env python3
import sys


class UsageError(Exception):
    pass


class Param:
    def __init__(self, name, default=None, help=""):
        self.name, self.default, self.help = name, default, help

    @property
    def flag(self):
        return "--" + self.name.replace("_", "-")

    def consume(self, argv):
        """consume `arg` in argv or return `default`"""
        if self.flag not in argv:
            return self.default
        i = argv.index(self.flag)
        del argv[i]                      # drop the flag itself
        if i >= len(argv):
            raise UsageError(f"{self.flag} requires a value")
        return argv.pop(i)               # drop and return the value


def option(name, **kwargs):
    def decorator(func):
        # decorators run bottom-up, so this list is in reverse source order
        func.__params__ = [*getattr(func, "__params__", []), Param(name, **kwargs)]
        return func
    return decorator


class Command:
    def __init__(self, func):
        self.name = func.__name__
        self.callback = func
        self.params = list(reversed(getattr(func, "__params__", [])))   # flip it back

    def help_text(self):
        doc = (self.callback.__doc__ or "").strip()
        lines = [f"Usage: {self.name} [options]"]
        if doc:
            lines += ["", doc]
        lines += ["", "Options:"]
        for p in self.params:
            default = "" if p.default is None else f" (default: {p.default})"
            lines.append(f"  {p.flag:<16} {p.help}{default}")
        lines.append(f"  {'-h, --help':<16} show this help")
        return "\n".join(lines)

    def __call__(self, argv=None):
        argv = list(sys.argv[1:] if argv is None else argv)
        if "-h" in argv or "--help" in argv:
            print(self.help_text())
            return 0
        try:
            values = {p.name: p.consume(argv) for p in self.params}
        except UsageError as e:
            print(f"Error: {e}", file=sys.stderr)
            return 2
        if argv:
            print(f"Error: unexpected arguments: {' '.join(argv)}", file=sys.stderr)
            return 2
        self.callback(**values)
        return 0


# ===============================
# demo commands
# ===============================

@Command
@option("name", default="world", help="who to greet")
@option("times", default="1", help="how many times")
def greet(name, times):
    """Say hello."""
    for _ in range(int(times)):
        print(f"Hello, {name}!")


@Command
@option("a", default="0", help="first number")
@option("b", default="0", help="second number")
def add(a, b):
    """Add two numbers."""
    print(int(a) + int(b))


COMMANDS = {cmd.name: cmd for cmd in (greet, add)}


def main(argv=None) -> int:
    argv = list(sys.argv[1:] if argv is None else argv)
    if not argv or argv[0] in ("-h", "--help"):
        print("Usage: decorator_cliparse.py <command> [options]\n\nCommands:")
        for name, cmd in COMMANDS.items():
            print(f"  {name:<8} {(cmd.callback.__doc__ or '').strip()}")
        return 0

    name, rest = argv[0], argv[1:]
    if name not in COMMANDS:
        print(f"Error: unknown command: {name}", file=sys.stderr)
        return 2
    return COMMANDS[name](rest)


if __name__ == "__main__":
    sys.exit(main())
