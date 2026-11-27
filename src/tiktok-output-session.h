#pragma once

#include "tiktok-live-session.h"
#include <QString>
#include <atomic>
#include <functional>
#include <memory>
#include <string>

class TikTokOutputSession {
public:
    TikTokOutputSession();
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
    struct DualFeedState;
    std::unique_ptr<DualFeedState> dual_;
    std::unique_ptr<TikTokLiveSession> liveStudio_;
    std::string streamlabsAccessToken_;
    std::string streamlabsId_;
    std::string server_;
    std::string key_;
    QString error_;
    std::atomic_bool stopping_{false};
    bool streamlabsStarted_ = false;
};
