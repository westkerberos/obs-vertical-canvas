#pragma once

#include "tiktok-live-settings.h"
#include <QObject>
#include <QJsonObject>
#include <QString>
#include <functional>
#include <memory>
#include <string>
#include <vector>

struct obs_output;

struct TikTokLayoutAudioConfig {
    std::string encoderId = "ffmpeg_aac";
    int mixer = 0;
    int bitrate = 160;
    QJsonObject encoderParams;
};
struct TikTokLayoutVideoConfig {
    std::string sceneUuid, sceneName, encoderId;
    int width = 1080, height = 1920, bitrate = 6000;
    bool crop = false;
    TikTokLayoutAudioConfig audio;
    int fpsDivisor = 1;
    QJsonObject encoderParams;
    std::string canvasUuid;
};
struct TikTokDualLayoutConfig {
    bool enabled = false;
    bool sameAudio = true;
    bool independentAudioInitialized = false;
    TikTokLayoutAudioConfig sharedAudio;
    TikTokLayoutVideoConfig portrait;
    TikTokLayoutVideoConfig landscape = [] {
        TikTokLayoutVideoConfig value;
        value.width = 1920; value.height = 1080;
        return value;
    }();
};

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
    bool monitorViolations = true;
    TikTokDualLayoutConfig dualLayout;
    bool UsesDualLayout() const { return dualLayout.enabled; }
};
class TikTokLiveSession : public QObject {
public:
    TikTokLiveSession();
    ~TikTokLiveSession() override;
    bool Start(const TikTokOutputConfig &config, obs_output *output,
        std::function<void(const QJsonObject &)> saveDevices);
    bool Start(const TikTokOutputConfig &config, const std::vector<obs_output *> &outputs,
        std::function<void(const QJsonObject &)> saveDevices);
    void ConfirmStarted();
    void CancelPreparation();
    void Stop();
    QString Error() const;
    std::string Server(bool landscape = false) const;
    std::string Key(bool landscape = false) const;
private:
    struct State;
    std::unique_ptr<State> state_;
};
