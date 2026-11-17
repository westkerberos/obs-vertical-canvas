#pragma once

#include "tiktok-live-session.h"
#include <QWidget>
#include <optional>
#include <QJsonObject>

class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QSpinBox;

struct TikTokDualChoice {
    QString id;
    QString name;
    QString canvasId;
};

class TikTokDualLayoutWidget final : public QWidget {
public:
    explicit TikTokDualLayoutWidget(QWidget *parent = nullptr);
    void Load(const TikTokDualLayoutConfig &config);
    void Save(TikTokDualLayoutConfig &config) const;
    bool Enabled() const;
    bool Validate(QString *error) const;
    void SetAccess(std::optional<bool> available, bool cameraLive = true);
    void SetMethod(const QString &method);

private:
    struct AudioFields {
        QComboBox *encoder = nullptr;
        QComboBox *track = nullptr;
        QSpinBox *bitrate = nullptr;
        QJsonObject encoderParams;
    };
    struct VideoFields {
        QGroupBox *group = nullptr;
        QComboBox *scene = nullptr;
        QComboBox *canvas = nullptr;
        QComboBox *encoder = nullptr;
        QSpinBox *width = nullptr;
        QSpinBox *height = nullptr;
        QSpinBox *bitrate = nullptr;
        QSpinBox *fpsDivisor = nullptr;
        QComboBox *framing = nullptr;
        QJsonObject encoderParams;
        AudioFields audio;
    };

    void RefreshCatalog();
    void UpdateVisibility();
    static AudioFields CreateAudioFields(QWidget *parent, const QString &prefix);
    static void LoadAudio(const AudioFields &fields, const TikTokLayoutAudioConfig &config);
    static void SaveAudio(const AudioFields &fields, TikTokLayoutAudioConfig &config);
    VideoFields CreateVideoFields(QWidget *parent, const QString &prefix, bool portrait);
    static void LoadVideo(const VideoFields &fields, const TikTokLayoutVideoConfig &config);
    static void SaveVideo(const VideoFields &fields, TikTokLayoutVideoConfig &config);

    QCheckBox *enabled_ = nullptr;
    QCheckBox *sameAudio_ = nullptr;
    QGroupBox *settings_ = nullptr;
    VideoFields portrait_;
    VideoFields landscape_;
    AudioFields sharedAudio_;
    QLabel *accessStatus_ = nullptr;
    bool canDual_ = false;
    bool cameraLive_ = true;
    bool nativeMethod_ = true;
};
