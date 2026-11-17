#include "tiktok-dual-layout-widget.h"

#include <obs-module.h>
#include <obs.h>
#include <obs.hpp>

#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QStandardItemModel>
#include <QSignalBlocker>

#include <algorithm>
#include <cstring>

namespace {
struct TikTokDualCatalog {
    QVector<TikTokDualChoice> scenes;
    QVector<TikTokDualChoice> canvases;
    QVector<TikTokDualChoice> videoEncoders;
    QVector<TikTokDualChoice> audioEncoders;
};

bool EnumTikTokDualScene(void *opaque, obs_source_t *source)
{
    auto catalog = static_cast<TikTokDualCatalog *>(opaque);
    auto canvas = obs_source_get_canvas(source);
    if (!canvas)
        return true;
    QString name = QString::fromUtf8(obs_source_get_name(source));
    if (!(obs_canvas_get_flags(canvas) & MAIN))
        name += " — " + QString::fromUtf8(obs_canvas_get_name(canvas));
    catalog->scenes.push_back({QString::fromUtf8(obs_source_get_uuid(source)), name,
        QString::fromUtf8(obs_canvas_get_uuid(canvas))});
    obs_canvas_release(canvas);
    return true;
}

TikTokDualCatalog ReadTikTokDualCatalog()
{
    TikTokDualCatalog result;
    if (!obs_initialized())
        return result;

    obs_enum_canvases([](void *opaque, obs_canvas_t *canvas) {
        if (obs_canvas_removed(canvas) || (obs_canvas_get_flags(canvas) & EPHEMERAL))
            return true;
        if (!(obs_canvas_get_flags(canvas) & MAIN)) {
            auto catalog = static_cast<TikTokDualCatalog *>(opaque);
            catalog->canvases.push_back({QString::fromUtf8(obs_canvas_get_uuid(canvas)),
                QString::fromUtf8(obs_canvas_get_name(canvas)), QString::fromUtf8(obs_canvas_get_uuid(canvas))});
        }
        obs_canvas_enum_scenes(canvas, EnumTikTokDualScene, opaque);
        return true;
    }, &result);

    const char *encoderId = nullptr;
    for (size_t index = 0; obs_enum_encoder_types(index, &encoderId); ++index) {
        if (!encoderId || (obs_get_encoder_caps(encoderId) & (OBS_ENCODER_CAP_DEPRECATED | OBS_ENCODER_CAP_INTERNAL)))
            continue;
        const char *codec = obs_get_encoder_codec(encoderId);
        if (!codec)
            continue;
        const TikTokDualChoice choice{QString::fromUtf8(encoderId),
            QString::fromUtf8(obs_encoder_get_display_name(encoderId)), {}};
        if (!strcmp(codec, "h264") || !strcmp(codec, "hevc"))
            result.videoEncoders.push_back(choice);
        else if (!strcmp(codec, "aac"))
            result.audioEncoders.push_back(choice);
    }
    return result;
}

QSpinBox *MakeNumber(QWidget *parent, const QString &name, int minimum, int maximum, int value, int step = 1)
{
    auto number = new QSpinBox(parent);
    number->setObjectName(name);
    number->setRange(minimum, maximum);
    number->setSingleStep(step);
    number->setValue(value);
    return number;
}

void PopulateCombo(QComboBox *combo, const QVector<TikTokDualChoice> &choices)
{
    const auto selectedId = combo->currentData().toString();
    QSignalBlocker blocker(combo);
    combo->clear();
    for (const auto &choice : choices) {
        combo->addItem(choice.name, choice.id);
        combo->setItemData(combo->count() - 1, choice.name, Qt::UserRole + 1);
    }
    int index = combo->findData(selectedId);
    if (index < 0 && !selectedId.isEmpty()) {
        combo->addItem(selectedId + " (" + obs_module_text("Dual.Missing") + ")", selectedId);
        index = combo->count() - 1;
    }
    if (index >= 0)
        combo->setCurrentIndex(index);
}
}

TikTokDualLayoutWidget::TikTokDualLayoutWidget(QWidget *parent)
    : QWidget(parent)
{
    auto root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    enabled_ = new QCheckBox(obs_module_text("Dual.Enable"), this);
    enabled_->setObjectName("tiktokDualLayoutEnabled");
    root->addWidget(enabled_);

    accessStatus_ = new QLabel(this);
    accessStatus_->setObjectName("tiktokDualLayoutAccess");
    accessStatus_->setWordWrap(true);
    accessStatus_->setTextFormat(Qt::PlainText);
    root->addWidget(accessStatus_);

    settings_ = new QGroupBox(obs_module_text("Dual.Configuration"), this);
    settings_->setObjectName("tiktokDualLayoutSettings");
    auto settingsLayout = new QVBoxLayout(settings_);
    settingsLayout->setContentsMargins(8, 10, 8, 8);

    auto videoRow = new QHBoxLayout;
    portrait_ = CreateVideoFields(settings_, "portrait", true);
    landscape_ = CreateVideoFields(settings_, "landscape", false);
    videoRow->addWidget(portrait_.group, 1);
    videoRow->addWidget(landscape_.group, 1);
    settingsLayout->addLayout(videoRow);

    sameAudio_ = new QCheckBox(obs_module_text("Dual.SameAudio"), settings_);
    sameAudio_->setObjectName("tiktokDualSameAudio");
    sameAudio_->setChecked(true);
    settingsLayout->addWidget(sameAudio_);

    auto sharedGroup = new QGroupBox(obs_module_text("Dual.SharedAudio"), settings_);
    auto sharedLayout = new QFormLayout(sharedGroup);
    sharedAudio_ = CreateAudioFields(sharedGroup, "shared");
    sharedLayout->addRow(obs_module_text("Dual.AudioEncoder"), sharedAudio_.encoder);
    sharedLayout->addRow(obs_module_text("Dual.AudioMixer"), sharedAudio_.track);
    sharedLayout->addRow(obs_module_text("Dual.BitrateKbps"), sharedAudio_.bitrate);
    settingsLayout->addWidget(sharedGroup);

    auto independentAudioRow = new QHBoxLayout;
    auto portraitAudioGroup = new QGroupBox(obs_module_text("Dual.PortraitAudio"), settings_);
    auto portraitAudioForm = new QFormLayout(portraitAudioGroup);
    portrait_.audio = CreateAudioFields(portraitAudioGroup, "portrait");
    portraitAudioForm->addRow(obs_module_text("Dual.AudioEncoder"), portrait_.audio.encoder);
    portraitAudioForm->addRow(obs_module_text("Dual.AudioMixer"), portrait_.audio.track);
    portraitAudioForm->addRow(obs_module_text("Dual.BitrateKbps"), portrait_.audio.bitrate);
    auto landscapeAudioGroup = new QGroupBox(obs_module_text("Dual.LandscapeAudio"), settings_);
    auto landscapeAudioForm = new QFormLayout(landscapeAudioGroup);
    landscape_.audio = CreateAudioFields(landscapeAudioGroup, "landscape");
    landscapeAudioForm->addRow(obs_module_text("Dual.AudioEncoder"), landscape_.audio.encoder);
    landscapeAudioForm->addRow(obs_module_text("Dual.AudioMixer"), landscape_.audio.track);
    landscapeAudioForm->addRow(obs_module_text("Dual.BitrateKbps"), landscape_.audio.bitrate);
    independentAudioRow->addWidget(portraitAudioGroup, 1);
    independentAudioRow->addWidget(landscapeAudioGroup, 1);
    settingsLayout->addLayout(independentAudioRow);
    root->addWidget(settings_);

    QObject::connect(enabled_, &QCheckBox::toggled, this, [this] { UpdateVisibility(); });
    QObject::connect(sameAudio_, &QCheckBox::toggled, this, [this] { UpdateVisibility(); });
    for (auto fields : {&portrait_, &landscape_}) {
        QObject::connect(fields->canvas, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, fields] {
            const auto id = fields->canvas->currentData().toString();
            if (!id.isEmpty()) {
                OBSCanvasAutoRelease canvas = obs_get_canvas_by_uuid(id.toUtf8().constData());
                obs_video_info info{};
                if (canvas && obs_canvas_get_video_info(canvas, &info)) {
                    fields->width->setValue(static_cast<int>(info.output_width));
                    fields->height->setValue(static_cast<int>(info.output_height));
                }
            }
            RefreshCatalog();
        });
    }
    RefreshCatalog();
    SetAccess(std::nullopt);
    UpdateVisibility();
}

TikTokDualLayoutWidget::AudioFields TikTokDualLayoutWidget::CreateAudioFields(QWidget *parent, const QString &prefix)
{
    AudioFields fields;
    fields.encoder = new QComboBox(parent);
    fields.encoder->setObjectName(prefix + "DualAudioEncoder");
    fields.track = new QComboBox(parent);
    fields.track->setObjectName(prefix + "DualAudioMixer");
    for (int i = 0; i < MAX_AUDIO_MIXES; ++i)
        fields.track->addItem(QString::number(i + 1), i);
    fields.bitrate = MakeNumber(parent, prefix + "DualAudioBitrate", 32, 320, 160);
    return fields;
}

TikTokDualLayoutWidget::VideoFields TikTokDualLayoutWidget::CreateVideoFields(QWidget *parent,
    const QString &prefix, bool portrait)
{
    VideoFields fields;
    fields.group = new QGroupBox(obs_module_text(portrait ? "Dual.PortraitFeed" : "Dual.LandscapeFeed"), parent);
    fields.group->setObjectName(prefix + "DualVideoSettings");
    auto form = new QFormLayout(fields.group);
    fields.canvas = new QComboBox(fields.group);
    fields.canvas->setObjectName(prefix + "DualCanvas");
    fields.scene = new QComboBox(fields.group);
    fields.scene->setObjectName(prefix + "DualScene");
    fields.encoder = new QComboBox(fields.group);
    fields.encoder->setObjectName(prefix + "DualVideoEncoder");
    fields.width = MakeNumber(fields.group, prefix + "DualWidth", 64, 4096, portrait ? 1080 : 1920, 2);
    fields.height = MakeNumber(fields.group, prefix + "DualHeight", 64, 4096, portrait ? 1920 : 1080, 2);
    fields.bitrate = MakeNumber(fields.group, prefix + "DualVideoBitrate", 100, 50000, 6000);
    fields.fpsDivisor = MakeNumber(fields.group, prefix + "DualFPSDivisor", 1, 6, 1);
    fields.framing = new QComboBox(fields.group);
    fields.framing->setObjectName(prefix + "DualFraming");
    fields.framing->addItem(obs_module_text("Dual.Fit"), false);
    fields.framing->addItem(obs_module_text("Dual.Crop"), true);
    form->addRow(obs_module_text("Primary.Canvas"), fields.canvas);
    form->addRow(obs_module_text("Scene"), fields.scene);
    form->addRow(obs_module_text("Encoder"), fields.encoder);
    form->addRow(obs_module_text("Dual.Width"), fields.width);
    form->addRow(obs_module_text("Dual.Height"), fields.height);
    form->addRow(obs_module_text("Dual.VideoBitrate"), fields.bitrate);
    form->addRow(obs_module_text("Dual.FPSDivisor"), fields.fpsDivisor);
    form->addRow(obs_module_text("Dual.Framing"), fields.framing);
    return fields;
}

void TikTokDualLayoutWidget::RefreshCatalog()
{
    const auto catalog = ReadTikTokDualCatalog();
    for (auto fields : {&portrait_, &landscape_}) {
        const QString currentScene = fields->scene->currentData().toString();
        const QString currentCanvas = fields->canvas->currentData().toString();
        PopulateCombo(fields->canvas, catalog.canvases);
        if (!currentCanvas.isEmpty())
            fields->canvas->setCurrentIndex(fields->canvas->findData(currentCanvas));
        else {
            fields->canvas->insertItem(0, obs_module_text("Primary.NewCanvas"), QString());
            fields->canvas->setCurrentIndex(0);
        }
        QVector<TikTokDualChoice> scenes;
        const QString selectedCanvas = fields->canvas->currentData().toString();
        for (const auto &scene : catalog.scenes) {
            if (selectedCanvas.isEmpty() || scene.canvasId == selectedCanvas)
                scenes.push_back(scene);
        }
        PopulateCombo(fields->scene, scenes);
        if (!currentScene.isEmpty())
            fields->scene->setCurrentIndex(fields->scene->findData(currentScene));
        PopulateCombo(fields->encoder, catalog.videoEncoders);
        PopulateCombo(fields->audio.encoder, catalog.audioEncoders);
    }
    PopulateCombo(sharedAudio_.encoder, catalog.audioEncoders);
}

void TikTokDualLayoutWidget::Load(const TikTokDualLayoutConfig &config)
{
    QSignalBlocker enabledBlock(enabled_);
    QSignalBlocker audioBlock(sameAudio_);
    enabled_->setChecked(config.enabled);
    pendingSavedDual_ = config.enabled && !canDual_;
    sameAudio_->setChecked(config.sameAudio);
    LoadVideo(portrait_, config.portrait);
    LoadVideo(landscape_, config.landscape);
    LoadAudio(sharedAudio_, config.sharedAudio);
    RefreshCatalog();
    UpdateVisibility();
}

void TikTokDualLayoutWidget::Save(TikTokDualLayoutConfig &config) const
{
    config.enabled = Enabled();
    config.sameAudio = sameAudio_->isChecked();
    config.independentAudioInitialized = !config.sameAudio;
    SaveVideo(portrait_, config.portrait);
    SaveVideo(landscape_, config.landscape);
    SaveAudio(sharedAudio_, config.sharedAudio);
}

bool TikTokDualLayoutWidget::Enabled() const
{
    return enabled_->isChecked();
}

bool TikTokDualLayoutWidget::Validate(QString *error) const
{
    if (!Enabled())
        return true;
    if (!canDual_ || !cameraLive_ || !nativeMethod_) {
        if (error) *error = accessStatus_->text();
        return false;
    }
    const auto checkVideo = [&](const VideoFields &fields, bool portrait) {
        if (fields.scene->currentData().toString().isEmpty()) {
            if (error) *error = QString(obs_module_text("Dual.Error.SceneRequired")).arg(obs_module_text(portrait ? "Dual.Portrait" : "Dual.Landscape"));
            return false;
        }
        if (fields.encoder->currentData().toString().isEmpty()) {
            if (error) *error = QString(obs_module_text("Dual.Error.EncoderRequired")).arg(obs_module_text(portrait ? "Dual.Portrait" : "Dual.Landscape"));
            return false;
        }
        const int width = fields.width->value(), height = fields.height->value();
        if (width % 2 || height % 2 || (portrait ? width >= height : width <= height)) {
            if (error) *error = QString(obs_module_text("Dual.Error.Resolution")).arg(obs_module_text(portrait ? "Dual.Portrait" : "Dual.Landscape"));
            return false;
        }
        if (fields.bitrate->value() < 100 || fields.fpsDivisor->value() < 1 || fields.fpsDivisor->value() > 6) {
            if (error) *error = QString(obs_module_text("Dual.Error.VideoSettings")).arg(obs_module_text(portrait ? "Dual.Portrait" : "Dual.Landscape"));
            return false;
        }
        return true;
    };
    const auto checkAudio = [&](const AudioFields &fields, const char *name) {
        if (fields.encoder->currentData().toString().isEmpty() || fields.bitrate->value() < 32 ||
            fields.bitrate->value() > 320 || fields.track->currentData().toInt() < 0 ||
            fields.track->currentData().toInt() >= MAX_AUDIO_MIXES) {
            if (error) {
            const char *layoutKey = strcmp(name, "shared") == 0 ? "Dual.SharedAudio" :
                strcmp(name, "portrait") == 0 ? "Dual.Portrait" : "Dual.Landscape";
            *error = QString(obs_module_text("Dual.Error.AudioSettings")).arg(obs_module_text(layoutKey));
        }
            return false;
        }
        return true;
    };
    return checkVideo(portrait_, true) && checkVideo(landscape_, false) &&
        (sameAudio_->isChecked() ? checkAudio(sharedAudio_, "shared") :
            (checkAudio(portrait_.audio, "portrait") && checkAudio(landscape_.audio, "landscape")));
}

void TikTokDualLayoutWidget::SetAccess(std::optional<bool> available, bool cameraLive)
{
    cameraLive_ = cameraLive;
    if (available.has_value()) {
        canDual_ = *available;
        if (pendingSavedDual_ && canDual_ && cameraLive_ && nativeMethod_) {
            enabled_->setChecked(true);
            pendingSavedDual_ = false;
        } else if (!canDual_) {
            pendingSavedDual_ = false;
        }
    }
    const bool allowed = canDual_ && cameraLive_ && nativeMethod_;
    enabled_->setEnabled(allowed || (pendingSavedDual_ && cameraLive_ && nativeMethod_));
    if (!allowed && !pendingSavedDual_)
        enabled_->setChecked(false);
    if (!cameraLive_)
        accessStatus_->setText(obs_module_text("Dual.Error.CameraOnly"));
    else if (!available.has_value())
        accessStatus_->setText(obs_module_text("Dual.AccessUnknown"));
    else if (!*available)
        accessStatus_->setText(obs_module_text("Dual.AccessLocked"));
    else if (!nativeMethod_)
        accessStatus_->setText(obs_module_text("Dual.NativeOnly"));
    else
        accessStatus_->setText(obs_module_text("Dual.AccessAvailable"));
}

void TikTokDualLayoutWidget::SetMethod(const QString &method)
{
    nativeMethod_ = method == "live-studio" || method == "live-studio-mobile";
    enabled_->setEnabled(canDual_ && cameraLive_ && nativeMethod_);
    if (!nativeMethod_)
        enabled_->setChecked(false);
    SetAccess(canDual_ ? std::optional<bool>(true) : std::nullopt, cameraLive_);
}

void TikTokDualLayoutWidget::UpdateVisibility()
{
    settings_->setVisible(Enabled());
    sharedAudio_.encoder->parentWidget()->setVisible(sameAudio_->isChecked());
    for (auto fields : {&portrait_, &landscape_})
        fields->audio.encoder->parentWidget()->setVisible(!sameAudio_->isChecked());
}

void TikTokDualLayoutWidget::LoadAudio(AudioFields &fields, const TikTokLayoutAudioConfig &config)
{
    fields.encoderParams = config.encoderParams;
    const int index = fields.encoder->findData(QString::fromStdString(config.encoderId));
    if (index >= 0) fields.encoder->setCurrentIndex(index);
    const int track = fields.track->findData(config.mixer);
    if (track >= 0) fields.track->setCurrentIndex(track);
    fields.bitrate->setValue(config.bitrate);
}

void TikTokDualLayoutWidget::SaveAudio(const AudioFields &fields, TikTokLayoutAudioConfig &config)
{
    config.encoderId = fields.encoder->currentData().toString().toStdString();
    config.mixer = fields.track->currentData().toInt();
    config.bitrate = fields.bitrate->value();
    config.encoderParams = fields.encoderParams;
}

void TikTokDualLayoutWidget::LoadVideo(VideoFields &fields, const TikTokLayoutVideoConfig &config)
{
    fields.encoderParams = config.encoderParams;
    const int canvas = fields.canvas->findData(QString::fromStdString(config.canvasUuid));
    if (canvas >= 0) fields.canvas->setCurrentIndex(canvas);
    const int scene = fields.scene->findData(QString::fromStdString(config.sceneUuid));
    if (scene >= 0) fields.scene->setCurrentIndex(scene);
    const int encoder = fields.encoder->findData(QString::fromStdString(config.encoderId));
    if (encoder >= 0) fields.encoder->setCurrentIndex(encoder);
    fields.width->setValue(config.width);
    fields.height->setValue(config.height);
    fields.bitrate->setValue(config.bitrate);
    fields.fpsDivisor->setValue(config.fpsDivisor);
    fields.framing->setCurrentIndex(config.crop ? 1 : 0);
    LoadAudio(fields.audio, config.audio);
}

void TikTokDualLayoutWidget::SaveVideo(const VideoFields &fields, TikTokLayoutVideoConfig &config)
{
    config.sceneUuid = fields.scene->currentData().toString().toStdString();
    config.sceneName = (fields.scene->currentData(Qt::UserRole + 1).isValid()
        ? fields.scene->currentData(Qt::UserRole + 1).toString()
        : fields.scene->currentText()).toStdString();
    config.canvasUuid = fields.canvas->currentData().toString().toStdString();
    config.encoderId = fields.encoder->currentData().toString().toStdString();
    config.width = fields.width->value();
    config.height = fields.height->value();
    config.bitrate = fields.bitrate->value();
    config.fpsDivisor = fields.fpsDivisor->value();
    config.crop = fields.framing->currentData().toBool();
    config.encoderParams = fields.encoderParams;
    SaveAudio(fields.audio, config.audio);
}
