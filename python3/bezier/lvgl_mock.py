import matplotlib.pyplot as plt

LV_BEZIER_VAL_SHIFT = 10
LV_BEZIER_VAL_MAX = 1 << LV_BEZIER_VAL_SHIFT


def cubic_bezier(x, x1, y1, x2, y2, tolerance=1e-6, max_iterations=100):
    """
    Calculate the y value on a cubic Bezier curve at a given x coordinate.

    The curve is defined with:
    - p0 = (0, 0)
    - p1 = (x1, y1)
    - p2 = (x2, y2)
    - p3 = (1, 1)

    Parameters:
    - x: The x-coordinate (0 <= x <= 1)
    - x1, y1: First control point coordinates
    - x2, y2: Second control point coordinates
    - tolerance: Precision for Newton-Raphson method
    - max_iterations: Maximum iterations for Newton-Raphson

    Returns:
    - The y-coordinate corresponding to x on the Bezier curve
    """

    # Clamp x to [0, 1] range
    x = max(0, min(1, x))

    # Handle edge cases
    if x == 0:
        return 0
    if x == 1:
        return 1

    # The parametric equation for x(t):
    # x(t) = 3*(1-t)²*t*x1 + 3*(1-t)*t²*x2 + t³

    # We need to find t such that x(t) = x
    # We'll use Newton-Raphson method to solve for t

    # Initial guess for t
    t = x

    for _ in range(max_iterations):
        # Calculate x(t) and its derivative
        t2 = t * t
        t3 = t2 * t
        mt = 1 - t
        mt2 = mt * mt
        mt3 = mt2 * mt

        # x(t) = 3*mt2*t*x1 + 3*mt*t2*x2 + t3
        xt = 3 * mt2 * t * x1 + 3 * mt * t2 * x2 + t3

        # dx/dt = 3*mt2*x1 + 6*mt*t*(x2-x1) + 3*t2*(1-x2)
        dxt = 3 * mt2 * x1 + 6 * mt * t * (x2 - x1) + 3 * t2 * (1 - x2)

        # Check if we're close enough
        if abs(xt - x) < tolerance:
            break

        # Newton-Raphson update
        t = t - (xt - x) / dxt

        # Keep t in [0, 1] range
        t = max(0, min(1, t))

    # Now calculate y(t) with the found t value
    y = 3 * mt2 * t * y1 + 3 * mt * t2 * y2 + t3

    return y

def lv_map(x, in_min, in_max, out_min, out_max):
    return (x - in_min) * (out_max - out_min) // (in_max - in_min) + out_min

def lv_cubic_bezier(x, x0, y0, x1, y1):
    # Cubic bezier interpolation, using float to simplify
    x = x / LV_BEZIER_VAL_MAX
    x0 = x0 / LV_BEZIER_VAL_MAX
    y0 = y0 / LV_BEZIER_VAL_MAX
    x1 = x1 / LV_BEZIER_VAL_MAX
    y1 = y1 / LV_BEZIER_VAL_MAX
    return int(cubic_bezier(x, x0, y0, x1, y1) * LV_BEZIER_VAL_MAX)

def lv_bezier3(t, p0, p1, p2, p3):
    return lv_cubic_bezier(t, 341, p1, 683, p2)

def lv_anim_path_bounce(a):
    # Calculate the current step
    t = lv_map(a['act_time'], 0, a['duration'], 0, LV_BEZIER_VAL_MAX)
    diff = (a['end_value'] - a['start_value'])

    # 3 bounces has 5 parts: 3 down and 2 up. One part is t / 5 long
    if t < 408:
        # Go down
        t = (t * 2500) >> LV_BEZIER_VAL_SHIFT  # [0..1024] range
        t = LV_BEZIER_VAL_MAX - t
    elif t >= 408 and t < 614:
        # First bounce back
        t -= 408
        t = t * 5  # to [0..1024] range
        diff = diff // 20
    elif t >= 614 and t < 819:
        # Fall back
        t -= 614
        t = t * 5  # to [0..1024] range
        t = LV_BEZIER_VAL_MAX - t
        diff = diff // 20
    elif t >= 819 and t < 921:
        # Second bounce back
        t -= 819
        t = t * 10  # to [0..1024] range
        diff = diff // 40
    elif t >= 921 and t <= LV_BEZIER_VAL_MAX:
        # Fall back
        t -= 921
        t = t * 10  # to [0..1024] range
        t = LV_BEZIER_VAL_MAX - t
        diff = diff // 40

    if t > LV_BEZIER_VAL_MAX: t = LV_BEZIER_VAL_MAX
    if t < 0: t = 0
    
    step = lv_bezier3(t, 0, 500, 800, LV_BEZIER_VAL_MAX)

    new_value = step * diff
    new_value = new_value >> LV_BEZIER_VAL_SHIFT
    new_value = a['end_value'] - new_value

    return new_value

# Demo the bounce animation
def demo_bounce_animation():
    # Animation parameters
    anim = {
        'start_value': 0,
        'end_value': 1024,
        'duration': 1024,  # ms
    }
    
    # Simulate animation over time
    time_points = range(0, anim['duration'])
    values = []
    
    for t in time_points:
        anim['act_time'] = t
        value = lv_anim_path_bounce(anim)
        values.append(value)
    
    # Plot the animation path
    plt.figure(figsize=(10, 6))
    plt.plot(time_points, values)
    plt.title("Bounce Animation Path")
    plt.xlabel("Time (ms)")
    plt.ylabel("Value")
    plt.grid(True)
    plt.show()

if __name__ == "__main__":
    demo_bounce_animation()
