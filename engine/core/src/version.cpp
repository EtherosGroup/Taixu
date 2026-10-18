#include "taixu/core/version.hpp"

#if defined(__clang__)
#define TXU_IMPL_COMPILER "clang " __clang_version__
#elif defined(_MSC_VER)
#define TXU_IMPL_COMPILER "msvc"
#elif defined(__GNUC__)
#define TXU_IMPL_COMPILER "gcc " __VERSION__
#else
#define TXU_IMPL_COMPILER "unknown"
#endif

#if defined(NDEBUG)
#define TXU_IMPL_CONFIG " release"
#else
#define TXU_IMPL_CONFIG " debug"
#endif

#define TXU_IMPL_STRINGIFY_(x) #x
#define TXU_IMPL_STRINGIFY(x) TXU_IMPL_STRINGIFY_(x)

namespace taixu::core
{

    const char* version_string() noexcept
    {
        return TXU_IMPL_STRINGIFY(TXU_VERSION_MAJOR) "." TXU_IMPL_STRINGIFY(TXU_VERSION_MINOR) "." TXU_IMPL_STRINGIFY(
            TXU_VERSION_PATCH);
    }

    const char* build_id() noexcept { return TXU_IMPL_COMPILER TXU_IMPL_CONFIG; }

} // namespace taixu::core
