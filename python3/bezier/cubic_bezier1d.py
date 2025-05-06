import matplotlib.pyplot as plt
import numpy as np
import argparse

def cubic_bezier(t, p0, p1, p2, p3):
    """Cubic Bézier curve calculation (1D)"""
    t = np.clip(t, 0, 1)  # Ensure t is between 0 and 1
    mt = 1 - t
    # B(t) = (1-t)³ * P0 + 3*(1-t)²*t * P1 + 3*(1-t)*t² * P2 + t³ * P3,  t ∈ [0,1]
    # Also see: https://zh.wikipedia.org/wiki/贝塞尔曲线
    return mt**3 * p0 + 3 * mt**2 * t * p1 + 3 * mt * t**2 * p2 + t**3 * p3

def parse_arguments():
    """Parse command line arguments for control points"""
    parser = argparse.ArgumentParser(description='Plot a Cubic Bézier Curve')
    parser.add_argument('--P0', type=float, help='Start point value (t=0)',    default=0)
    parser.add_argument('--P1', type=float, help='First control point value',  default=1/3 + 0.2)
    parser.add_argument('--P2', type=float, help='Second control point value', default=2/3 - 0.2)
    parser.add_argument('--P3', type=float, help='End point value (t=1)',      default=1)
    parser.add_argument('--samples', type=int, default=100,
                        help='Number of samples (default: 100)')
    parser.add_argument('--output', type=str,
                        help='Save plot to file instead of showing')
    return parser.parse_args()

def main():
    args = parse_arguments()

    # Generate time values
    t = np.linspace(0, 1, args.samples)

    # Calculate corresponding Bézier values
    values = cubic_bezier(t, args.P0, args.P1, args.P2, args.P3)

    # Create the plot
    plt.figure(figsize=(8, 5))
    plt.plot(t, values, 'b-', linewidth=2, label='Cubic Bézier')

    # Mark control points (projected onto the curve)
    for cp, (tx, ty) in zip(['P1', 'P2'], [(1/3, args.P1), (2/3, args.P2)]):
        plt.plot(tx, ty, 'ro', markersize=8)
        plt.text(tx, ty + 0.05 * (args.P3 - args.P0), cp, ha='center')

    # Style the plot
    plt.title(f'Cubic Bézier Curve: P0={args.P0}, P1={args.P1}, P2={args.P2}, P3={args.P3}', fontsize=12)
    plt.xlabel('Time (t)', fontsize=12)
    plt.ylabel('Value', fontsize=12)
    plt.grid(True, linestyle='--', alpha=0.5)
    plt.legend()
    plt.tight_layout()

    if args.output:
        plt.savefig(args.output)
        print(f"Plot saved to {args.output}")
    else:
        plt.show()

if __name__ == "__main__":
    main()
