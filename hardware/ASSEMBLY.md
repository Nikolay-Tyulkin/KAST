# Hardware Assembly

Assembly guide for the enclosure, board, buttons, battery, and strap.

## Assembly Steps

1. Print the 3D-printed parts.
2. Glue the 12x12 mm button modules into `KA-001-3D - Base plate.step`.
3. Solder the buttons according to the wiring plan and bend the legs inward so
   they do not protrude.
4. Mount the `Waveshare ESP32-S3-LCD-1.69` on
   `KA-001-3D - Base plate.step` with 4 M2x5 screws.
5. Connect the battery to the `Waveshare ESP32-S3-LCD-1.69`.
6. Insert `KA-003-3D - Main button.step` and
   `KA-004-3D - Counter button.step` into
   `KA-001-3D - Base plate.step`.
7. Insert `KA-001-3D - Base plate.step` into
   `KA-002-3D - Frontal case.step`.
8. Close the enclosure with `KA-005-3D - Back case.step` until it snaps in.
9. Thread the silicone strap `KA-007-S - Band.step` through
   `KA-008-3D - Band holder.step` and glue it to
   `KA-005-3D - Back case.step`.
10. Insert `KA-009-3D - Band mashroom.step` into the strap.

Use `KA-000-AS.step` as the component placement reference.

Components and wiring are documented in [`BOM.md`](BOM.md).
