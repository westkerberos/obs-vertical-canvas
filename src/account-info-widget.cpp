#include "account-info-widget.h"
#include <obs-module.h>
#include <QJsonDocument>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

AccountInfoWidget::AccountInfoWidget(AccountInfoLookup lookup, QWidget *parent)
    : QWidget(parent), lookup_(std::move(lookup))
{
    auto layout = new QFormLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    auto row = new QHBoxLayout;
    username_ = new QLineEdit(this);
    username_->setObjectName("tiktokAccountUsername");
    username_->setReadOnly(true);
    refresh_ = new QPushButton(obs_module_text("Account.Refresh"), this);
    refresh_->setObjectName("refreshTikTokAccount");
    refresh_->setAutoDefault(false);
    row->addWidget(username_, 1);
    row->addWidget(refresh_);
    layout->addRow(obs_module_text("Account.Username"), row);

    streamlabsDetails_ = new QWidget(this);
    auto details = new QFormLayout(streamlabsDetails_);
    details->setContentsMargins(0, 0, 0, 0);
    applicationStatus_ = new QLineEdit(streamlabsDetails_);
    applicationStatus_->setObjectName("streamlabsApplicationStatus");
    applicationStatus_->setReadOnly(true);
    canGoLive_ = new QLineEdit(streamlabsDetails_);
    canGoLive_->setObjectName("streamlabsCanGoLive");
    canGoLive_->setReadOnly(true);
    details->addRow(obs_module_text("Account.ApplicationStatus"), applicationStatus_);
    details->addRow(obs_module_text("Account.CanGoLive"), canGoLive_);
    layout->addRow(streamlabsDetails_);
    streamlabsDetails_->hide();

    status_ = new QLabel(this);
    status_->setObjectName("tiktokAccountStatus");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    layout->addRow(status_);

    dualDetails_ = new QWidget(this);
    auto dualForm = new QFormLayout(dualDetails_);
    dualForm->setContentsMargins(0, 0, 0, 0);
    canDualLayout_ = new QLineEdit(dualDetails_);
    canDualLayout_->setReadOnly(true);
    canDualLayout_->setObjectName("canDualLayout");
    dualForm->addRow(obs_module_text("Dual.CanDualLayout"), canDualLayout_);
    dualStatus_ = new QLabel(dualDetails_);
    dualStatus_->setObjectName("dualLayoutAccessStatus");
    dualStatus_->setTextFormat(Qt::PlainText);
    dualStatus_->setWordWrap(true);
    dualForm->addRow(dualStatus_);
    layout->addRow(dualDetails_);
    dualDetails_->hide();

    debounce_.setSingleShot(true);
    QObject::connect(&debounce_, &QTimer::timeout, this, [this] {
        pending_ = true;
        StartLookup();
    });
    QObject::connect(&poll_, &QTimer::timeout, this, [this] { Poll(); });
    QObject::connect(refresh_, &QPushButton::clicked, this, [this] { Refresh(); });
}

AccountInfoWidget::~AccountInfoWidget()
{
    if (cancelled_)
        *cancelled_ = true;
    if (result_.valid())
        result_.wait();
}

const char *AccountInfoWidget::InputHint() const
{
    if (request_.method == "custom")
        return "Account.NotApplicable";
    if (request_.method == "streamlabs")
        return request_.streamlabsAccessToken.empty() ? "Account.EnterToken" : nullptr;
    return request_.rapidApiKey.empty() || request_.cookieFilePath.empty() ? "Account.EnterCookies" : nullptr;
}

static std::string LookupIdentity(const TikTokOutputConfig &config)
{
    QJsonObject identity;
    identity.insert("method", QString::fromStdString(config.method));
    if (config.method == "streamlabs") {
        identity.insert("token", QString::fromStdString(config.streamlabsAccessToken));
    } else if (config.method == "live-studio" || config.method == "live-studio-mobile") {
        identity.insert("key", QString::fromStdString(config.rapidApiKey));
        identity.insert("cookies", QString::fromStdString(config.cookieFilePath));
        identity.insert("client", QString::fromStdString(config.client));
        identity.insert("devices", config.deviceProfiles);
    }
    return QJsonDocument(identity).toJson(QJsonDocument::Compact).toStdString();
}

void AccountInfoWidget::SetConfig(const TikTokOutputConfig &config)
{
    TikTokOutputConfig request;
    request.method = config.method;
    if (config.method == "streamlabs") {
        request.streamlabsAccessToken = config.streamlabsAccessToken;
    } else if (config.method == "live-studio" || config.method == "live-studio-mobile") {
        request.rapidApiKey = config.rapidApiKey;
        request.cookieFilePath = config.cookieFilePath;
        request.client = config.client;
        request.deviceProfiles = config.deviceProfiles;
    }

    setVisible(config.method != "custom");
    dualDetails_->setVisible(config.method == "live-studio" || config.method == "live-studio-mobile");
    streamlabsDetails_->setVisible(config.method == "streamlabs");
    const auto next = LookupIdentity(request);
    if (identity_ == next)
        return;
    identity_ = next;
    request_ = std::move(request);
    Refresh();
}

void AccountInfoWidget::Refresh()
{
    ++generation_;
    pending_ = false;
    debounce_.stop();
    if (cancelled_)
        *cancelled_ = true;

    const auto hint = InputHint();
    username_->clear();
    canDualLayout_->setText(obs_module_text("Account.Unknown"));
    dualStatus_->setText(obs_module_text(hint ? "Dual.AccessUnknown" : "Dual.CheckingAccess"));
    if (dualAccessChanged_)
        dualAccessChanged_(std::nullopt);
    applicationStatus_->setText(obs_module_text("Account.Unknown"));
    canGoLive_->setText(obs_module_text("Account.Unknown"));
    status_->setText(obs_module_text(hint ? hint : "Account.Loading"));
    status_->show();
    refresh_->setEnabled(!hint);
    if (!hint)
        debounce_.start(600);
    if (contentChanged_)
        contentChanged_();
}

void AccountInfoWidget::StartLookup()
{
    if (!pending_ || result_.valid())
        return;
    pending_ = false;
    runningGeneration_ = generation_;
    cancelled_ = std::make_shared<std::atomic_bool>(false);
    const auto cancelled = cancelled_;
    const auto request = request_;
    const auto lookup = lookup_;
    result_ = std::async(std::launch::async, [request, cancelled, lookup] {
        try {
            return lookup(request, *cancelled);
        } catch (...) {
            return AccountInfoResult{{}, "Account.NetworkError"};
        }
    });
    poll_.start(50);
}

void AccountInfoWidget::Poll()
{
    if (!result_.valid() || result_.wait_for(std::chrono::seconds(0)) != std::future_status::ready)
        return;

    const auto result = result_.get();
    poll_.stop();
    if (runningGeneration_ == generation_ && !*cancelled_) {
        if (!result.deviceProfile.isEmpty() &&
            (request_.method == "live-studio" || request_.method == "live-studio-mobile")) {
            request_.deviceProfiles.insert(QString::fromStdString(request_.client), result.deviceProfile);
            identity_ = LookupIdentity(request_);
            if (deviceProfileChanged_)
                deviceProfileChanged_(request_.client, result.deviceProfile);
        }

        const auto mobile = request_.deviceProfiles.value("mobile").toObject();
        const bool cameraLive = request_.client != "mobile" ||
            mobile.value("broadcast_type").toString("video") == "video";
        const QString dualAllowed = !cameraLive ? "Dual.CameraOnly" :
            !result.canDualLayout.has_value() ? "Account.Unknown" :
            *result.canDualLayout ? "Account.Yes" : "Account.No";
        canDualLayout_->setText(obs_module_text(dualAllowed.toUtf8().constData()));
        dualStatus_->setText(!cameraLive ? obs_module_text("Dual.Error.CameraOnly") :
            !result.dualLayoutStatus.isEmpty() ? result.dualLayoutStatus :
            obs_module_text(result.canDualLayout.value_or(false) ? "Dual.AccessAvailable" :
                result.canDualLayout.has_value() ? "Dual.AccessLocked" : "Dual.AccessUnknown"));
        if (dualAccessChanged_)
            dualAccessChanged_(result.canDualLayout);

        if (!result.errorKey && (!result.username.isEmpty() || !result.applicationStatus.isEmpty() ||
            result.canGoLive.has_value())) {
            username_->setText(result.username.isEmpty() ? obs_module_text("Account.Unknown") : result.username);
            username_->setCursorPosition(0);
            applicationStatus_->setText(result.applicationStatus.isEmpty() ?
                obs_module_text("Account.Unknown") : result.applicationStatus);
            applicationStatus_->setCursorPosition(0);
            canGoLive_->setText(obs_module_text(result.canGoLive.has_value() ?
                (*result.canGoLive ? "Account.Yes" : "Account.No") : "Account.Unknown"));
            status_->hide();
        } else {
            status_->setText(result.errorMessage.isEmpty() ?
                obs_module_text(result.errorKey ? result.errorKey : "Account.Unavailable") : result.errorMessage);
            status_->show();
        }
    }
    StartLookup();
    if (contentChanged_)
        contentChanged_();
}
