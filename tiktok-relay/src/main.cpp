#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>

#include <iostream>
#include <string>

namespace {

constexpr int protocol_version = 1;
constexpr qsizetype maximum_request_id_length = 64;

void write_response(const QJsonObject &response)
{
    const auto bytes = QJsonDocument(response).toJson(QJsonDocument::Compact);
    std::cout.write(bytes.constData(), static_cast<std::streamsize>(bytes.size()));
    std::cout << std::endl;
}

QJsonObject error_response(const QString &code, const QString &message)
{
    QJsonObject result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("error"), code);
    result.insert(QStringLiteral("message"), message);
    return result;
}

QJsonObject decorate_response(const QJsonObject &request, QJsonObject response)
{
    response.insert(QStringLiteral("protocol_version"), protocol_version);

    const QJsonValue request_id = request.value(QStringLiteral("request_id"));
    if (request_id.isString())
        response.insert(QStringLiteral("request_id"), request_id.toString());

    return response;
}

QJsonObject handle_command(const QJsonObject &request)
{
    const QJsonValue request_id = request.value(QStringLiteral("request_id"));
    if (request.contains(QStringLiteral("request_id")) &&
        (!request_id.isString() || request_id.toString().isEmpty() ||
         request_id.toString().size() > maximum_request_id_length)) {
        return decorate_response(
            QJsonObject{},
            error_response(QStringLiteral("invalid_request_id"),
                           QStringLiteral("request_id must be a non-empty string of at most 64 characters.")));
    }

    const QJsonValue version = request.value(QStringLiteral("protocol_version"));
    if (!version.isDouble() || version.toInt(-1) != protocol_version) {
        return decorate_response(
            request,
            error_response(QStringLiteral("protocol_version_mismatch"),
                           QStringLiteral("This relay supports control protocol version 1 only.")));
    }

    const QString command = request.value(QStringLiteral("command")).toString();

    if (command == QStringLiteral("hello")) {
        QJsonObject result;
        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("relay"), QStringLiteral("west-tiktok-relay"));
        result.insert(QStringLiteral("state"), QStringLiteral("prototype"));
        return decorate_response(request, result);
    }

    if (command == QStringLiteral("capabilities")) {
        QJsonObject features;
        features.insert(QStringLiteral("control_protocol"), true);
        features.insert(QStringLiteral("rtmp_ingest"), false);
        features.insert(QStringLiteral("tiktok_session"), false);
        features.insert(QStringLiteral("request_signing"), false);
        features.insert(QStringLiteral("dual_live"), false);

        QJsonObject result;
        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("features"), features);
        result.insert(QStringLiteral("implementation"), QStringLiteral("protocol-stub"));
        return decorate_response(request, result);
    }

    if (command == QStringLiteral("status")) {
        QJsonObject result;
        result.insert(QStringLiteral("ok"), true);
        result.insert(QStringLiteral("state"), QStringLiteral("idle"));
        result.insert(QStringLiteral("implementation"), QStringLiteral("protocol-stub"));
        return decorate_response(request, result);
    }

    if (command == QStringLiteral("start_live") || command == QStringLiteral("stop_live") ||
        command == QStringLiteral("account_status") ||
        command == QStringLiteral("dual_live_eligibility") ||
        command == QStringLiteral("get_topics") ||
        command == QStringLiteral("get_games") ||
        command == QStringLiteral("get_safety_status")) {
        return decorate_response(
            request,
            error_response(
                QStringLiteral("not_implemented"),
                QStringLiteral("TikTok relay runtime is not implemented. Do not send credentials to this prototype.")));
    }

    return decorate_response(
        request,
        error_response(QStringLiteral("unknown_command"),
                       QStringLiteral("The command is not part of this protocol version.")));
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    Q_UNUSED(application);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty())
            continue;

        QJsonParseError parse_error;
        const QJsonDocument document =
            QJsonDocument::fromJson(QByteArray::fromStdString(line), &parse_error);

        if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
            write_response(decorate_response(
                QJsonObject{},
                error_response(QStringLiteral("invalid_json"),
                               QStringLiteral("Expected one JSON object per input line."))));
            continue;
        }

        write_response(handle_command(document.object()));
    }

    return 0;
}
