import math
import matplotlib.pyplot as plt

FILENAME = "Run2_extra_obstacles.txt"

MODE = "filtered"

MAX_DIST_MM_GLOBAL = 500        # ignore |dist| > 500 mm per 50 ms
MAX_ANGLE_DEG_GLOBAL = 45       # ignore |angle| > 45 deg per 50 ms

ANGLE_NOISE_DEG = 3             # |angle| below this while moving -> 0 (straight)
TURN_ONLY_DIST_MM = 3           # |dist| <= this AND |angle| >= ANGLE_NOISE_DEG -> turn in place


def load_dist_angle(filename):
    data = []
    with open(filename, "r") as f:
        for line in f:
            line = line.strip()
            if not line or "," not in line:
                continue

            try:
                dist_str, angle_str = line.split(",")
                dist = int(dist_str.strip())
                angle = int(angle_str.strip())
                data.append((dist, angle))
            except ValueError:
                # Skip lines that aren't clean "int,int"
                continue
    return data

def integrate_path(dist_angle_list, mode="filtered"):
    x, y, theta = 0.0, 0.0, 0.0  # meters, meters, radians
    xs = [x]
    ys = [y]

    for dist_mm, angle_deg in dist_angle_list:
        if abs(dist_mm) > MAX_DIST_MM_GLOBAL or abs(angle_deg) > MAX_ANGLE_DEG_GLOBAL:
            continue

        d = dist_mm / 1000.0  # mm -> m

        if mode == "raw":
            dtheta = math.radians(angle_deg)
            x += d * math.cos(theta)
            y += d * math.sin(theta)
            theta += dtheta
            xs.append(x)
            ys.append(y)
            continue

        # Case A: almost no movement, noticeable angle ⇒ turn in place
        if abs(dist_mm) <= TURN_ONLY_DIST_MM and abs(angle_deg) >= ANGLE_NOISE_DEG:
            theta += math.radians(angle_deg)
            xs.append(x)
            ys.append(y)
            continue

        # Case B: moving; decide whether to treat as straight or turning
        if abs(angle_deg) < ANGLE_NOISE_DEG:
            dtheta = 0.0  # treat as straight
        else:
            dtheta = math.radians(angle_deg)

        # Move along current heading
        x += d * math.cos(theta)
        y += d * math.sin(theta)
        # Then apply heading change
        theta += dtheta

        xs.append(x)
        ys.append(y)

    return xs, ys



def main():
    data = load_dist_angle(FILENAME)
    if not data:
        print("No usable data found in file:", FILENAME)
        return

    xs, ys = integrate_path(data, mode=MODE)

    plt.figure(figsize=(6, 6))
    plt.plot(xs, ys, marker=".", linewidth=1)
    plt.scatter(xs[0], ys[0], label="Start", s=60)
    plt.scatter(xs[-1], ys[-1], label="End", s=60)

    plt.title(f"Roomba Path ({MODE}) from {FILENAME}")
    plt.xlabel("X (meters)")
    plt.ylabel("Y (meters)")
    plt.axis("equal")
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.show()


if __name__ == "__main__":
    main()
