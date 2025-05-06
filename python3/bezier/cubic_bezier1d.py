import matplotlib.pyplot as plt
import numpy as np

def cubic_bezier(t, p0, p1, p2, p3):
    """Cubic Bézier curve calculation (1D)"""
    t = np.clip(t, 0, 1)  # Ensure t is between 0 and 1
    mt = 1 - t
    # B(t) = (1-t)³ * P0 + 3*(1-t)²*t * P1 + 3*(1-t)*t² * P2 + t³ * P3,  t ∈ [0,1]
    # Also see: https://zh.wikipedia.org/wiki/贝塞尔曲线
    return mt**3 * p0 + 3 * mt**2 * t * p1 + 3 * mt * t**2 * p2 + t**3 * p3

# Control points (for time-value mapping)
p0 = 0.0  # Start value (t=0)
p1 = 0.1  # First control point (affects initial curve)
p2 = 0.3  # Second control point (affects final curve)
p3 = 1.0  # End value (t=1)

# Generate time values
t = np.linspace(0, 1, 100)

# Calculate corresponding Bézier values
values = [cubic_bezier(x, p0, p1, p2, p3) for x in t]

# Create the plot
plt.figure(figsize=(8, 5))
plt.plot(t, values, 'b-', linewidth=2, label='Cubic Bézier')

# Mark control points (projected onto the curve)
for cp, (tx, ty) in zip(['P1', 'P2'], [(1/3, p1), (2/3, p2)]):
    plt.plot(tx, ty, 'ro', markersize=8)
    plt.text(tx, ty + 0.05, cp, ha='center')

# Style the plot
plt.title('Cubic Bézier Curve: X-t Graph', fontsize=14)
plt.xlabel('Time (t)', fontsize=12)
plt.ylabel('Value', fontsize=12)
plt.grid(True, linestyle='--', alpha=0.5)
plt.legend()
plt.tight_layout()
plt.show()
