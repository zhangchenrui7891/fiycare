"""CPU projection of exported scenery triangles. Not a DirectX game screenshot.

Run scenery_check mesh.txt, then python tools/preview_scenery.py mesh.txt out.png.
Uses Pillow/numpy for a diagnostic perspective projection and satellite sampling;
it does not reproduce game shaders, weather, AA, camera or GPU performance.
"""
from pathlib import Path
import sys
import numpy as np
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[1]
mesh = np.loadtxt(sys.argv[1], dtype=np.float64).reshape(-1, 3, 12)
texture = np.asarray(Image.open(root / 'assets/pvg_satellite.jpg').convert('RGB')) / 255
width, height = 1280, 720
eye = np.array([-820., 240., -1450.])
forward = np.array([-2300., 15., 800.]) - eye
forward /= np.linalg.norm(forward)
right = np.cross([0., 1., 0.], forward)
right /= np.linalg.norm(right)
up = np.cross(forward, right)
light = np.array([.3, .85, -.44])
light /= np.linalg.norm(light)
sky = np.array([.64, .77, .83])
pixels = np.empty((height, width, 3))
pixels[:] = sky
depth = np.full((height, width), np.inf)
for tri in mesh:
    rel = tri[:, :3] - eye
    z = rel @ forward
    if np.any(z < 10):
        continue
    x = width / 2 + 820 * (rel @ right) / z
    y = height / 2 - 820 * (rel @ up) / z
    xmin, xmax = max(0, int(np.floor(x.min()))), min(width-1, int(np.ceil(x.max())))
    ymin, ymax = max(0, int(np.floor(y.min()))), min(height-1, int(np.ceil(y.max())))
    if xmin > xmax or ymin > ymax:
        continue
    xx, yy = np.meshgrid(np.arange(xmin, xmax+1)+.5, np.arange(ymin, ymax+1)+.5)
    denominator = (y[1]-y[2])*(x[0]-x[2])+(x[2]-x[1])*(y[0]-y[2])
    if abs(denominator) < 1e-8:
        continue
    a = ((y[1]-y[2])*(xx-x[2])+(x[2]-x[1])*(yy-y[2]))/denominator
    b = ((y[2]-y[0])*(xx-x[2])+(x[0]-x[2])*(yy-y[2]))/denominator
    c = 1-a-b
    reciprocal = a/z[0]+b/z[1]+c/z[2]
    distance = 1/np.maximum(reciprocal, 1e-9)
    subdepth = depth[ymin:ymax+1, xmin:xmax+1]
    mask = (a>=0) & (b>=0) & (c>=0) & (distance < subdepth)
    if not mask.any():
        continue
    if tri[0, 11] == 1:
        u = (a*tri[0, 9]/z[0]+b*tri[1, 9]/z[1]+c*tri[2, 9]/z[2])*distance
        v = (a*tri[0, 10]/z[0]+b*tri[1, 10]/z[1]+c*tri[2, 10]/z[2])*distance
        tx = np.clip(((u*2048-1)/1022*1023).astype(int), 0, 1023)
        ty = np.clip(((v*2048-1)/1022*1023).astype(int), 0, 1023)
        rgb = texture[ty, tx] * .95
    else:
        shade = .5 + .5 * max(0, tri[0, 6:9] @ light)
        rgb = np.broadcast_to(tri[0, 3:6]*shade, (*a.shape, 3)).copy()
    fog = np.clip((distance-1800)/6500, 0, .65)[..., None]
    rgb = rgb*(1-fog)+sky*fog
    pixels[ymin:ymax+1, xmin:xmax+1][mask] = rgb[mask]
    subdepth[mask] = distance[mask]
im = Image.fromarray((np.clip(pixels, 0, 1)*255).astype('uint8'))
draw = ImageDraw.Draw(im)
draw.rectangle((0, 0, width, 72), fill='#132c38')
font = ImageFont.truetype('DejaVuSans.ttf', 21)
small = ImageFont.truetype('DejaVuSans.ttf', 16)
draw.text((24, 13), 'FlightCareerAir v0.50 | Illustrative offline scenery', font=font, fill='white')
draw.text((24, 43), 'CPU geometry projection - NOT a game screenshot or GPU/performance test', font=small, fill='#b5d5e3')
draw.rectangle((0, height-34, width, height), fill='#132c38')
draw.text((24, height-27), 'Imagery: EOX / Copernicus 2016-2017, CC BY 4.0 | Buildings, roads and fields are illustrative', font=small, fill='#c0d1db')
im.save(sys.argv[2])
print('Saved diagnostic CPU projection:', sys.argv[2])
