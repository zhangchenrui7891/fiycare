"""CPU mesh preview for asset orientation/scale review; not a GPU game screenshot."""
from pathlib import Path
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d.art3d import Poly3DCollection
import numpy as np

root = Path(__file__).resolve().parents[1]
fig = plt.figure(figsize=(15, 8), facecolor="#122331")
for column, name in enumerate(["a320", "b737", "b787"]):
    data = (root / "assets" / (name + ".fcmesh")).read_bytes()
    count = int.from_bytes(data[8:12], "little")
    vertices = np.frombuffer(data, dtype="<f4", offset=20, count=count * 12).reshape(-1, 3, 12)
    for row in range(2):
        ax = fig.add_subplot(2, 3, column + 1 + row * 3, projection="3d", facecolor="#122331")
        points = vertices[:, :, :3][:, :, [0, 2, 1]]
        colors = vertices[:, :, 3:6].mean(1)
        normals = vertices[:, :, 9:12].mean(1)
        shade = .6 + .4 * np.clip(normals @ np.array([.3, .85, .44]), 0, 1)
        mesh = Poly3DCollection(points, facecolors=np.clip(colors * shade[:, None], 0, 1),
                                linewidths=0, edgecolors="none")
        ax.add_collection3d(mesh)
        low = points.reshape(-1, 3).min(0)
        high = points.reshape(-1, 3).max(0)
        center = (low + high) / 2
        radius = max(high - low) / 2
        ax.set_xlim(center[0] - radius, center[0] + radius)
        ax.set_ylim(center[1] - radius, center[1] + radius)
        ax.set_zlim(center[2] - radius * .45, center[2] + radius * .45)
        ax.set_box_aspect([1, 1, .45])
        ax.set_axis_off()
        ax.view_init(elev=24 if row == 0 else 90, azim=-65 if row == 0 else -90)
        if row == 0:
            ax.set_title(name.upper() + " · amvlab CC BY 4.0", color="white", pad=4)
        else:
            print(name, "native bounds", vertices[:, :, :3].reshape(-1, 3).min(0),
                  vertices[:, :, :3].reshape(-1, 3).max(0))
fig.suptitle("Downloaded aircraft geometry · CPU preview, not a game screenshot", color="white", fontsize=16)
fig.tight_layout()
fig.savefig(root / "assets/new_fleet_preview.png", dpi=150, facecolor=fig.get_facecolor())
