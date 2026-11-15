#pragma once

#include "tiktok-live-session.h"
#include <QString>
#include <atomic>
#include <functional>
#include <memory>
#include <string>

struct TikTokOutputConfig {
    std::string method = "live-studio";
    std::string rapidApiKey;
    std::string cookieFilePath;
    std::string client = "pc";
    std::string title;
    std::string topicId;
    std::string gameId;
    bool matureContent = false;
    TikTokLiveSettings liveSettings;
    QJsonObject deviceProfiles;
    std::string streamlabsAccessToken;
    std::string streamlabsCategory;
    bool streamlabsMatureContent = false;
};

class TikTokOutputSession {
public:
    TikTokOutputSession() = default;
    ~TikTokOutputSession();
    bool Start(const TikTokOutputConfig &config, obs_output *output,
        std::function<void(const QJsonObject &)> saveDevices);
    void ConfirmStarted();
    void CancelPreparation();
    void Stop();
    QString Error() const;
    std::string Server() const;
    std::string Key() const;

private:
    std::unique_ptr<TikTokLiveSession> liveStudio_;
    std::string streamlabsAccessToken_;
    std::string streamlabsId_;
    std::string server_;
    std::string key_;
    QString error_;
    bool streamlabsStarted_ = false;
};
