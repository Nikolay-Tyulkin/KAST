# CAD Source Files

Source STEP files for the KAST enclosure and components.

## Assembly

| File | Purpose |
| --- | --- |
| `KA-000-AS - external band.step` | Full device assembly with the standard 22 mm purchased band holder |
| `KA-000-AS.step` | Legacy full device assembly with the cast silicone strap |

## 3D-Printed Parts

| File | Purpose |
| --- | --- |
| `KA-001-3D - Base plate.step` | Base plate |
| `KA-002-3D - Frontal case.step` | Front case |
| `KA-003-3D - Main button.step` | Main universal button |
| `KA-004-3D - Counter button.step` | Row-counter buttons |
| `KA-005-3D - Back case.step` | Back case |
| `KA-008-3D - Band holder V2 (for extrnal band).step` | Standard 22 mm purchased band holder |
| `KA-008-3D - Band holder.step` | Legacy cast silicone strap holder |
| `KA-009-3D - Band mashroom.step` | Legacy cast silicone strap fastener |

## Strap

| File | Purpose |
| --- | --- |
| `KA-007-S - Band.step` | Legacy cast silicone strap |

## Components: Purchased Parts and CAD References

The `Components/` folder contains STEP models for purchased or ready-made
components. It may also contain reference models used for assembly checks even
when those models are not part of the BOM.

| File | Purpose |
| --- | --- |
| `Components/ESP32-S3-Lcd-1_69_3D.step` | `Waveshare ESP32-S3-LCD-1.69` board model |
| `Components/button 12x12.step` | 12x12 mm button model |
| `Components/Type-C Ext CBL - MALE +conn.step` | USB Type-C male CAD reference for insertion checks; not included in the BOM |

## Molds: Silicone Strap Tooling

The `Molds/` folder contains 3D-printed tooling for producing the strap. This is
not a final device part; it is a mold for casting silicone.

| File | Purpose |
| --- | --- |
| `Molds/KA-100-M - Band mold part 1.step` | Part 1 of the silicone strap casting mold |
