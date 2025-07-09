#include <stdint.h>

#include "hilog/log_c.h"
#include "hilog/log_cpp.h"

using namespace OHOS;


constexpr uint32_t DOMAIN_OS_MIN = 0xD000000; // See hilog_common.h

static constexpr OHOS::HiviewDFX::HiLogLabel LOG_LABEL = { LOG_APP, DOMAIN_OS_MIN + 1, "HILOG_DEMO" };
#define LOGF(LOG_LABEL, fmt, args...) \
        HILOG_IMPL(LOG_CORE, LOG_FATAL, LOG_LABEL.domain, LOG_LABEL.tag,    \
            "%{public}s %{public}d: " fmt, __FUNCTION__, __LINE__, ##args)

int main() {
    LOGF(LOG_LABEL, "Hello World");
    return 0;
}
