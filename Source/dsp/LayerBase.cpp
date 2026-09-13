#include "LayerBase.h"

// LayerBase is header-only by design (everything is small and inlinable);
// this translation unit exists so the class has a home for its vtable and so
// the header is compiled at least once on its own.

namespace horizon
{
    static_assert (kNumLayers == 4, "Horizon Pad is built around exactly four layers");
    static_assert (kMaxVoices >= 1, "Need at least one voice");
}
