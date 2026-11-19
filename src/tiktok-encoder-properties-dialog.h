#pragma once
#include <QJsonObject>
#include <string>
class QWidget;
bool EditTikTokEncoderSettings(const std::string &encoderId, QJsonObject &encoderParams,
    int &bitrate, bool audio, QWidget *parent);
