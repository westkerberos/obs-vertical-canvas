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
    struct LayoutAudio {
        std::string encoderId = "ffmpeg_aac";
        int mixer = 0;
        int bitrate = 160;
        QJsonObject encoderParams;
    };
    struct LayoutVideo {
        std::string sceneUuid, sceneName, encoderId;
        int width = 1080, height = 1920, bitrate = 6000;
        bool crop = false;
        LayoutAudio audio;
        int fpsDivisor = 1;
        QJsonObject encoderParams;
        std::string canvasUuid;
    };
    struct DualLayout {
        bool enabled = false;
        bool sameAudio = true;
        bool independentAudioInitialized = false;
        LayoutAudio sharedAudio;
        LayoutVideo portrait;
        LayoutVideo landscape = LayoutVideo{"", "", "", 1920, 1080};
    } dualLayout;
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
