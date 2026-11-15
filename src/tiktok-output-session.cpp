#include "tiktok-output-session.h"

#include "streamlabs-api.h"
#include "tiktok-native/request_cancellation.hpp"

#include <QEventLoop>
#include <QTimer>
#include <obs-module.h>
#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <utility>

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
    if (liveStudio_) {
        const auto nativeError = liveStudio_->Error();
        liveStudio_->Stop();
        liveStudio_.reset();
        if (error_.isEmpty() && !nativeError.isEmpty())
            error_ = nativeError;
    }
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
