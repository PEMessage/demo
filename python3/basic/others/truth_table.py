
print(
    [(a, b) for a in range(0, 2) for b in range(0, 2)]
)

print(
    [a ^ b ^ b == a for a in range(0, 2) for b in range(0, 2)]
)

print(
    [a ^ b  for a in range(0, 2) for b in range(0, 2)]
)
