#include "tiktok-output-settings-dialog.h"
#include "tiktok-safety.h"
#include "account-info-widget.h"
#include "tiktok-dual-layout-widget.h"

#include "streamlabs-desktop.h"
#include "streamlabs-category-widget.h"
#include "tiktok-topic-widget.h"
#include "tiktok-native/live_client.hpp"
#include "tiktok-native/qr_login.hpp"
#include "tiktok-native/tiktok_studio_topics.hpp"
#include "tiktok-native/request_cancellation.hpp"
#include "tiktok-native/hosted_signing_service.hpp"
#include "tiktok-native/rapidapi_quota.hpp"

#include <obs-module.h>
#include <util/bmem.h>

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressDialog>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QProgressBar>
#include <QDateTime>
#include <QLocale>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <future>
#include <utility>

namespace {
class RapidApiQuotaIndicator final : public QWidget {
    HostedSigningServiceConfig service_;
    QProgressBar *bar_ = nullptr;
    QTimer timer_;

    void Refresh()
    {
        const auto quota = ReadRapidApiQuota(service_);
        bar_->setRange(0, 1000);
        if (quota.limit > 0 && quota.remaining >= 0) {
            const auto remaining = std::clamp(quota.remaining, qint64(0), quota.limit);
            const double fraction = double(remaining) / double(quota.limit);
            bar_->setValue(static_cast<int>(fraction * 1000.0));
            bar_->setFormat(QString("%1 of %2 requests remaining")
                .arg(QLocale().toString(remaining), QLocale().toString(quota.limit)));
        } else if (quota.remaining >= 0) {
            bar_->setValue(quota.remaining > 0 ? 1000 : 0);
            bar_->setFormat(QString("%1 requests remaining").arg(QLocale().toString(quota.remaining)));
        } else {
            bar_->setValue(0);
            bar_->setFormat("Usage appears here after the first TikTok LIVE signing request");
        }

        QStringList tips;
        if (quota.observed_epoch_seconds > 0)
            tips << QString("Last updated: %1").arg(QLocale().toString(
                QDateTime::fromSecsSinceEpoch(quota.observed_epoch_seconds).toLocalTime(), QLocale::ShortFormat));
        if (quota.has_calendar_reset())
            tips << QString("Quota resets: %1").arg(QLocale().toString(
                QDateTime::fromSecsSinceEpoch(quota.reset_epoch_seconds).toLocalTime(), QLocale::ShortFormat));
        if (tips.isEmpty())
            tips << "Quota is read passively from RapidAPI response headers; this indicator does not send requests.";
        bar_->setToolTip(tips.join('\n'));
    }

public:
    explicit RapidApiQuotaIndicator(QWidget *parent = nullptr) : QWidget(parent)
    {
        auto *layout = new QFormLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
        bar_ = new QProgressBar(this);
        bar_->setObjectName("rapidApiQuotaBar");
        bar_->setMinimumHeight(22);
        bar_->setTextVisible(true);
        layout->addRow("RapidAPI quota", bar_);
        connect(&timer_, &QTimer::timeout, this, [this] {
            if (isVisible())
                Refresh();
        });
        timer_.start(1000);
        Refresh();
    }

    void SetApiKey(const QString &key)
    {
        service_.api_key = key.trimmed();
        Refresh();
    }
};

class TikTokQrLoginDialog final : public QDialog {
    QLabel *picture_ = nullptr;
    QLabel *status_ = nullptr;
    QPushButton *retry_ = nullptr;
    QDialogButtonBox *buttons_ = nullptr;
    QTimer watcher_, poll_;
    std::future<TikTokQrLogin> work_;
    std::atomic_bool cancelled_{false};
    bool closing_ = false;
    int pollFailures_ = 0;
    QString cookieDirectory_;

    void Start(bool fresh)
    {
        if (work_.valid() || closing_)
            return;
        poll_.stop();
        retry_->hide();
        if (fresh) {
            pollFailures_ = 0;
            picture_->clear();
            picture_->setText("Preparing a new QR code…");
            status_->clear();
        }

        auto input = login;
        work_ = std::async(std::launch::async, [this, input, fresh]() mutable {
            TikTokRequestCancellation scope(&cancelled_);
            try {
                auto result = fresh ? begin_tiktok_qr_login(input.account) : poll_tiktok_qr_login(std::move(input));
                if (result.error.isEmpty() && result.state == TikTokQrState::Confirmed &&
                    result.account.username.trimmed().isEmpty() && !cancelled_) {
                    try {
                        QString error;
                        result.account.username = lookup_tiktok_username(result.account, &error);
                    } catch (...) {}
                }
                return result;
            } catch (const std::exception &exception) {
                input.error = QString::fromUtf8(exception.what());
                input.retryable = false;
                return input;
            } catch (...) {
                input.error = "TikTok QR login failed.";
                input.retryable = false;
                return input;
            }
        });
    }

    void Failed(const QString &error)
    {
        status_->setText(error);
        picture_->clear();
        retry_->show();
    }

    void Receive()
    {
        if (!work_.valid() || work_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
            return;
        login = work_.get();
        if (closing_) {
            QDialog::reject();
            return;
        }
        if (!login.error.isEmpty()) {
            if (login.retryable && !login.token.isEmpty() && ++pollFailures_ <= 6) {
                const int delay = pollFailures_ == 1 ? 5000 : pollFailures_ == 2 ? 10000 :
                    pollFailures_ == 3 ? 20000 : 30000;
                status_->setText(QString("TikTok is still processing the login. Retrying in %1 seconds…").arg(delay / 1000));
                retry_->show();
                poll_.start(delay);
                return;
            }
            if (login.retryable && !login.token.isEmpty()) {
                Start(true);
                return;
            }
            Failed(login.error);
            return;
        }

        pollFailures_ = 0;
        if (login.state == TikTokQrState::Confirmed) {
            QString error;
            cookiePath = save_tiktok_login_cookies(login.account.cookie_jar, cookieDirectory_, &error,
                login.account.username.isEmpty() ? login.account.user_id : login.account.username);
            if (cookiePath.isEmpty()) {
                Failed(error);
                return;
            }
            accept();
            return;
        }
        if (login.state == TikTokQrState::Expired || login.state == TikTokQrState::Refused ||
            login.state == TikTokQrState::Error) {
            Start(true);
            return;
        }

        QPixmap image;
        if (!image.loadFromData(login.png, "PNG")) {
            Failed("TikTok returned an invalid QR code.");
            return;
        }
        picture_->setPixmap(image.scaled(280, 280, Qt::KeepAspectRatio, Qt::FastTransformation));
        status_->setText(login.state == TikTokQrState::Scanned
            ? "QR scanned. Confirm the login on your phone."
            : "Scan this QR code with the TikTok app.");
        poll_.start(2500);
    }

public:
    TikTokQrLoginDialog(TikTokStudioAccountCredentials account, QString directory, QWidget *parent)
        : QDialog(parent), cookieDirectory_(std::move(directory))
    {
        setWindowTitle("TikTok LIVE Studio login");
        setModal(true);
        resize(420, 470);

        auto *layout = new QVBoxLayout(this);
        picture_ = new QLabel(this);
        picture_->setAlignment(Qt::AlignCenter);
        picture_->setMinimumSize(280, 280);
        picture_->setText("Preparing a QR code…");
        layout->addWidget(picture_);

        status_ = new QLabel(this);
        status_->setWordWrap(true);
        layout->addWidget(status_);

        retry_ = new QPushButton("Generate another QR code", this);
        retry_->hide();
        layout->addWidget(retry_);

        buttons_ = new QDialogButtonBox(QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons_);

        login.account = std::move(account);
        connect(buttons_, &QDialogButtonBox::rejected, this, &TikTokQrLoginDialog::reject);
        connect(retry_, &QPushButton::clicked, this, [this] { Start(true); });
        connect(&watcher_, &QTimer::timeout, this, [this] { Receive(); });
        watcher_.start(50);
        connect(&poll_, &QTimer::timeout, this, [this] { Start(false); });
        poll_.setSingleShot(true);
        QTimer::singleShot(0, this, [this] { Start(true); });
    }

    ~TikTokQrLoginDialog() override
    {
        cancelled_ = true;
        if (work_.valid())
            work_.wait();
    }

    TikTokQrLogin login;
    QString cookiePath;

    void reject() override
    {
        closing_ = true;
        cancelled_ = true;
        poll_.stop();
        picture_->clear();
        retry_->hide();
        if (work_.valid()) {
            status_->setText("Cancelling login…");
            buttons_->setEnabled(false);
        } else {
            QDialog::reject();
        }
    }
};

QString DetectStreamlabsToken(QWidget *parent)
{
    std::atomic_bool cancelled{false};
    auto work = std::async(std::launch::async, [&] {
        return FindStreamlabsDesktopToken(StreamlabsDesktopDirectory(), cancelled);
    });

    QProgressDialog progress("Looking for a Streamlabs Desktop login…", "Cancel", 0, 0, parent);
    progress.setWindowTitle("Streamlabs Desktop");
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(0);
    QTimer poll;
    QObject::connect(&poll, &QTimer::timeout, &progress, [&] {
        if (work.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            progress.accept();
    });
    QObject::connect(&progress, &QProgressDialog::canceled, [&] {
        cancelled = true;
    });
    poll.start(50);
    progress.exec();
    cancelled = true;
    try {
        return work.get();
    } catch (...) {
        return {};
    }
}

bool LoginTikTokWithQr(QWidget *parent, const QString &rapidApiKey,
    QString *cookiePath, QJsonObject *deviceProfiles)
{
    if (rapidApiKey.trimmed().isEmpty()) {
        QMessageBox::information(parent, "TikTok login", "Enter your RapidAPI key before logging in.");
        return false;
    }

    TikTokStudioAccountCredentials account;
    account.rapidapi_key = rapidApiKey;
    if (deviceProfiles) {
        account.device_profile = deviceProfiles->value("pc").toObject();
        account.device_id = account.device_profile.value("device_id").toString();
        account.install_id = account.device_profile.value("install_id").toString();
        account.live_studio_version = account.device_profile.value("live_studio_version").toString("1.27.0");
    }

    char *path = obs_module_config_path("cookies");
    const QString directory = path ? QString::fromUtf8(path) : QString();
    bfree(path);
    if (directory.isEmpty()) {
        QMessageBox::warning(parent, "TikTok login", "Could not resolve the OBS plugin configuration directory.");
        return false;
    }

    TikTokQrLoginDialog dialog(account, directory, parent);
    if (dialog.exec() != QDialog::Accepted)
        return false;

    if (cookiePath)
        *cookiePath = dialog.cookiePath;
    if (deviceProfiles) {
        QJsonObject profile = deviceProfiles->value("pc").toObject();
        profile.insert("device_id", dialog.login.account.device_id);
        profile.insert("install_id", dialog.login.account.install_id);
        profile.insert("live_studio_version", dialog.login.account.live_studio_version);
        deviceProfiles->insert("pc", profile);
    }
    return true;
}
static QJsonObject DualAudioToJson(const TikTokLayoutAudioConfig &audio)
{
    return {
        {"encoder_id", QString::fromStdString(audio.encoderId)},
        {"mixer", audio.mixer},
        {"bitrate", audio.bitrate},
        {"encoder_params", audio.encoderParams}
    };
}

static QJsonObject DualVideoToJson(const TikTokLayoutVideoConfig &video)
{
    return {
        {"scene_uuid", QString::fromStdString(video.sceneUuid)},
        {"scene_name", QString::fromStdString(video.sceneName)},
        {"encoder_id", QString::fromStdString(video.encoderId)},
        {"width", video.width},
        {"height", video.height},
        {"bitrate", video.bitrate},
        {"crop", video.crop},
        {"fps_divisor", video.fpsDivisor},
        {"encoder_params", video.encoderParams},
        {"canvas_uuid", QString::fromStdString(video.canvasUuid)},
        {"audio", DualAudioToJson(video.audio)}
    };
}

static QJsonObject DualLayoutToJson(const TikTokDualLayoutConfig &config)
{
    return {
        {"enabled", config.enabled},
        {"same_audio", config.sameAudio},
        {"independent_audio_initialized", config.independentAudioInitialized},
        {"shared_audio", DualAudioToJson(config.sharedAudio)},
        {"portrait", DualVideoToJson(config.portrait)},
        {"landscape", DualVideoToJson(config.landscape)}
    };
}

static TikTokDualLayoutConfig DualLayoutFromJson(const QJsonObject &json)
{
    TikTokDualLayoutConfig config;
    config.enabled = json.value("enabled").toBool(false);
    config.sameAudio = json.value("same_audio").toBool(true);
    config.independentAudioInitialized = json.value("independent_audio_initialized").toBool(false);
    const auto readAudio = [](const QJsonObject &value, TikTokLayoutAudioConfig &audio) {
        audio.encoderId = value.value("encoder_id").toString("ffmpeg_aac").toStdString();
        audio.mixer = value.value("mixer").toInt(0);
        audio.bitrate = value.value("bitrate").toInt(160);
        audio.encoderParams = value.value("encoder_params").toObject();
    };
    const auto readVideo = [&](const QJsonObject &value, TikTokLayoutVideoConfig &video) {
        video.sceneUuid = value.value("scene_uuid").toString().toStdString();
        video.sceneName = value.value("scene_name").toString().toStdString();
        video.encoderId = value.value("encoder_id").toString().toStdString();
        video.width = value.value("width").toInt(video.width);
        video.height = value.value("height").toInt(video.height);
        video.bitrate = value.value("bitrate").toInt(video.bitrate);
        video.crop = value.value("crop").toBool(false);
        video.fpsDivisor = value.value("fps_divisor").toInt(1);
        video.encoderParams = value.value("encoder_params").toObject();
        video.canvasUuid = value.value("canvas_uuid").toString().toStdString();
        readAudio(value.value("audio").toObject(), video.audio);
    };
    readAudio(json.value("shared_audio").toObject(), config.sharedAudio);
    readVideo(json.value("portrait").toObject(), config.portrait);
    readVideo(json.value("landscape").toObject(), config.landscape);
    return config;
}

} // namespace

bool ConfigureTikTokOutput(QWidget *parent, obs_data_t *settings, const QString &outputName)
{
    if (!settings)
        return false;

    QDialog dialog(parent);
    dialog.setWindowTitle(QString("Configure %1").arg(outputName.isEmpty() ? "TikTok" : outputName));
    dialog.setMinimumSize(560, 620);
    auto *root = new QVBoxLayout(&dialog);
    auto *scroll = new QScrollArea(&dialog);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto *content = new QWidget(scroll);
    auto *contentLayout = new QVBoxLayout(content);
    contentLayout->setAlignment(Qt::AlignTop);
    auto *form = new QFormLayout;
    contentLayout->addLayout(form);
    scroll->setWidget(content);
    root->addWidget(scroll, 1);

    auto *method = new QComboBox(&dialog);
    method->addItem("Custom RTMP / stream key", "custom");
    method->addItem("TikTok LIVE Studio — Desktop", "live-studio");
    method->addItem("TikTok LIVE Studio — Mobile", "live-studio-mobile");
    method->addItem("Streamlabs Desktop", "streamlabs");
    const QString savedMethod = QString::fromUtf8(obs_data_get_string(settings, "tiktok_method"));
    int methodIndex = method->findData(savedMethod.isEmpty() ? "custom" : savedMethod);
    method->setCurrentIndex(methodIndex < 0 ? 0 : methodIndex);
    form->addRow("Connection method", method);

    auto *title = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "tiktok_title")), &dialog);
    form->addRow("LIVE title", title);

    auto *studioGroup = new QWidget(&dialog);
    auto *studioForm = new QFormLayout(studioGroup);
    auto *rapidApiKey = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "tiktok_rapidapi_key")), studioGroup);
    rapidApiKey->setEchoMode(QLineEdit::Password);
    studioForm->addRow("RapidAPI key", rapidApiKey);
    auto *quota = new RapidApiQuotaIndicator(studioGroup);
    studioForm->addRow(quota);
    QObject::connect(rapidApiKey, &QLineEdit::textChanged, quota, &RapidApiQuotaIndicator::SetApiKey);

    auto *cookieRow = new QWidget(studioGroup);
    auto *cookieLayout = new QHBoxLayout(cookieRow);
    cookieLayout->setContentsMargins(0, 0, 0, 0);
    auto *cookiePath = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "tiktok_cookie_path")), cookieRow);
    auto *browseCookie = new QPushButton("Browse…", cookieRow);
    auto *qrLogin = new QPushButton("Login with TikTok QR", cookieRow);
    cookieLayout->addWidget(cookiePath, 1);
    cookieLayout->addWidget(browseCookie);
    cookieLayout->addWidget(qrLogin);
    studioForm->addRow("TikTok cookies", cookieRow);

    auto *topics = new TikTokTopicWidget(LookupTikTokGames, studioGroup);
    topics->SetSelection(QString::fromUtf8(obs_data_get_string(settings, "tiktok_topic_id")),
        QString::fromUtf8(obs_data_get_string(settings, "tiktok_game_id")),
        QString::fromUtf8(obs_data_get_string(settings, "tiktok_game_name")));
    studioForm->addRow(topics);
    auto *allowGifts = new QCheckBox("Allow gifts", studioGroup);
    auto *allowComments = new QCheckBox("Allow comments", studioGroup);
    allowGifts->setChecked(obs_data_get_bool(settings, "tiktok_allow_gifts") || !obs_data_has_user_value(settings, "tiktok_allow_gifts"));
    allowComments->setChecked(obs_data_get_bool(settings, "tiktok_allow_comments") || !obs_data_has_user_value(settings, "tiktok_allow_comments"));
    studioForm->addRow(allowGifts);
    studioForm->addRow(allowComments);
    auto *disclosure = new QComboBox(studioGroup);
    disclosure->addItem("No disclosure", "off");
    disclosure->addItem("Own brand", "own_brand");
    disclosure->addItem("Third-party brand", "third_party");
    disclosure->addItem("Own + third-party", "both");
    const QString savedDisclosure = QString::fromUtf8(obs_data_get_string(settings, "tiktok_content_disclosure"));
    const int disclosureIndex = disclosure->findData(savedDisclosure.isEmpty() ? "off" : savedDisclosure);
    disclosure->setCurrentIndex(disclosureIndex < 0 ? 0 : disclosureIndex);
    studioForm->addRow("Content disclosure", disclosure);
    auto *monitorViolations = new QCheckBox("Monitor LIVE violations and notify me", studioGroup);
    monitorViolations->setObjectName("monitorTikTokViolations");
    monitorViolations->setChecked(!obs_data_has_user_value(settings, "tiktok_monitor_violations") ||
        obs_data_get_bool(settings, "tiktok_monitor_violations"));
    studioForm->addRow(monitorViolations);
    auto *checkSafety = new QPushButton("Check LIVE violations and appeals", studioGroup);
    checkSafety->setObjectName("checkTikTokViolations");
    checkSafety->setAutoDefault(false);
    studioForm->addRow(checkSafety);
    auto *studioMature = new QCheckBox("Mature content", studioGroup);
    studioMature->setChecked(obs_data_get_bool(settings, "tiktok_mature_content"));
    studioForm->addRow(studioMature);
    form->addRow(studioGroup);

    QJsonObject profiles = QJsonDocument::fromJson(
        QByteArray(obs_data_get_string(settings, "tiktok_device_profiles"))).object();
    QJsonObject mobileProfile = profiles.value("mobile").toObject();
    QObject::connect(checkSafety, &QPushButton::clicked, &dialog, [&] {
        TikTokOutputConfig config;
        config.method = method->currentData().toString().toStdString();
        config.title = title->text().toStdString();
        config.rapidApiKey = rapidApiKey->text().trimmed().toStdString();
        config.cookieFilePath = cookiePath->text().trimmed().toStdString();
        config.client = config.method == "live-studio-mobile" ? "mobile" : "pc";
        config.deviceProfiles = profiles;
        OpenTikTokSafety(config, &dialog);
    });
    auto *mobileGroup = new QWidget(&dialog);
    auto *mobileForm = new QFormLayout(mobileGroup);
    auto *mobileBroadcast = new QComboBox(mobileGroup);
    mobileBroadcast->addItem("Camera LIVE", "video");
    mobileBroadcast->addItem("Screen capture", "screen_record");
    const QString savedBroadcast = mobileProfile.value("broadcast_type").toString("video");
    mobileBroadcast->setCurrentIndex(mobileBroadcast->findData(savedBroadcast) < 0 ? 0 : mobileBroadcast->findData(savedBroadcast));
    mobileForm->addRow("Mobile broadcast type", mobileBroadcast);
    auto *mobileOrientation = new QComboBox(mobileGroup);
    mobileOrientation->addItem("Portrait", "1");
    mobileOrientation->addItem("Landscape", "2");
    const QString savedOrientation = mobileProfile.value("orientation").toString("1");
    mobileOrientation->setCurrentIndex(mobileOrientation->findData(savedOrientation) < 0 ? 0 : mobileOrientation->findData(savedOrientation));
    mobileForm->addRow("Screen-capture orientation", mobileOrientation);
    auto *mobileDeviceId = new QLineEdit(mobileProfile.value("device_id").toString(), mobileGroup);
    auto *mobileInstallId = new QLineEdit(mobileProfile.value("install_id").toString(), mobileGroup);
    mobileDeviceId->setPlaceholderText("Leave blank to register automatically");
    mobileInstallId->setPlaceholderText("Leave blank to register automatically");
    mobileForm->addRow("Android device ID", mobileDeviceId);
    mobileForm->addRow("Android install ID", mobileInstallId);
    form->addRow(mobileGroup);

    auto *streamlabsGroup = new QWidget(&dialog);
    auto *streamlabsForm = new QFormLayout(streamlabsGroup);
    auto *streamlabsToken = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "streamlabs_access_token")), streamlabsGroup);
    streamlabsToken->setEchoMode(QLineEdit::Password);
    auto *detectToken = new QPushButton("Detect Streamlabs Desktop login", streamlabsGroup);
    auto *tokenRow = new QWidget(streamlabsGroup);
    auto *tokenLayout = new QHBoxLayout(tokenRow);
    tokenLayout->setContentsMargins(0, 0, 0, 0);
    tokenLayout->addWidget(streamlabsToken, 1);
    tokenLayout->addWidget(detectToken);
    streamlabsForm->addRow("Streamlabs access token", tokenRow);
    auto *category = new StreamlabsCategoryWidget(SearchStreamlabsCategories, streamlabsGroup);
    std::optional<std::string> savedCategoryId;
    if (obs_data_has_user_value(settings, "streamlabs_category_id"))
        savedCategoryId = obs_data_get_string(settings, "streamlabs_category_id");
    category->SetSelection(QString::fromUtf8(obs_data_get_string(settings, "streamlabs_category")), savedCategoryId);
    streamlabsForm->addRow("Category", category);
    auto *streamlabsMature = new QCheckBox("Mature content", streamlabsGroup);
    streamlabsMature->setChecked(obs_data_get_bool(settings, "streamlabs_mature_content"));
    streamlabsForm->addRow(streamlabsMature);
    form->addRow(streamlabsGroup);

    const char *savedDualText = obs_data_get_string(settings, "tiktok_dual_layout");
    const auto savedDualDoc = QJsonDocument::fromJson(QByteArray(savedDualText ? savedDualText : ""));
    auto *dualLayout = new TikTokDualLayoutWidget(content);
    dualLayout->Load(savedDualDoc.isObject() ? DualLayoutFromJson(savedDualDoc.object()) : TikTokDualLayoutConfig{});
    form->addRow(dualLayout);

    auto *accountInfo = new AccountInfoWidget(LookupAccountInfo, &dialog);
    accountInfo->setObjectName("tiktokAccountInfo");
    form->addRow(accountInfo);
    std::optional<bool> dualAccess;
    accountInfo->SetDualAccessChangedCallback([&](std::optional<bool> available) {
        dualAccess = available;
        const auto selected = method->currentData().toString();
        const bool cameraLive = selected != "live-studio-mobile" ||
            mobileBroadcast->currentData().toString() == "video";
        dualLayout->SetAccess(available, cameraLive);
    });
    accountInfo->SetDeviceProfileChangedCallback([&](const std::string &client, const QJsonObject &profile) {
        profiles.insert(QString::fromStdString(client), profile);
        if (client == "mobile") {
            mobileProfile = profile;
            const QSignalBlocker deviceBlock(mobileDeviceId);
            const QSignalBlocker installBlock(mobileInstallId);
            mobileDeviceId->setText(profile.value("device_id").toString());
            mobileInstallId->setText(profile.value("install_id").toString());
        }
    });

    auto updateAccountInfo = [&] {
        TikTokOutputConfig config;
        config.method = method->currentData().toString().toStdString();
        config.client = config.method == "live-studio-mobile" ? "mobile" : "pc";
        config.rapidApiKey = rapidApiKey->text().trimmed().toStdString();
        config.cookieFilePath = cookiePath->text().trimmed().toStdString();
        config.streamlabsAccessToken = streamlabsToken->text().trimmed().toStdString();
        config.deviceProfiles = profiles;
        if (config.client == "mobile") {
            QJsonObject profile = profiles.value("mobile").toObject();
            profile.insert("broadcast_type", mobileBroadcast->currentData().toString());
            profile.insert("orientation", mobileOrientation->currentData().toString());
            profile.insert("device_id", mobileDeviceId->text().trimmed());
            profile.insert("install_id", mobileInstallId->text().trimmed());
            config.deviceProfiles.insert("mobile", profile);
        }
        accountInfo->SetConfig(config);
        dualLayout->SetMethod(QString::fromStdString(config.method));
        const bool cameraLive = config.method != "live-studio-mobile" ||
            mobileBroadcast->currentData().toString() == "video";
        dualLayout->SetAccess(dualAccess, cameraLive);
    };

    auto updateVisibility = [&] {
        const auto selected = method->currentData().toString();
        const bool studioActive = selected == "live-studio" || selected == "live-studio-mobile";
        const bool mobileCamera = selected == "live-studio-mobile" && mobileBroadcast->currentData().toString() == "video";
        studioGroup->setVisible(studioActive);
        mobileGroup->setVisible(selected == "live-studio-mobile");
        streamlabsGroup->setVisible(selected == "streamlabs");
        topics->SetContext(studioActive, cookiePath->text(), mobileCamera);
        category->SetContext(selected == "streamlabs", streamlabsToken->text().trimmed().toStdString());
        updateAccountInfo();
    };
    QObject::connect(method, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateVisibility);
    QObject::connect(cookiePath, &QLineEdit::textChanged, &dialog, updateVisibility);
    QObject::connect(mobileBroadcast, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateVisibility);
    QObject::connect(streamlabsToken, &QLineEdit::textChanged, &dialog, updateVisibility);
    QObject::connect(rapidApiKey, &QLineEdit::textChanged, &dialog, updateVisibility);
    QObject::connect(mobileDeviceId, &QLineEdit::textChanged, &dialog, updateVisibility);
    QObject::connect(mobileInstallId, &QLineEdit::textChanged, &dialog, updateVisibility);
    QObject::connect(mobileOrientation, QOverload<int>::of(&QComboBox::currentIndexChanged), &dialog, updateVisibility);
    QObject::connect(browseCookie, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getOpenFileName(&dialog, "Select TikTok cookies", cookiePath->text(),
            "Cookie files (*.txt *.cookies);;All files (*.*)");
        if (!path.isEmpty())
            cookiePath->setText(path);
    });
    QObject::connect(qrLogin, &QPushButton::clicked, &dialog, [&] {
        QString savedCookiePath = cookiePath->text();
        if (LoginTikTokWithQr(&dialog, rapidApiKey->text(), &savedCookiePath, &profiles)) {
            cookiePath->setText(savedCookiePath);
        }
    });
    QObject::connect(detectToken, &QPushButton::clicked, &dialog, [&] {
        const auto token = DetectStreamlabsToken(&dialog);
        if (!token.isEmpty())
            streamlabsToken->setText(token);
        else
            QMessageBox::information(&dialog, "Streamlabs Desktop", "No Streamlabs Desktop token was found.");
    });

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    root->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        const auto selected = method->currentData().toString();
        const bool studioActive = selected == "live-studio" || selected == "live-studio-mobile";
        if (studioActive && topics->RequiresGame() && !topics->HasValidGame()) {
            QMessageBox::warning(&dialog, "TikTok LIVE", "Select a game from the TikTok catalogue before saving this gaming topic.");
            return;
        }
        if (studioActive && dualLayout->Enabled()) {
            QString validationError;
            if (!dualLayout->Validate(&validationError)) {
                QMessageBox::warning(&dialog, "TikTok Dual LIVE", validationError);
                return;
            }
        }
        dialog.accept();
    });
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    updateVisibility();

    if (dialog.exec() != QDialog::Accepted)
        return false;

    obs_data_set_string(settings, "tiktok_method", method->currentData().toString().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_title", title->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_rapidapi_key", rapidApiKey->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_cookie_path", cookiePath->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_topic_id", topics->TopicId().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_game_id", topics->GameId().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_game_name", topics->GameName().toUtf8().constData());
    obs_data_set_bool(settings, "tiktok_allow_gifts", allowGifts->isChecked());
    obs_data_set_bool(settings, "tiktok_allow_comments", allowComments->isChecked());
    obs_data_set_string(settings, "tiktok_content_disclosure", disclosure->currentData().toString().toUtf8().constData());
    obs_data_set_bool(settings, "tiktok_mature_content", studioMature->isChecked());
    obs_data_set_bool(settings, "tiktok_monitor_violations", monitorViolations->isChecked());
    TikTokDualLayoutConfig dualConfig;
    dualLayout->Save(dualConfig);
    const QByteArray dualJson = QJsonDocument(DualLayoutToJson(dualConfig)).toJson(QJsonDocument::Compact);
    obs_data_set_string(settings, "tiktok_dual_layout", dualJson.constData());
    mobileProfile.insert("broadcast_type", mobileBroadcast->currentData().toString());
    mobileProfile.insert("orientation", mobileOrientation->currentData().toString());
    mobileProfile.insert("device_id", mobileDeviceId->text().trimmed());
    mobileProfile.insert("install_id", mobileInstallId->text().trimmed());
    profiles.insert("mobile", mobileProfile);
    obs_data_set_string(settings, "tiktok_device_profiles",
        QJsonDocument(profiles).toJson(QJsonDocument::Compact).constData());
    obs_data_set_string(settings, "streamlabs_access_token", streamlabsToken->text().toUtf8().constData());
    obs_data_set_string(settings, "streamlabs_category", category->Name().toUtf8().constData());
    if (category->SelectedId())
        obs_data_set_string(settings, "streamlabs_category_id", category->SelectedId()->c_str());
    else
        obs_data_set_string(settings, "streamlabs_category_id", "");
    obs_data_set_bool(settings, "streamlabs_mature_content", streamlabsMature->isChecked());
    return true;
}
