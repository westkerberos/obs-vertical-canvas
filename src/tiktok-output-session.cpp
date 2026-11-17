#include "tiktok-output-session.h"

#include "streamlabs-api.h"
#include "tiktok-native/request_cancellation.hpp"

#include <QCoreApplication>
#include <QEventLoop>
#include <QMetaObject>
#include <QTimer>
#include <obs-module.h>
#include <obs.hpp>
#include <graphics/vec2.h>
#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <utility>
#include <cstring>

namespace {
class TikTokLayoutScene {
    obs_canvas_t *canvas_ = nullptr;
    obs_scene_t *wrapper_ = nullptr;
    obs_source_t *source_ = nullptr;
public:
    ~TikTokLayoutScene()
    {
        if (canvas_)
            obs_canvas_set_channel(canvas_, 0, nullptr);
        if (wrapper_) {
            obs_canvas_scene_remove(wrapper_);
            obs_scene_release(wrapper_);
        }
        if (source_)
            obs_source_release(source_);
        if (canvas_) {
            if (!obs_canvas_removed(canvas_))
                obs_canvas_remove(canvas_);
            obs_canvas_release(canvas_);
        }
    }

    bool Create(const TikTokLayoutVideoConfig &config, const char *label, QString *error)
    {
        source_ = obs_get_source_by_uuid(config.sceneUuid.c_str());
        if (!source_ || !obs_source_is_scene(source_) || obs_source_removed(source_)) {
            *error = QString("The selected TikTok %1 scene no longer exists.").arg(QString::fromUtf8(label));
            return false;
        }

        obs_video_info info{};
        if (!obs_get_video_info(&info)) {
            *error = "OBS video is unavailable.";
            return false;
        }
        info.base_width = info.output_width = static_cast<uint32_t>(config.width);
        info.base_height = info.output_height = static_cast<uint32_t>(config.height);
        const QString canvasName = QString("TikTok %1").arg(QString::fromUtf8(label));
        canvas_ = obs_canvas_create_private(canvasName.toUtf8().constData(), &info, ACTIVATE | SCENE_REF | EPHEMERAL);
        if (!canvas_ || !obs_canvas_has_video(canvas_)) {
            *error = QString("Could not create the TikTok %1 canvas.").arg(QString::fromUtf8(label));
            return false;
        }
        wrapper_ = obs_canvas_scene_create(canvas_, QString("%1 composition").arg(canvasName).toUtf8().constData());
        if (!wrapper_) {
            *error = QString("Could not create the TikTok %1 scene.").arg(QString::fromUtf8(label));
            return false;
        }
        auto item = obs_scene_add(wrapper_, source_);
        if (!item) {
            *error = QString("Could not add the selected scene to the TikTok %1 layout.").arg(QString::fromUtf8(label));
            return false;
        }

        vec2 bounds;
        vec2_set(&bounds, static_cast<float>(config.width), static_cast<float>(config.height));
        obs_sceneitem_set_alignment(item, OBS_ALIGN_TOP | OBS_ALIGN_LEFT);
        obs_sceneitem_set_bounds_alignment(item, OBS_ALIGN_CENTER);
        obs_sceneitem_set_bounds_type(item, config.crop ? OBS_BOUNDS_SCALE_OUTER : OBS_BOUNDS_SCALE_INNER);
        obs_sceneitem_set_bounds_crop(item, config.crop);
        obs_sceneitem_set_bounds(item, &bounds);
        obs_canvas_set_channel(canvas_, 0, obs_scene_get_source(wrapper_));
        return true;
    }

    video_t *Video() const { return canvas_ ? obs_canvas_get_video(canvas_) : nullptr; }
};

static obs_data_t *EncoderSettings(const QJsonObject &params, int bitrate)
{
    OBSDataAutoRelease settings = obs_data_create();
    if (bitrate > 0)
        obs_data_set_int(settings, "bitrate", bitrate);
    const auto encoded = QJsonDocument(params).toJson(QJsonDocument::Compact);
    OBSDataAutoRelease supplied = obs_data_create_from_json(encoded.constData());
    if (supplied)
        obs_data_apply(settings, supplied);
    return obs_data_get_ref(settings);
}

static obs_encoder_t *CreateTikTokVideoEncoder(const TikTokLayoutVideoConfig &config,
    const char *name, video_t *video, QString *error)
{
    if (config.encoderId.empty()) {
        *error = "Choose a video encoder for each TikTok layout.";
        return nullptr;
    }
    OBSDataAutoRelease settings = EncoderSettings(config.encoderParams, config.bitrate);
    auto encoder = obs_video_encoder_create(config.encoderId.c_str(), name, settings, nullptr);
    if (!encoder) {
        *error = QString("Could not create the TikTok video encoder '%1'.")
            .arg(QString::fromStdString(config.encoderId));
        return nullptr;
    }
    obs_encoder_set_video(encoder, video);
    if (config.fpsDivisor > 1)
        obs_encoder_set_frame_rate_divisor(encoder, static_cast<uint32_t>(config.fpsDivisor));
    return encoder;
}

static obs_encoder_t *CreateTikTokAudioEncoder(const TikTokLayoutAudioConfig &config,
    const char *name, QString *error)
{
    if (config.encoderId.empty() || config.mixer < 0 || config.mixer >= MAX_AUDIO_MIXES ||
        config.bitrate < 32 || config.bitrate > 320) {
        *error = "TikTok audio encoder settings are invalid.";
        return nullptr;
    }
    OBSDataAutoRelease settings = EncoderSettings(config.encoderParams, config.bitrate);
    auto encoder = obs_audio_encoder_create(config.encoderId.c_str(), name, settings,
        static_cast<size_t>(config.mixer), nullptr);
    if (!encoder) {
        *error = QString("Could not create the TikTok audio encoder '%1'.")
            .arg(QString::fromStdString(config.encoderId));
        return nullptr;
    }
    obs_encoder_set_audio(encoder, obs_get_audio());
    return encoder;
}
}

struct TikTokOutputSession::DualFeedState {
    std::unique_ptr<TikTokLayoutScene> portraitScene;
    std::unique_ptr<TikTokLayoutScene> landscapeScene;
    obs_encoder_t *portraitVideo = nullptr;
    obs_encoder_t *landscapeVideo = nullptr;
    obs_encoder_t *sharedAudio = nullptr;
    obs_encoder_t *portraitAudio = nullptr;
    obs_encoder_t *landscapeAudio = nullptr;
    obs_output_t *primaryOutput = nullptr;
    obs_output_t *landscapeOutput = nullptr;
    obs_service_t *landscapeService = nullptr;
    std::atomic_bool stopping{false};
    bool stopConnected = false;

    ~DualFeedState()
    {
        stopping = true;
        if (landscapeOutput && stopConnected)
            signal_handler_disconnect(obs_output_get_signal_handler(landscapeOutput), "stop", LandscapeStopped, this);
        if (landscapeOutput && obs_output_active(landscapeOutput))
            obs_output_force_stop(landscapeOutput);
        if (landscapeOutput)
            obs_output_release(landscapeOutput);
        if (landscapeService)
            obs_service_release(landscapeService);
        for (auto encoder : {portraitVideo, landscapeVideo, sharedAudio, portraitAudio, landscapeAudio})
            if (encoder)
                obs_encoder_release(encoder);
    }

    static void LandscapeStopped(void *opaque, calldata_t *data)
    {
        auto state = static_cast<DualFeedState *>(opaque);
        if (!state || state->stopping || !state->primaryOutput)
            return;

        const int code = static_cast<int>(calldata_int(data, "code"));
        const char *detail = state->landscapeOutput ? obs_output_get_last_error(state->landscapeOutput) : nullptr;
        const QString message = detail && *detail ? QString::fromUtf8(detail)
            : QString("TikTok landscape feed stopped (OBS code %1).").arg(code);
        auto primary = obs_output_get_ref(state->primaryOutput);
        QMetaObject::invokeMethod(QCoreApplication::instance(), [primary, message] {
            if (!primary)
                return;
            obs_output_set_last_error(primary, message.toUtf8().constData());
            obs_output_force_stop(primary);
            obs_output_release(primary);
        }, Qt::QueuedConnection);
    }

    bool Create(const TikTokOutputConfig &config, obs_output_t *primary, QString *error)
    {
        primaryOutput = primary;
        portraitScene = std::make_unique<TikTokLayoutScene>();
        landscapeScene = std::make_unique<TikTokLayoutScene>();
        if (!portraitScene->Create(config.dualLayout.portrait, "portrait", error) ||
            !landscapeScene->Create(config.dualLayout.landscape, "landscape", error))
            return false;

        portraitVideo = CreateTikTokVideoEncoder(config.dualLayout.portrait,
            "tiktok_dual_portrait_video", portraitScene->Video(), error);
        if (!portraitVideo)
            return false;
        landscapeVideo = CreateTikTokVideoEncoder(config.dualLayout.landscape,
            "tiktok_dual_landscape_video", landscapeScene->Video(), error);
        if (!landscapeVideo)
            return false;

        if (config.dualLayout.sameAudio) {
            sharedAudio = CreateTikTokAudioEncoder(config.dualLayout.sharedAudio, "tiktok_dual_shared_audio", error);
            if (!sharedAudio)
                return false;
            portraitAudio = sharedAudio;
            landscapeAudio = sharedAudio;
            obs_encoder_addref(sharedAudio);
        } else {
            portraitAudio = CreateTikTokAudioEncoder(config.dualLayout.portrait.audio, "tiktok_dual_portrait_audio", error);
            if (!portraitAudio)
                return false;
            landscapeAudio = CreateTikTokAudioEncoder(config.dualLayout.landscape.audio, "tiktok_dual_landscape_audio", error);
            if (!landscapeAudio)
                return false;
        }

        auto oldVideo = obs_output_get_video_encoder(primaryOutput);
        obs_output_set_video_encoder(primaryOutput, portraitVideo);
        if (oldVideo)
            obs_encoder_release(oldVideo);
        auto oldAudio = obs_output_get_audio_encoder(primaryOutput, 0);
        obs_output_set_audio_encoder(primaryOutput, portraitAudio, 0);
        if (oldAudio)
            obs_encoder_release(oldAudio);

        OBSDataAutoRelease outputSettings = obs_data_create();
        landscapeOutput = obs_output_create("rtmp_output", "TikTok landscape feed", outputSettings, nullptr);
        landscapeService = obs_service_create("rtmp_custom", "TikTok landscape feed service", nullptr, nullptr);
        if (!landscapeOutput || !landscapeService) {
            *error = "Could not create the TikTok landscape transport.";
            return false;
        }
        obs_output_set_video_encoder(landscapeOutput, landscapeVideo);
        obs_output_set_audio_encoder(landscapeOutput, landscapeAudio, 0);
        obs_output_set_service(landscapeOutput, landscapeService);
        obs_output_set_reconnect_settings(landscapeOutput, 5, 2);
        signal_handler_connect(obs_output_get_signal_handler(landscapeOutput), "stop", LandscapeStopped, this);
        stopConnected = true;
        return true;
    }

    bool StartLandscape(const std::string &server, const std::string &key, QString *error)
    {
        if (!landscapeOutput || server.empty() || key.empty()) {
            *error = "TikTok did not return a valid landscape RTMP endpoint.";
            return false;
        }
        OBSDataAutoRelease settings = obs_service_get_settings(landscapeService);
        obs_data_set_string(settings, "server", server.c_str());
        obs_data_set_string(settings, "key", key.c_str());
        obs_data_set_string(settings, "bearer_token", key.c_str());
        obs_service_update(landscapeService, settings);
        if (!obs_output_start(landscapeOutput)) {
            const char *last = obs_output_get_last_error(landscapeOutput);
            *error = last && *last ? QString::fromUtf8(last) : QString("Could not start the TikTok landscape feed.");
            return false;
        }
        return true;
    }

    void Stop()
    {
        stopping = true;
        if (landscapeOutput && stopConnected) {
            signal_handler_disconnect(obs_output_get_signal_handler(landscapeOutput), "stop", LandscapeStopped, this);
            stopConnected = false;
        }
        if (landscapeOutput && obs_output_active(landscapeOutput))
            obs_output_force_stop(landscapeOutput);
    }
};

TikTokOutputSession::~TikTokOutputSession()
{
    Stop();
}

bool TikTokOutputSession::Start(const TikTokOutputConfig &config, obs_output *output,
    std::function<void(const QJsonObject &)> saveDevices)
{
    error_.clear();
    server_.clear();
    key_.clear();

    if (config.method == "streamlabs") {
        if (config.streamlabsAccessToken.empty()) {
            error_ = "Enter a Streamlabs access token.";
            return false;
        }

        streamlabsAccessToken_ = config.streamlabsAccessToken;
        auto result = std::async(std::launch::async, [&config] {
            const std::atomic_bool cancelled{false};
            TikTokRequestCancellation cancellation(&cancelled);
            const auto title = config.title.empty() ? std::string("TikTok LIVE") : config.title;
            const auto category = StreamlabsAPI::CategorySearch(config.streamlabsAccessToken, config.streamlabsCategory);
            return StreamlabsAPI::StartStream(config.streamlabsAccessToken, title, category,
                config.streamlabsMatureContent ? 1 : 0);
        });

        QEventLoop loop;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (result.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                loop.quit();
        });
        poll.start(50);
        loop.exec();

        try {
            auto [success, message, server, key] = result.get();
            if (!success) {
                error_ = QString::fromStdString(message);
                streamlabsAccessToken_.clear();
                return false;
            }
            server_ = std::move(server);
            key_ = std::move(key);
            streamlabsId_ = StreamlabsAPI::ExtractStreamId(key_);
            streamlabsStarted_ = true;
            if (server_.empty() || key_.empty()) {
                error_ = "Streamlabs returned an empty RTMP endpoint or stream key.";
                Stop();
                return false;
            }
            return true;
        } catch (const std::exception &exception) {
            error_ = QString("Streamlabs connection failed: %1").arg(exception.what());
        } catch (...) {
            error_ = "Streamlabs connection failed.";
        }
        streamlabsAccessToken_.clear();
        return false;
    }

    if (config.method != "live-studio" && config.method != "live-studio-mobile") {
        error_ = "Unsupported TikTok connection method.";
        return false;
    }

    TikTokOutputConfig studioConfig = config;
    studioConfig.client = config.method == "live-studio-mobile" ? "mobile" : "pc";
    liveStudio_ = std::make_unique<TikTokLiveSession>();

    if (studioConfig.UsesDualLayout()) {
        if (studioConfig.dualLayout.portrait.sceneUuid.empty() ||
            studioConfig.dualLayout.landscape.sceneUuid.empty() ||
            studioConfig.dualLayout.portrait.encoderId.empty() ||
            studioConfig.dualLayout.landscape.encoderId.empty() ||
            studioConfig.dualLayout.portrait.width < 64 || studioConfig.dualLayout.portrait.height < 64 ||
            studioConfig.dualLayout.landscape.width < 64 || studioConfig.dualLayout.landscape.height < 64 ||
            studioConfig.dualLayout.portrait.width >= studioConfig.dualLayout.portrait.height ||
            studioConfig.dualLayout.landscape.width <= studioConfig.dualLayout.landscape.height) {
            error_ = "Configure a valid portrait and landscape scene, resolution and video encoder.";
            liveStudio_.reset();
            return false;
        }
        dual_ = std::make_unique<DualFeedState>();
        QString layoutError;
        if (!dual_->Create(studioConfig, output, &layoutError)) {
            error_ = layoutError;
            dual_.reset();
            liveStudio_.reset();
            return false;
        }
        const std::vector<obs_output_t *> feeds{output, dual_->landscapeOutput};
        if (!liveStudio_->Start(studioConfig, feeds, std::move(saveDevices))) {
            error_ = liveStudio_->Error();
            Stop();
            return false;
        }
        server_ = liveStudio_->Server(false);
        key_ = liveStudio_->Key(false);
        if (server_.empty() || key_.empty() ||
            !dual_->StartLandscape(liveStudio_->Server(true), liveStudio_->Key(true), &layoutError)) {
            error_ = !layoutError.isEmpty() ? layoutError : QString("TikTok did not return both RTMP feed endpoints.");
            Stop();
            return false;
        }
    } else {
        if (!liveStudio_->Start(studioConfig, output, std::move(saveDevices))) {
            error_ = liveStudio_->Error();
            liveStudio_.reset();
            return false;
        }
        server_ = liveStudio_->Server();
        key_ = liveStudio_->Key();
        if (server_.empty() || key_.empty()) {
            error_ = "TikTok did not return a usable RTMP endpoint.";
            Stop();
            return false;
        }
    }
    return true;
}

void TikTokOutputSession::ConfirmStarted()
{
    if (liveStudio_)
        liveStudio_->ConfirmStarted();
}

void TikTokOutputSession::CancelPreparation()
{
    if (liveStudio_)
        liveStudio_->CancelPreparation();
}

void TikTokOutputSession::Stop()
{
    if (dual_)
        dual_->Stop();
    if (liveStudio_) {
        liveStudio_->Stop();
        const auto nativeError = liveStudio_->Error();
        liveStudio_.reset();
        if (error_.isEmpty() && !nativeError.isEmpty())
            error_ = nativeError;
    }
    dual_.reset();
    if (streamlabsStarted_) {
        const auto token = streamlabsAccessToken_;
        const auto id = streamlabsId_;
        streamlabsStarted_ = false;
        auto result = std::async(std::launch::async, [token, id] {
            return StreamlabsAPI::EndStream(token, id);
        });
        QEventLoop loop;
        QTimer poll;
        QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
            if (result.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
                loop.quit();
        });
        poll.start(50);
        loop.exec();
        try {
            if (!result.get() && error_.isEmpty())
                error_ = "Streamlabs could not confirm that the TikTok LIVE was ended.";
        } catch (...) {
            if (error_.isEmpty())
                error_ = "Could not confirm that Streamlabs ended the TikTok LIVE.";
        }
    }
    streamlabsAccessToken_.clear();
    streamlabsId_.clear();
    server_.clear();
    key_.clear();
}

QString TikTokOutputSession::Error() const
{
    auto result = error_;
    if (!streamlabsAccessToken_.empty())
        result.replace(QString::fromStdString(streamlabsAccessToken_), "<redacted>");
    if (liveStudio_) {
        const auto liveError = liveStudio_->Error();
        if (!liveError.isEmpty())
            result = liveError;
    }
    return result;
}

std::string TikTokOutputSession::Server() const
{
    return server_;
}

std::string TikTokOutputSession::Key() const
{
    return key_;
}
