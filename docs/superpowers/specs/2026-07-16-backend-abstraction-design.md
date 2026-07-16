# Backend 抽象化设计

## 目标

将 Backend 从具体的 DX11 final class 重构为纯抽象接口，DX11 实现移至 DirectX11 子目录，
DX11 Device/Context 的创建移至 main.cpp（类似 ImGui 风格），Engine 改为依赖注入接收 Backend 对象。

## 架构变更

```
Before:                     After:
Engine(HWND)                Engine(unique_ptr<Backend>)
  └── owns Backend(fxnal)     └── owns Backend (abstract)
        ├── DX11 code                  ▲
        ├── HLSL shaders               │
        ├── font atlas          implements
        └── stb_truetype               │
                                  DirectX11 final : Backend
                                    ├── DX11 code
                                    ├── HLSL shaders
                                    ├── font atlas
                                    └── stb_truetype

main.cpp:
  Engine eng(hwnd);          ID3D11Device* dev;
                              ID3D11DeviceContext* ctx;
                              D3D11CreateDevice(...);
                              auto dx11 = make_unique<DirectX11>(dev, ctx, hwnd);
                              auto eng  = make_unique<Engine>(move(dx11));
```

## 接口设计

### Backend（纯抽象基类）

```cpp
namespace neko::backend {

class Backend {
public:
    virtual ~Backend() = default;

    // Lifecycle
    virtual auto resize(int width, int height) -> void = 0;
    virtual auto set_dpi(float dpi) -> void = 0;
    virtual auto get_dpi_scale() const -> float = 0;

    // Frame
    virtual auto begin() -> void = 0;
    virtual auto end() -> void = 0;

    // Drawing
    virtual auto draw_rect_fill(float x, float y, float w, float h,
                                uint32_t color, float radius = 0) -> void = 0;
    virtual auto draw_rect(float x, float y, float w, float h,
                           uint32_t color, float thickness = 1) -> void = 0;
    virtual auto draw_line(float x1, float y1, float x2, float y2,
                           uint32_t color, float thickness = 1) -> void = 0;
    virtual auto draw_circle_fill(float cx, float cy, float r,
                                  uint32_t color) -> void = 0;
    virtual auto draw_text(float x, float y, std::string_view text,
                           uint32_t color, float size = 16) -> void = 0;
};

} // namespace neko::backend
```

### 移除项

- `friend class engine::Engine` — Engine 通过接口访问，无需友元
- `Backend.cpp` — 纯接口无实现，删除
- `#include <d3d11.h>` / `<dxgi1_2.h>` / `DirectXMath.h` — 移至 DirectX11
- HLSL 着色器字符串常量 — 移至 DirectX11
- `stb_truetype.h` — 移至 DirectX11 目录
- 所有 ID3D11* 成员变量 — 移至 DirectX11

## DirectX11 具体实现

```cpp
namespace neko::backend {

class DirectX11 final : public Backend {
public:
    DirectX11(ID3D11Device* device, ID3D11DeviceContext* ctx, HWND hwnd);
    ~DirectX11() override;

    // Backend interface
    auto resize(int width, int height) -> void override;
    auto set_dpi(float dpi) -> void override;
    auto get_dpi_scale() const -> float override;
    auto begin() -> void override;
    auto end() -> void override;
    auto draw_rect_fill(...) -> void override;
    // ... 其余方法
};

} // namespace neko::backend
```

### 构造函数职责

- 接收 `ID3D11Device*` + `ID3D11DeviceContext*`（由 main.cpp 创建）
- 接收 `HWND`，内部创建 SwapChain + RTV
- 调用 `init_shaders()` / `init_states()` / `init_font()`

### 析构函数

- 释放所有 ID3D11 COM 指针（同当前 Backend 析构逻辑）

## Engine 变更

```cpp
// Engine.hpp
class Engine final {
public:
    explicit Engine(std::unique_ptr<Backend> backend);
    // ... 其余不变
private:
    std::unique_ptr<Backend> backend_;  // 类型从具体 Backend 改为抽象
};
```

- Engine 不再接收 HWND（Engine 不需要窗口句柄）
- 删除 `backend = std::make_unique<backend::Backend>(hwnd)` 构造逻辑
- EventRouter 仍引用 Backend（通过抽象接口指针）

## main.cpp 变更

```cpp
// DX11 device/context 创建
ComPtr<ID3D11Device> device;
ComPtr<ID3D11DeviceContext> ctx;
D3D11CreateDevice(
    nullptr, D3D_DRIVER_TYPE_HARDWARE,
    nullptr, D3D11_CREATE_DEVICE_DEBUG,
    nullptr, 0, D3D11_SDK_VERSION,
    &device, nullptr, &ctx
);

auto backend = std::make_unique<backend::DirectX11>(device.Get(), ctx.Get(), hwnd);
auto engine  = std::make_unique<engine::Engine>(std::move(backend));
```

## 工程配置

- `NekoUI.vcxproj`：移除 `Backend.cpp` 编译项（纯接口无实现）
- `NekoUI.vcxproj.filters`：移除 `Backend.cpp` 过滤项
- `DirectX11.cpp` 已在 vcxproj 中（原为空占位），无需添加

## 文件清单

| 文件 | 操作 | 说明 |
|------|------|------|
| `Backend/Backend.hpp` | 重写 | 变为纯抽象接口，删除 DX11 依赖 |
| `Backend/Backend.cpp` | 删除 | 内容移至 DirectX11.cpp |
| `Backend/DirectX11/DirectX11.hpp` | 重写 | 从空占位变为 class DirectX11 final |
| `Backend/DirectX11/DirectX11.cpp` | 重写 | 实现所有虚方法（来自旧 Backend.cpp） |
| `Backend/stb_truetype.h` | 保持 | 字体光栅化依赖 |
| `Engine/Engine.hpp` | 修改 | Backend 参数替换 HWND |
| `Engine/Engine.cpp` | 修改 | 移除 Backend 创建逻辑 |
| `main.cpp` | 修改 | 新增 D3D11CreateDevice + DirectX11 构造 |
| `NekoUI.vcxproj` | 修改 | 移除 Backend.cpp |
| `NekoUI.vcxproj.filters` | 修改 | 移除 Backend.cpp |

## 边界情况与错误处理

- D3D11CreateDevice 失败 → main.cpp 抛异常（同现有风格）
- SwapChain 创建失败（如 HWND 无效）→ DirectX11 构造函数抛异常
- resize 传入 0 宽/高 → DirectX11 内部跳过 SwapChain ResizeBuffers
- 绘制方法不检查指针有效性（假设 begin/end 成对调用，同现有行为）

## 后续扩展（非本次）

- 可添加 `D3D12Backend`、`VulkanBackend` 等子类
- 可引入 BackendFactory 按平台选择具体后端
