# Task 1 Report: Widget 基类重构

## Status
**DONE**

## Commit
`190eb86` — `feat: Widget 基类重构 —— 树结构、bounds、z-order、dirty 传播`

## Summary
Replaced entire `Widget.hpp` (25 lines) with new retained-mode base class (175 lines), adding:

| Feature | Description |
|---------|-------------|
| `Constraints` struct | Available area for layout (x, y, width, height, defaults INT_MAX) |
| Tree structure | `m_parent`/`m_children`, `register_child()`/`unregister_child()` (private, friend `Sub`) |
| Bounds | `glm::ivec4` with `set_bounds()`, `bounds()`, `x()`, `y()`, `width()`, `height()` |
| Z-order | `m_z_order`, `set_z_order()`, `z_order()` |
| Dirty propagation | `mark_dirty()` cascades to parent, `dirty()`, `clear_dirty()` |
| Visibility | `m_visible`, `set_visible()`, checked in `hit_test` |
| `hit_test()` | Virtual, default uses `mouse.is_inside(m_bounds)` |
| Default `draw()` | Iterates children sorted by z_order ascending |
| Default `update()` | Iterates children |
| Default `handle_event()` | Children first (z_order descending), then self hit-test for mouse events |
| Default `layout()` | Empty (subclass override) |
| Sort helpers | `children_sorted_asc()`, `children_sorted_desc()` using `std::ranges::sort` |

## Concerns
- No compile test possible until later tasks adapt dependent code. Pre-existing GLM include path issue (`glm/glm.hpp` not found by LSP) is unchanged — GLM lives at `../Library/glm/` and is resolved at build time.
- `std::ranges::find`/`sort` require `<algorithm>` which is not explicitly included but likely comes transitively; if compilation fails, add `#include <algorithm>`.
