import matplotlib.pyplot as plt
import numpy as np

def cubic_bezier(t, p0, p1, p2, p3):
    """Calculate a point on a cubic Bézier curve"""
    t = max(0.0, min(1.0, t))
    mt = 1 - t
    return (mt**3 * p0 + 3 * mt**2 * t * p1 + 
            3 * mt * t**2 * p2 + t**3 * p3)

def cubic_bezier_vec(t, points):
    """Vector version for 2D points"""
    p0, p1, p2, p3 = points
    t = max(0.0, min(1.0, t))
    mt = 1 - t
    x = (mt**3 * p0[0] + 3 * mt**2 * t * p1[0] + 
         3 * mt * t**2 * p2[0] + t**3 * p3[0])
    y = (mt**3 * p0[1] + 3 * mt**2 * t * p1[1] + 
         3 * mt * t**2 * p2[1] + t**3 * p3[1])
    return (x, y)

# Define control points (P0, P1, P2, P3)
points = [
    (0, 0),    # P0 - Start point
    (1, 3),    # P1 - First control point
    (4, -2),   # P2 - Second control point
    (5, 1)     # P3 - End point
]

# Generate points along the curve
t_values = np.linspace(0, 1, 100)
curve_points = [cubic_bezier_vec(t, points) for t in t_values]

# Unzip the points for plotting
curve_x, curve_y = zip(*curve_points)
control_x, control_y = zip(*points)

# Create the plot
plt.figure(figsize=(10, 6))

# Plot the curve
plt.plot(curve_x, curve_y, 'b-', linewidth=2, label='Bézier Curve')

# Plot control points
plt.plot(control_x, control_y, 'ro--', alpha=0.5, label='Control Points')

# Mark the start and end points
plt.plot(control_x[0], control_y[0], 'go', markersize=8, label='Start (P0)')
plt.plot(control_x[-1], control_y[-1], 'yo', markersize=8, label='End (P3)')

# Draw control lines
plt.plot([control_x[0], control_x[1]], [control_y[0], control_y[1]], 'r--', alpha=0.3)
plt.plot([control_x[2], control_x[3]], [control_y[2], control_y[3]], 'r--', alpha=0.3)

# Add labels and title
plt.title('Cubic Bézier Curve Demonstration', fontsize=14)
plt.xlabel('X-axis')
plt.ylabel('Y-axis')
plt.grid(True, linestyle='--', alpha=0.7)
plt.legend()
plt.axis('equal')

# Show the plot
plt.tight_layout()
plt.show()
