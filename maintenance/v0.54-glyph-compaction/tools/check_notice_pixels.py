"""Compare real WARP fixture BMPs with v0.102. No image/performance grading.

Only tests the neutral-view notification relocation at 640x360. Pixel counts
have no FPS or target-hardware meaning. Uses Python stdlib, no image libraries.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import zipfile


def bitmap(data):
    if data[:2] != b'BM':
        raise ValueError('BMP signature required')
    offset = struct.unpack_from('<I', data, 10)[0]
    width, signed_height = struct.unpack_from('<ii', data, 18)
    bits = struct.unpack_from('<H', data, 28)[0]
    compression = struct.unpack_from('<I', data, 30)[0]
    height = abs(signed_height)
    if (width, height, bits, compression) != (640, 360, 32, 0):
        raise ValueError('Expected 640x360 uncompressed 32-bit fixture BMP')
    if len(data) != offset + width * height * 4:
        raise ValueError('BMP payload size mismatch')
    rows = [struct.unpack_from('<640I', data, offset + y * width * 4)
            for y in range(height)]
    return rows if signed_height < 0 else rows[::-1]


def difference(before, after):
    a, b = bitmap(before), bitmap(after)
    old_band = new_band = outside = 0
    for y, (row_a, row_b) in enumerate(zip(a, b)):
        changed = sum(x != z for x, z in zip(row_a, row_b))
        if 282 <= y <= 302:
            old_band += changed
        elif 29 <= y <= 49:
            new_band += changed
        else:
            outside += changed
    return {'old_notice_band_changed_pixels': old_band,
            'upper_notice_band_changed_pixels': new_band,
            'outside_notice_bands_changed_pixels': outside,
            'total_changed_pixels': old_band + new_band + outside}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--current', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    baseline = zipfile.ZipFile(args.baseline)
    comparisons = {}
    for camera in ('chase', 'cockpit'):
        name = f'CI_FIXTURE_WARP_DX11_{camera}.bmp'
        before, after = baseline.read(name), (args.current / name).read_bytes()
        comparisons[camera] = {**difference(before, after),
                               'baseline_bmp_sha256': hashlib.sha256(before).hexdigest(),
                               'current_bmp_sha256': hashlib.sha256(after).hexdigest()}
    cockpit, chase = comparisons['cockpit'], comparisons['chase']
    # Regions cover the old and new HUD bands with one-pixel margins. Allow
    # <=1% incidental rendering differences outside them, never a scene change.
    if cockpit['old_notice_band_changed_pixels'] < 1000 or cockpit['upper_notice_band_changed_pixels'] < 1000:
        raise ValueError('Cockpit notice was not visibly relocated from the panel band to the upper band')
    if cockpit['outside_notice_bands_changed_pixels'] > 640 * 360 // 100:
        raise ValueError('Unexpected changes outside cockpit notice bands')
    if chase['total_changed_pixels'] > 640 * 360 // 100:
        raise ValueError('Chase image changed beyond the neutral-scene tolerance')
    report = {'baseline': 'v0.102 Windows DX11 WARP synthetic fixture',
              'current': 'Windows DX11 WARP synthetic fixture',
              'resolution': [640, 360], 'notice_relocation_passed': True,
              'performance_verdict': 'NOT_EVALUATED',
              'target_hardware_validated': False, 'comparisons': comparisons}
    args.output.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS actual fixture pixels: cockpit notice moved out of lower panel band; chase retained; no hardware/performance verdict.')


if __name__ == '__main__':
    main()
