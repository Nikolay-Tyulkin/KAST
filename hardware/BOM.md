# BOM: Components and CAD Files

Bill of materials for the KAST knitting assistant. The classification follows
`hardware/3d-print/source/`:

- `Components/` contains purchased or ready-made components with STEP models.
- `Molds/` contains 3D-printed tooling, currently the mold for casting the
  silicone strap.
- `KA-*.step` files in the `source/` root are device parts and the full CAD
  assembly.

## Purchased Components

Files are in `hardware/3d-print/source/Components/`. These items are not printed
as enclosure parts; they are purchased components or ready-made modules used in
the assembly.

| Item | STEP file | Name | Qty | Purpose | Status |
| --- | --- | --- | --- | --- | --- |
| C1 | `Components/ESP32-S3-Lcd-1_69_3D.step` | Waveshare ESP32-S3-LCD-1.69 | 1 | Main board with screen | Confirmed |
| C2 | `Components/button 12x12.step` | 12x12 mm tactile button | 3 | Buttons for `+`, `-`, and universal function | STEP model available |
| C3 | No STEP | LiPo battery 042030P 320 mAh | 1 | Device power | 30 x 20 x 4 mm, manufacturer: Dongguan Yalitong Electronics Technology Co., Ltd |
| C4 | No STEP | AWG 24 wire | As needed | Button and power wiring | Confirmed |
| C5 | Screw | M2x5 | 4 | ESP32 board mounting | Confirmed |

## 3D-Printed Device Parts

Files are in `hardware/3d-print/source/`. Before printing, export them to `.stl`
or `.3mf` and place print-ready files in `hardware/3d-print/enclosure/`.

| Item | STEP file | Name | Qty | Purpose | Status |
| --- | --- | --- | --- | --- | --- |
| P1 | `KA-001-3D - Base plate.step` | Base plate | 1 | Enclosure base | STEP available |
| P2 | `KA-002-3D - Frontal case.step` | Front case | 1 | Front panel with screen window | STEP available |
| P3 | `KA-003-3D - Main button.step` | Main button | 1 | Universal-button pusher | STEP available |
| P4 | `KA-004-3D - Counter button.step` | Counter button | 1 | Pusher for `+` and `-` buttons | STEP available |
| P5 | `KA-005-3D - Back case.step` | Back case | 1 | Enclosure back cover | STEP available |
| P6 | `KA-008-3D - Band holder.step` | Strap holder | 1 | Strap attachment to the enclosure | STEP available |
| P7 | `KA-009-3D - Band mashroom.step` | Strap fastener | 1 | Strap fixation | STEP available |

## Silicone Strap

| Item | STEP file | Name | Qty | Manufacturing | Status |
| --- | --- | --- | --- | --- | --- |
| S1 | `KA-007-S - Band.step` | Silicone strap | 1 | Cast with a 3D-printed mold | STEP available |

## 3D-Printed Silicone Mold

The file is in `hardware/3d-print/source/Molds/`. This is not a final device
part; it is tooling for producing the silicone strap.

| Item | STEP file | Name | Qty | Purpose | Status |
| --- | --- | --- | --- | --- | --- |
| M1 | `Molds/KA-100-M - Band mold part 1.step` | Strap casting mold, part 1 | 1 | 3D-printed mold for the silicone strap | STEP available; full mold set needs verification |

## CAD Assembly

| Item | STEP file | Name | Purpose | Status |
| --- | --- | --- | --- | --- |
| A1 | `KA-000-AS.step` | Full assembly | Check relative placement of enclosure, strap, and components | STEP available |

## Button Wiring

Buttons are connected between GPIO and `GND`; active level is `0`.

| Function | GPIO | Connection |
| --- | --- | --- |
| Add row | `GPIO2` | `GPIO2` -> button -> `GND` |
| Remove row | `GPIO16` | `GPIO16` -> button -> `GND` |
| Universal button | `GPIO17` | `GPIO17` -> button -> `GND` |

## Not Included in the BOM

| File | Why it is excluded |
| --- | --- |
| `Components/Type-C Ext CBL - MALE +conn.step` | USB Type-C male reference model for CAD insertion checks. The USB connector is already built into the Waveshare board. |

## Assembly Notes

- All three tactile buttons are identical 12x12 mm components; quantity is 3.
- The silicone strap is not printed as a final part; it is cast in a 3D-printed
  mold.
- `Molds/` currently contains `KA-100-M - Band mold part 1.step` for casting the
  silicone strap.
