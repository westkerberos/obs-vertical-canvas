#include "tiktok-encoder-properties-dialog.h"
#include <obs.h>
#include <obs-module.h>
#include <QJsonDocument>
#include <QByteArray>
#include <QCheckBox>
#include <QColor>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QVariant>
#include <QMetaObject>
#include <QStandardItemModel>
#include <algorithm>

namespace {
class EncoderPropertiesDialog final : public QDialog {
public:
    obs_properties_t *properties = nullptr;
    obs_data_t *settings = nullptr;
    QScrollArea *scroll = nullptr;
    QWidget *contents = nullptr;

    EncoderPropertiesDialog(obs_properties_t *props, obs_data_t *data, QWidget *parent)
        : QDialog(parent), properties(props), settings(data)
    {
        resize(540, 620);
        auto root = new QVBoxLayout(this);
        scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        root->addWidget(scroll, 1);
        auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        QObject::connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        QObject::connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        root->addWidget(buttons);
        Rebuild();
    }

    ~EncoderPropertiesDialog() override
    {
        if (properties)
            obs_properties_destroy(properties);
        if (settings)
            obs_data_release(settings);
    }

    void Rebuild()
    {
        if (properties)
            obs_properties_apply_settings(properties, settings);
        contents = new QWidget(scroll);
        auto form = new QFormLayout(contents);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        if (properties)
            BuildProperties(properties, form);
        if (form->rowCount() == 0) {
            auto label = new QLabel(obs_module_text("Dual.EncoderNoProperties"), contents);
            label->setWordWrap(true);
            form->addRow(label);
        }
        scroll->setWidget(contents);
    }

    void PropertyChanged(obs_property_t *property)
    {
        const bool rebuild = obs_property_modified(property, settings);
        if (properties)
            obs_properties_apply_settings(properties, settings);
        if (rebuild) {
            QMetaObject::invokeMethod(this, [this] { Rebuild(); }, Qt::QueuedConnection);
        } else {
            RefreshState(properties, contents);
        }
    }

    void BuildProperties(obs_properties_t *list, QFormLayout *target)
    {
        for (obs_property_t *property = obs_properties_first(list); property; obs_property_next(&property)) {
            if (!obs_property_visible(property))
                continue;
            const char *name = obs_property_name(property);
            const char *description = obs_property_description(property);
            if (!name)
                continue;
            const QString key = QString::fromUtf8(name);
            const QString label = QString::fromUtf8(description ? description : name);
            QWidget *control = nullptr;
            const auto type = obs_property_get_type(property);

            if (type == OBS_PROPERTY_GROUP) {
                auto group = new QGroupBox(label, contents);
                auto childForm = new QFormLayout(group);
                auto childProps = obs_property_group_content(property);
                if (childProps)
                    BuildProperties(childProps, childForm);
                target->addRow(group);
                continue;
            }

            switch (type) {
            case OBS_PROPERTY_BOOL: {
                auto box = new QCheckBox(contents);
                box->setObjectName(key);
                box->setChecked(obs_data_get_bool(settings, name));
                QObject::connect(box, &QCheckBox::toggled, this, [this, property, name](bool value) {
                    obs_data_set_bool(settings, name, value);
                    PropertyChanged(property);
                });
                control = box;
                break;
            }
            case OBS_PROPERTY_INT: {
                auto spin = new QSpinBox(contents);
                spin->setObjectName(key);
                int minimum = obs_property_int_min(property);
                int maximum = obs_property_int_max(property);
                if (minimum >= maximum) {
                    minimum = -2147483647;
                    maximum = 2147483647;
                }
                spin->setRange(minimum, maximum);
                spin->setSingleStep(std::max(1, obs_property_int_step(property)));
                spin->setValue(static_cast<int>(obs_data_get_int(settings, name)));
                const char *suffix = obs_property_int_suffix(property);
                if (suffix)
                    spin->setSuffix(QString::fromUtf8(suffix));
                QObject::connect(spin, QOverload<int>::of(&QSpinBox::valueChanged), this,
                    [this, property, name](int value) {
                        obs_data_set_int(settings, name, value);
                        PropertyChanged(property);
                    });
                control = spin;
                break;
            }
            case OBS_PROPERTY_FLOAT: {
                auto spin = new QDoubleSpinBox(contents);
                spin->setObjectName(key);
                double minimum = obs_property_float_min(property);
                double maximum = obs_property_float_max(property);
                if (minimum >= maximum) {
                    minimum = -1000000.0;
                    maximum = 1000000.0;
                }
                spin->setRange(minimum, maximum);
                spin->setDecimals(5);
                spin->setSingleStep(std::max(0.00001, obs_property_float_step(property)));
                spin->setValue(obs_data_get_double(settings, name));
                const char *suffix = obs_property_float_suffix(property);
                if (suffix)
                    spin->setSuffix(QString::fromUtf8(suffix));
                QObject::connect(spin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this,
                    [this, property, name](double value) {
                        obs_data_set_double(settings, name, value);
                        PropertyChanged(property);
                    });
                control = spin;
                break;
            }
            case OBS_PROPERTY_TEXT: {
                if (obs_property_text_type(property) == OBS_TEXT_INFO) {
                    auto info = new QLabel(QString::fromUtf8(obs_data_get_string(settings, name)), contents);
                    info->setWordWrap(obs_property_text_info_word_wrap(property));
                    control = info;
                } else if (obs_property_text_type(property) == OBS_TEXT_MULTILINE) {
                    auto edit = new QPlainTextEdit(contents);
                    edit->setObjectName(key);
                    edit->setPlainText(QString::fromUtf8(obs_data_get_string(settings, name)));
                    QObject::connect(edit, &QPlainTextEdit::textChanged, this, [this, property, name, edit] {
                        obs_data_set_string(settings, name, edit->toPlainText().toUtf8().constData());
                        PropertyChanged(property);
                    });
                    control = edit;
                } else {
                    auto edit = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, name)), contents);
                    edit->setObjectName(key);
                    if (obs_property_text_type(property) == OBS_TEXT_PASSWORD)
                        edit->setEchoMode(QLineEdit::Password);
                    QObject::connect(edit, &QLineEdit::textEdited, this, [this, property, name](const QString &value) {
                        obs_data_set_string(settings, name, value.toUtf8().constData());
                        PropertyChanged(property);
                    });
                    control = edit;
                }
                break;
            }
            case OBS_PROPERTY_PATH: {
                auto row = new QWidget(contents);
                auto rowLayout = new QHBoxLayout(row);
                rowLayout->setContentsMargins(0, 0, 0, 0);
                auto edit = new QLineEdit(QString::fromUtf8(obs_data_get_string(settings, name)), row);
                edit->setObjectName(key);
                auto browse = new QPushButton("…", row);
                rowLayout->addWidget(edit, 1);
                rowLayout->addWidget(browse);
                QObject::connect(edit, &QLineEdit::textEdited, this, [this, property, name](const QString &value) {
                    obs_data_set_string(settings, name, value.toUtf8().constData());
                    PropertyChanged(property);
                });
                QObject::connect(browse, &QPushButton::clicked, this, [this, property, name, edit] {
                    const auto pathType = obs_property_path_type(property);
                    const QString current = edit->text();
                    QString value;
                    if (pathType == OBS_PATH_DIRECTORY)
                        value = QFileDialog::getExistingDirectory(this, QString(), current);
                    else if (pathType == OBS_PATH_FILE_SAVE)
                        value = QFileDialog::getSaveFileName(this, QString(), current,
                            QString::fromUtf8(obs_property_path_filter(property)));
                    else
                        value = QFileDialog::getOpenFileName(this, QString(), current,
                            QString::fromUtf8(obs_property_path_filter(property)));
                    if (!value.isEmpty()) {
                        edit->setText(value);
                        obs_data_set_string(settings, name, value.toUtf8().constData());
                        PropertyChanged(property);
                    }
                });
                control = row;
                break;
            }
            case OBS_PROPERTY_LIST: {
                auto combo = new QComboBox(contents);
                combo->setObjectName(key);
                if (obs_property_list_type(property) == OBS_COMBO_TYPE_EDITABLE)
                    combo->setEditable(true);
                const auto format = obs_property_list_format(property);
                const size_t count = obs_property_list_item_count(property);
                for (size_t i = 0; i < count; ++i) {
                    const char *itemName = obs_property_list_item_name(property, i);
                    QVariant value;
                    if (format == OBS_COMBO_FORMAT_STRING)
                        value = QString::fromUtf8(obs_property_list_item_string(property, i));
                    else if (format == OBS_COMBO_FORMAT_INT)
                        value = QVariant::fromValue<qlonglong>(obs_property_list_item_int(property, i));
                    else if (format == OBS_COMBO_FORMAT_FLOAT)
                        value = obs_property_list_item_float(property, i);
                    else if (format == OBS_COMBO_FORMAT_BOOL)
                        value = obs_property_list_item_bool(property, i);
                    combo->addItem(QString::fromUtf8(itemName ? itemName : ""), value);
                    if (obs_property_list_item_disabled(property, i)) {
                        auto model = qobject_cast<QStandardItemModel *>(combo->model());
                        if (model && model->item(combo->count() - 1))
                            model->item(combo->count() - 1)->setEnabled(false);
                    }
                }
                if (format == OBS_COMBO_FORMAT_STRING)
                    combo->setCurrentIndex(combo->findData(QString::fromUtf8(obs_data_get_string(settings, name))));
                else if (format == OBS_COMBO_FORMAT_INT)
                    combo->setCurrentIndex(combo->findData(QVariant::fromValue<qlonglong>(obs_data_get_int(settings, name))));
                else if (format == OBS_COMBO_FORMAT_FLOAT)
                    combo->setCurrentIndex(combo->findData(obs_data_get_double(settings, name)));
                else if (format == OBS_COMBO_FORMAT_BOOL)
                    combo->setCurrentIndex(combo->findData(obs_data_get_bool(settings, name)));
                QObject::connect(combo, QOverload<int>::of(&QComboBox::activated), this,
                    [this, property, name, combo, format](int index) {
                        if (index < 0)
                            return;
                        const QVariant value = combo->itemData(index);
                        if (format == OBS_COMBO_FORMAT_STRING)
                            obs_data_set_string(settings, name, value.toString().toUtf8().constData());
                        else if (format == OBS_COMBO_FORMAT_INT)
                            obs_data_set_int(settings, name, value.toLongLong());
                        else if (format == OBS_COMBO_FORMAT_FLOAT)
                            obs_data_set_double(settings, name, value.toDouble());
                        else if (format == OBS_COMBO_FORMAT_BOOL)
                            obs_data_set_bool(settings, name, value.toBool());
                        PropertyChanged(property);
                    });
                control = combo;
                break;
            }
            case OBS_PROPERTY_COLOR:
            case OBS_PROPERTY_COLOR_ALPHA: {
                auto button = new QPushButton(contents);
                button->setObjectName(key);
                const QColor color = QColor::fromRgba(static_cast<QRgb>(obs_data_get_int(settings, name)));
                button->setText(color.name(QColor::HexArgb));
                button->setStyleSheet(QString("background-color: %1").arg(color.name(QColor::HexArgb)));
                QObject::connect(button, &QPushButton::clicked, this, [this, property, name, button] {
                    const QColor current = QColor::fromRgba(static_cast<QRgb>(obs_data_get_int(settings, name)));
                    const QColor picked = QColorDialog::getColor(current, this, QString(), QColorDialog::ShowAlphaChannel);
                    if (picked.isValid()) {
                        obs_data_set_int(settings, name, static_cast<long long>(picked.rgba()));
                        button->setText(picked.name(QColor::HexArgb));
                        button->setStyleSheet(QString("background-color: %1").arg(picked.name(QColor::HexArgb)));
                        PropertyChanged(property);
                    }
                });
                control = button;
                break;
            }
            case OBS_PROPERTY_BUTTON: {
                auto button = new QPushButton(label, contents);
                QObject::connect(button, &QPushButton::clicked, this, [this, property] {
                    if (obs_property_button_clicked(property, this))
                        Rebuild();
                });
                target->addRow(button);
                continue;
            }
            default: {
                auto unsupported = new QLabel(obs_module_text("Dual.EncoderPropertyUnsupported"), contents);
                unsupported->setWordWrap(true);
                control = unsupported;
                break;
            }
            }

            if (control) {
                control->setObjectName(key);
                control->setEnabled(obs_property_enabled(property));
                control->setToolTip(QString::fromUtf8(obs_property_long_description(property)
                    ? obs_property_long_description(property) : ""));
                target->addRow(label, control);
            }
        }
    }

    void RefreshState(obs_properties_t *list, QWidget *root)
    {
        if (!list || !root)
            return;
        for (obs_property_t *property = obs_properties_first(list); property; obs_property_next(&property)) {
            const char *name = obs_property_name(property);
            if (!name)
                continue;
            for (auto control : root->findChildren<QWidget *>()) {
                if (control->objectName() == QString::fromUtf8(name)) {
                    control->setVisible(obs_property_visible(property));
                    control->setEnabled(obs_property_enabled(property));
                }
            }
        }
    }
};
}

bool EditTikTokEncoderSettings(const std::string &encoderId, QJsonObject &encoderParams,
    int &bitrate, bool audio, QWidget *parent)
{
    if (encoderId.empty())
        return false;
    obs_data_t *settings = obs_encoder_defaults(encoderId.c_str());
    if (!settings)
        settings = obs_data_create();
    if (!settings)
        return false;

    const QByteArray savedJson = QJsonDocument(encoderParams).toJson(QJsonDocument::Compact);
    obs_data_t *saved = obs_data_create_from_json(savedJson.constData());
    if (saved) {
        obs_data_apply(settings, saved);
        obs_data_release(saved);
    }
    if (bitrate > 0)
        obs_data_set_int(settings, "bitrate", bitrate);

    obs_properties_t *properties = obs_get_encoder_properties(encoderId.c_str());
    EncoderPropertiesDialog dialog(properties, settings, parent);
    dialog.setWindowTitle(QString("%1 — %2").arg(obs_module_text("Dual.EncoderProperties"),
        audio ? obs_module_text("Dual.AudioEncoder") : obs_module_text("Encoder")));
    if (dialog.exec() != QDialog::Accepted)
        return false;

    const char *json = obs_data_get_json(settings);
    const auto doc = QJsonDocument::fromJson(QByteArray(json ? json : "{}"));
    if (doc.isObject())
        encoderParams = doc.object();
    const int updatedBitrate = static_cast<int>(obs_data_get_int(settings, "bitrate"));
    if (updatedBitrate > 0)
        bitrate = updatedBitrate;
    return true;
}
