#include "account-info.h"
#include "streamlabs-info.h"
#include "tiktok-native/live_client.hpp"
#include "tiktok-native/request_cancellation.hpp"
#include <QJsonDocument>
#include <curl/curl.h>

AccountInfoResult LookupAccountInfo(const TikTokOutputConfig &config, const std::atomic_bool &cancelled)
{
    if (cancelled)
        return {};

    static const auto initialized = curl_global_init(CURL_GLOBAL_DEFAULT);
    if (initialized != CURLE_OK)
        return {{}, "Account.NetworkError"};

    TikTokRequestCancellation cancellation(&cancelled);
    if (config.method == "streamlabs") {
        const auto response = FetchStreamlabsInfo(config.streamlabsAccessToken, cancelled);
        if (response.errorKey)
            return {{}, response.errorKey};

        AccountInfoResult result;
        result.username = response.data.value("user").toObject().value("username").toString().trimmed();
        result.applicationStatus = response.data.value("application_status").toObject().value("status").toString();
        const auto canLive = response.data.value("can_be_live");
        if (canLive.isBool())
            result.canGoLive = canLive.toBool();
        if (result.username.isEmpty() && result.applicationStatus.isEmpty() && !result.canGoLive.has_value())
            result.errorKey = "Account.Unavailable";
        return result;
    }

    if (config.method != "live-studio" && config.method != "live-studio-mobile")
        return {};

    TikTokStudioAccountCredentials account;
    account.rapidapi_key = QString::fromStdString(config.rapidApiKey);
    account.mobile = config.client == "mobile";
    account.device_profile = config.deviceProfiles.value(QString::fromStdString(config.client)).toObject();
    account.device_id = account.device_profile.value("device_id").toString();
    account.install_id = account.device_profile.value("install_id").toString();
    account.live_studio_version = account.device_profile.value("live_studio_version").toString("1.27.0");

    QString error;
    account.cookie_jar = read_tiktok_cookie_file(QString::fromStdString(config.cookieFilePath), &error);
    if (!error.isEmpty()) {
        AccountInfoResult invalid;
        invalid.errorKey = "Account.InvalidCookies";
        invalid.errorMessage = error;
        return invalid;
    }
    if (cancelled)
        return {};

    AccountInfoResult result;
    if (account.mobile) {
        error = initialize_tiktok_account(account);
        result.deviceProfile = account.device_profile;
        if (!error.isEmpty()) {
            result.errorKey = "Account.DeviceSetupFailed";
            result.errorMessage = error;
            return result;
        }
    }

    result.username = lookup_tiktok_username(account, &error);
    if (result.username.isEmpty() || !error.isEmpty()) {
        result.errorKey = "Account.TikTokUnavailable";
        result.errorMessage = error;
    }
    if (cancelled)
        return {};

    const auto access = lookup_tiktok_dual_access(account);
    result.canDualLayout = access.available;
    result.dualLayoutStatus = access.reason;
    return result;
}
