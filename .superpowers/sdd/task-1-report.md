# Task 1: Extract standalone easing functions to Animation.hpp

## Implementation

Added 30 standalone `inline auto easing_name(float t) -> float` easing functions in the `neko::animation` namespace, right before the closing `}` brace.

## Files Changed

- `NekoUI/NekoUI/Widget/Component/Animation.hpp` — inserted 89 lines (functions + comments) at line 835, before the namespace closing brace. File grew from 835 to 924 lines.

## Functions Added

- `linear`
- `ease_in_sine`, `ease_out_sine`, `ease_in_out_sine`
- `ease_in_quad`, `ease_out_quad`, `ease_in_out_quad`
- `ease_in_cubic`, `ease_out_cubic`, `ease_in_out_cubic`
- `ease_in_quart`, `ease_out_quart`, `ease_in_out_quart`
- `ease_in_quint`, `ease_out_quint`, `ease_in_out_quint`
- `ease_in_expo`, `ease_out_expo`, `ease_in_out_expo`
- `ease_in_circ`, `ease_out_circ`, `ease_in_out_circ`
- `ease_in_back`, `ease_out_back`, `ease_in_out_back`
- `ease_in_elastic`, `ease_out_elastic`, `ease_in_out_elastic`
- `ease_in_bounce`, `ease_out_bounce`, `ease_in_out_bounce`

## Self-Review Findings

- Bounce functions correctly delegate to existing `out_bounce_impl` helper — no duplication.
- No existing code was modified or removed.
- LSP diagnostics shown after edit are all pre-existing (errors relating to `std::numbers` and `std::optional` not being recognized by the LSP compiler). The project builds with Intel C++ 2026 / MSVC v145 with `stdcpplatest` which has full `<numbers>` support.

## Commits

- `ee5a6da` — feat: extract 30 standalone easing functions to Animation.hpp
