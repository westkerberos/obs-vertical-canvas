#pragma once
#include "tiktok-live-session.h"
#include <QString>
class QWidget;
class TikTokLayoutScene {
public:
    TikTokLayoutScene() = default;
    TikTokLayoutScene(const TikTokLayoutScene &) = delete;
    TikTokLayoutScene &operator=(const TikTokLayoutScene &) = delete;
    ~TikTokLayoutScene();
    bool Create(const TikTokLayoutVideoConfig &config, QString *error);
    bool CreatePreview(const TikTokLayoutVideoConfig &config, QString *error);
    video_t *Video() const;
    void Render() const;
private:
    obs_canvas_t *canvas_ = nullptr;
    obs_scene_t *wrapper_ = nullptr;
    obs_source_t *source_ = nullptr;
    bool borrowedCanvas_ = false;
};
void ShowTikTokLayoutPreview(const TikTokLayoutVideoConfig &config, QWidget *parent);
