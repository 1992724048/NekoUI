#pragma once
#include "../Widget.hpp"

namespace neko::widget {
    class Center final : public Widget {
    public:
        auto draw(Vec4I rect, engine::Context& context, backend::Backend& backend) -> Rect override;
    };
}
