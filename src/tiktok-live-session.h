#pragma once

#include "tiktok-live-settings.h"
#include <QObject>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <memory>
#include <string>

struct obs_output;

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

class TikTokLiveSession : public QObject {
public:
    TikTokLiveSession();
    ~TikTokLiveSession() override;
    bool Start(const TikTokOutputConfig &config, obs_output *output,
        std::function<void(const QJsonObject &)> saveDevices);
    void ConfirmStarted();
    void CancelPreparation();
    void Stop();
    QString Error() const;
    std::string Server() const;
    std::string Key() const;
private:
    struct State;
    std::unique_ptr<State> state_;
};
