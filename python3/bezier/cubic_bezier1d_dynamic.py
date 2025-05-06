import matplotlib.pyplot as plt
import numpy as np
from matplotlib.widgets import Slider

def cubic_bezier(t, p0, p1, p2, p3):
    """Calculate cubic Bézier curve (vectorized implementation)"""
    t = np.clip(t, 0, 1)
    mt = 1 - t
    return mt**3 * p0 + 3 * mt**2 * t * p1 + 3 * mt * t**2 * p2 + t**3 * p3

# Initial control points
init_points = [0.0, 0.1, 0.3, 1.0]  # [P0, P1, P2, P3]

# Setup figure and axis
fig, ax = plt.subplots(figsize=(10, 7))
plt.subplots_adjust(bottom=0.4)

# Time values (vectorized)
t = np.linspace(0, 1, 100)

# Initial curve calculation (vectorized)
values = cubic_bezier(t, *init_points)

# Plot elements
line, = ax.plot(t, values, 'b-', lw=2, label='Cubic Bézier')
control_points = ax.plot([1/3, 2/3], init_points[1:3], 'ro', ms=8)[0]
texts = [
    ax.text(1/3, init_points[1] + 0.05, 'P1', ha='center'),
    ax.text(2/3, init_points[2] + 0.05, 'P2', ha='center')
]

# Style plot
ax.set(title='Interactive Cubic Bézier Curve', xlabel='Time (t)', ylabel='Value')
ax.grid(True, ls='--', alpha=0.5)
ax.legend()
ax.set_ylim(-0.1, 1.1)

# Create sliders
sliders = []
for i, (label, y_pos) in enumerate(zip(['P0', 'P1', 'P2', 'P3'], [0.3, 0.25, 0.2, 0.15])):
    ax_slider = plt.axes([0.25, y_pos, 0.65, 0.03])
    sliders.append(Slider(ax_slider, label, 0, 1.0, valinit=init_points[i]))

def update(val):
    # Get current slider values
    points = [s.val for s in sliders]
    
    # Update curve (vectorized)
    line.set_ydata(cubic_bezier(t, *points))
    
    # Update control points and text
    control_points.set_ydata(points[1:3])
    for i, text in enumerate(texts):
        text.set_position((texts[i].get_position()[0], points[i+1] + 0.05))
    
    fig.canvas.draw_idle()

# Register update function
for slider in sliders:
    slider.on_changed(update)

plt.show()
