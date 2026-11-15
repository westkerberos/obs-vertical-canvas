#include "tiktok-output-settings-dialog.h"

#include "streamlabs-desktop.h"
#include "tiktok-native/live_client.hpp"
#include "tiktok-native/qr_login.hpp"
#include "tiktok-native/request_cancellation.hpp"

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
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>

#include <atomic>
#include <future>

namespace {
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
} // namespace

bool ConfigureTikTokOutput(QWidget *parent, obs_data_t *settings, const QString &outputName)
{
    if (!settings)
        return false;

    QDialog dialog(parent);
    dialog.setWindowTitle(QString("Configure %1").arg(outputName.isEmpty() ? "TikTok" : outputName));
    dialog.setMinimumWidth(520);
    auto *root = new QVBoxLayout(&dialog);
    auto *form = new QFormLayout;
    root->addLayout(form);

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

    auto *topicId = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "tiktok_topic_id")), studioGroup);
    auto *gameId = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "tiktok_game_id")), studioGroup);
    studioForm->addRow("Topic ID", topicId);
    studioForm->addRow("Game ID (optional)", gameId);
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
    auto *studioMature = new QCheckBox("Mature content", studioGroup);
    studioMature->setChecked(obs_data_get_bool(settings, "tiktok_mature_content"));
    studioForm->addRow(studioMature);
    form->addRow(studioGroup);

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
    auto *category = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, "streamlabs_category")), streamlabsGroup);
    streamlabsForm->addRow("Category", category);
    auto *streamlabsMature = new QCheckBox("Mature content", streamlabsGroup);
    streamlabsMature->setChecked(obs_data_get_bool(settings, "streamlabs_mature_content"));
    streamlabsForm->addRow(streamlabsMature);
    form->addRow(streamlabsGroup);

    auto updateVisibility = [=] {
        const auto selected = method->currentData().toString();
        studioGroup->setVisible(selected == "live-studio" || selected == "live-studio-mobile");
        streamlabsGroup->setVisible(selected == "streamlabs");
    };
    QObject::connect(method, &QComboBox::currentIndexChanged, &dialog, updateVisibility);
    QObject::connect(browseCookie, &QPushButton::clicked, &dialog, [&] {
        const auto path = QFileDialog::getOpenFileName(&dialog, "Select TikTok cookies", cookiePath->text(),
            "Cookie files (*.txt *.cookies);;All files (*.*)");
        if (!path.isEmpty())
            cookiePath->setText(path);
    });
    QObject::connect(qrLogin, &QPushButton::clicked, &dialog, [&] {
        QJsonObject profiles = QJsonDocument::fromJson(
            QByteArray(obs_data_get_string(settings, "tiktok_device_profiles"))).object();
        QString savedCookiePath = cookiePath->text();
        if (LoginTikTokWithQr(&dialog, rapidApiKey->text(), &savedCookiePath, &profiles)) {
            cookiePath->setText(savedCookiePath);
            obs_data_set_string(settings, "tiktok_device_profiles",
                QJsonDocument(profiles).toJson(QJsonDocument::Compact).constData());
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
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    updateVisibility();

    if (dialog.exec() != QDialog::Accepted)
        return false;

    obs_data_set_string(settings, "tiktok_method", method->currentData().toString().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_title", title->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_rapidapi_key", rapidApiKey->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_cookie_path", cookiePath->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_topic_id", topicId->text().toUtf8().constData());
    obs_data_set_string(settings, "tiktok_game_id", gameId->text().toUtf8().constData());
    obs_data_set_bool(settings, "tiktok_allow_gifts", allowGifts->isChecked());
    obs_data_set_bool(settings, "tiktok_allow_comments", allowComments->isChecked());
    obs_data_set_string(settings, "tiktok_content_disclosure", disclosure->currentData().toString().toUtf8().constData());
    obs_data_set_bool(settings, "tiktok_mature_content", studioMature->isChecked());
    obs_data_set_string(settings, "streamlabs_access_token", streamlabsToken->text().toUtf8().constData());
    obs_data_set_string(settings, "streamlabs_category", category->text().toUtf8().constData());
    obs_data_set_bool(settings, "streamlabs_mature_content", streamlabsMature->isChecked());
    return true;
}
