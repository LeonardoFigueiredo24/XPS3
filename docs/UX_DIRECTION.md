# XPS3 interface — Cell Horizon

## Purpose

Cell Horizon is the XPS3 television-first interface for Xbox Developer Mode. It borrows the useful hierarchy of console media libraries—large selected content, a horizontal rail, readable settings cards and controller hints—but does not copy third-party branding, artwork or layouts.

The frontend must remain an honest view of emulator state. It never invents games, compatibility ratings, performance results or settings that are not wired to the RPCS3-based core.

## Visual system

| Role | Colour |
|---|---|
| Void background | `#080B16` |
| Deep Cell surface | `#101936` |
| Cobalt action | `#245CFF` |
| Iris depth | `#7A5CFF` |
| Cyan signal | `#42D7FF` |
| Warm warning | `#FFB45C` |
| Primary text | `#F4F6FF` |

The signature element is an abstract Cell horizon assembled from XAML circles and diagonal signal lines. It is deliberately lightweight and does not require a bitmap background. Segoe UI is retained because it is available on the target, reads well from a television and avoids adding a font dependency to the emulator package.

## Layout

- The header separates XPS3 identity from build and firmware status.
- The library gives the selected real game a cinematic hero panel and uses a horizontal controller-friendly rail.
- Install uses the same surfaces and displays only files found through the storage implementation.
- Settings lists only real core-backed values. A note explicitly explains this constraint.
- Dialog, loading, pause and error layers share the same surface, focus and status colours.
- Controller hints stay in the bottom-right safe area and use Xbox button semantics.

## Performance and accessibility

- Effects are gradients and simple vector geometry; there is no blur, video wallpaper or continuous decorative animation.
- Text sizes target viewing at television distance, while the 64 px outer margin preserves overscan safety.
- Cobalt focus/action, cyan information, amber warning and red error have distinct semantic roles.
- The horizontal library keeps the selected item and adjacent choices visible without shrinking titles.

## 120 Hz policy

The existing core exposes a 120 frame-limit choice. It is shown as a real advanced setting, not as a universal enhancement. A title still needs compatible timing or a tested patch, the Xbox output and television must negotiate 120 Hz, and measured performance must reach the target. Per-title results belong in the compatibility evidence, not in promotional UI.
