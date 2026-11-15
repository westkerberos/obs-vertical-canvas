#pragma once

#include <obs.h>
#include <QWidget>

bool ConfigureTikTokOutput(QWidget *parent, obs_data_t *settings, const QString &outputName);
