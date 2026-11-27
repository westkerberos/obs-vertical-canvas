#pragma once
#include "tiktok-live-session.h"
#include <QJsonObject>
#include <QString>
#include <atomic>
#include <functional>
#include <optional>

struct AccountInfoResult {
    QString username;
    const char *errorKey = nullptr;
    QString applicationStatus;
    std::optional<bool> canGoLive;
    QString dualLayoutStatus;
    std::optional<bool> canDualLayout;
    QJsonObject deviceProfile;
    QString errorMessage;
};
using AccountInfoLookup = std::function<AccountInfoResult(const TikTokOutputConfig &, const std::atomic_bool &)>;
AccountInfoResult LookupAccountInfo(const TikTokOutputConfig &config, const std::atomic_bool &cancelled);
