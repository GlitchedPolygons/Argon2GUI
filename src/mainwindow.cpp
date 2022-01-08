#include "mainwindow.h"
#include "./ui_mainwindow.h"

#ifdef _WIN32
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <bcrypt.h>
#endif

#include "constants.h"

#include <argon2.h>

#include <QTimer>
#include <QDialog>
#include <QDateTime>
#include <QSettings>
#include <QMessageBox>
#include <QStyleFactory>
#include <QCryptographicHash>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int (*hashFunction)( /// This is the function pointer that holds a reference to the selected Argon2 algorithm variant selected by the user. ///////
        const uint32_t, //////// Time cost paramter (n of iterations)   ///////////////////////////////////////////////////////////////////////////////////
        const uint32_t, //////// Memory cost parameter (in KiB)         ///////////////////////////////////////////////////////////////////////////////////
        const uint32_t, //////// Parallelism parameter (n of threads)   ///////////////////////////////////////////////////////////////////////////////////
        const void*, /////////// The password to hash with Argon2       ///////////////////////////////////////////////////////////////////////////////////
        const size_t, ////////// Length of the passed password          ///////////////////////////////////////////////////////////////////////////////////
        const void*, /////////// Salt bytes to use for hashing (these should be random)   /////////////////////////////////////////////////////////////////
        const size_t, ////////// Length of the passed salt bytes array   //////////////////////////////////////////////////////////////////////////////////
        const size_t, ////////// Desired hash length in bytes            //////////////////////////////////////////////////////////////////////////////////
        char*, ///////////////// Output buffer for the encoded hash      //////////////////////////////////////////////////////////////////////////////////
        const size_t /////////// Size of the output buffer in bytes               /////////////////////////////////////////////////////////////////////////
        ) /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        = &argon2id_hash_encoded; /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Read n bytes from /dev/urandom (or BCryptGenRandom on Windows).
static const inline void dev_urandom(uint8_t* outputBuffer, const size_t outputBufferSize)
{
    if (outputBuffer != NULL && outputBufferSize > 0)
    {
#ifdef _WIN32
        BCryptGenRandom(NULL, outputBuffer, (ULONG)outputBufferSize, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
#else
        FILE* rnd = fopen("/dev/urandom", "r");
        if (rnd != NULL)
        {
            const size_t n = fread(outputBuffer, sizeof(unsigned char), outputBufferSize, rnd);
            if (n != outputBufferSize)
            {
                fprintf(stderr, "Argon2UI: Warning! Only %zu bytes out of %zu have been read from /dev/urandom\n", n, outputBufferSize);
            }
            fclose(rnd);
        }
#endif
    }
}

static const inline argon2_type argonAlgoFromEncodedHashString(const char* encodedHashUtf8)
{
    if (strstr(encodedHashUtf8, "$argon2id$") == encodedHashUtf8)
        return Argon2_id;

    if (strstr(encodedHashUtf8, "$argon2i$") == encodedHashUtf8)
        return Argon2_i;

    return Argon2_d;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow) /////////////////
{
    ui->setupUi(this);

    loadSettings();

#ifdef __APPLE__
    setAttribute(Qt::WA_MacSmallSize);
#endif

    uint8_t initialEntropy[32];
    dev_urandom(initialEntropy, sizeof(initialEntropy));
    userEntropy = QString(reinterpret_cast<char*>(initialEntropy));
    appendEntropy(QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date));

    ui->hashAlgorithmButtonGroup->setId(ui->argon2idRadioButton, 0);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2iRadioButton, 1);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2dRadioButton, 2);
    ui->aboutTextLabel->setText(QString("About Argon2 GUI - v%1").arg(Constants::appVersion));

    QObject::connect(ui->hashAlgorithmButtonGroup, SIGNAL(idClicked(int)), this, SLOT(onChangedHashAlgorithm(int)));

    on_tabWidget_currentChanged(0);
}

MainWindow::~MainWindow()
{
    QSettings settings;

    const bool saveParams = ui->saveParametersOnQuitCheckBox->isChecked();
    const bool saveWindow = ui->saveWindowSizeOnQuitCheckBox->isChecked();
    const bool selectTextOnFocus = ui->selectTextOnFocusCheckBox->isChecked();

    if (saveParams)
    {
        settings.setValue(Constants::Settings::hashAlgo, QVariant(ui->hashAlgorithmButtonGroup->checkedId()));
        settings.setValue(Constants::Settings::timeCost, QVariant(ui->timeCostHorizontalSlider->value()));
        settings.setValue(Constants::Settings::memoryCost, QVariant(ui->memoryCostHorizontalSlider->value()));
        settings.setValue(Constants::Settings::parallelism, QVariant(ui->parallelismHorizontalSlider->value()));
        settings.setValue(Constants::Settings::hashLength, QVariant(ui->hashLengthHorizontalSlider->value()));
    }

    if (saveWindow)
    {
        settings.setValue(Constants::Settings::windowWidth, QVariant(width()));
        settings.setValue(Constants::Settings::windowHeight, QVariant(height()));
    }

    settings.setValue(Constants::Settings::saveHashParametersOnQuit, QVariant(saveParams));
    settings.setValue(Constants::Settings::saveWindowSizeOnQuit, QVariant(saveWindow));
    settings.setValue(Constants::Settings::selectTextOnFocus, QVariant(selectTextOnFocus));

    delete ui;
}

void MainWindow::loadSettings()
{
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings settings;

    const bool saveParams = settings.value(Constants::Settings::saveHashParametersOnQuit, QVariant(true)).toBool();
    const bool saveWindow = settings.value(Constants::Settings::saveWindowSizeOnQuit, QVariant(true)).toBool();
    const bool selectTextOnFocus = settings.value(Constants::Settings::selectTextOnFocus, QVariant(true)).toBool();

    ui->saveParametersOnQuitCheckBox->setChecked(saveParams);
    ui->saveWindowSizeOnQuitCheckBox->setChecked(saveWindow);
    ui->selectTextOnFocusCheckBox->setChecked(selectTextOnFocus);

    if (saveParams)
    {
        switch (settings.value(Constants::Settings::hashAlgo, QVariant(0)).toInt())
        {
            default:
                ui->argon2idRadioButton->setChecked(true);
                break;
            case 1:
                ui->argon2iRadioButton->setChecked(true);
                break;
            case 2:
                ui->argon2dRadioButton->setChecked(true);
                break;
        }

        ui->timeCostHorizontalSlider->setValue(settings.value(Constants::Settings::timeCost, QVariant(Constants::Settings::DefaultValues::timeCost)).toInt());
        ui->memoryCostHorizontalSlider->setValue(settings.value(Constants::Settings::memoryCost, QVariant(Constants::Settings::DefaultValues::memoryCostMiB)).toInt());
        ui->parallelismHorizontalSlider->setValue(settings.value(Constants::Settings::parallelism, QVariant(Constants::Settings::DefaultValues::parallelism)).toInt());
        ui->hashLengthHorizontalSlider->setValue(settings.value(Constants::Settings::hashLength, QVariant(Constants::Settings::DefaultValues::hashLength)).toInt());
    }

    if (saveWindow)
    {
        const int w = settings.value(Constants::Settings::windowWidth, QVariant(minimumSize().width())).toInt();
        const int h = settings.value(Constants::Settings::windowHeight, QVariant(minimumSize().height())).toInt();

        this->resize(w > 0 ? w : -w, h > 0 ? h : -h);
    }
}

void MainWindow::appendEntropy(const QString& additionalEntropy)
{
    const QString timestamp = QDateTime::currentDateTime().toString(Qt::DateFormat::ISODateWithMs);

    QCryptographicHash sha256(QCryptographicHash::Algorithm::Sha256);
    sha256.addData((userEntropy + additionalEntropy + timestamp).toUtf8());

    userEntropy = QString(sha256.result());
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    switch (index)
    {
        case 0:
            ui->passwordLineEdit->setFocus();
            ui->passwordLineEdit->selectAll();
            break;
        case 1:
            ui->inputTextEdit->setFocus();
            ui->inputTextEdit->selectAll();
            break;
        default:
            break;
    }
}

void MainWindow::on_showPasswordButton_pressed()
{
    ui->passwordLineEdit->setEchoMode(QLineEdit::EchoMode::Normal);
    ui->showPasswordButton->setText("Hide");
}

void MainWindow::on_showPasswordButton_released()
{
    ui->passwordLineEdit->setEchoMode(QLineEdit::EchoMode::Password);
    ui->showPasswordButton->setText("Show");
}

void MainWindow::on_clearButton_clicked()
{
    ui->passwordLineEdit->clear();
    ui->encodedHashTextEdit->clear();
}

void MainWindow::on_hashButton_clicked()
{
    if (busy)
    {
        return;
    }

    busy = true;

    ui->encodedHashTextEdit->setText("Working on it... CPU go brr!");
    repaint();

    uint8_t salt[32] = { 0x00 };
    char encodedHash[1024] = { 0x00 };

    dev_urandom(salt, 16);
    memcpy(salt + 16, userEntropy.constData(), 16);

    QString output;
    const QString password = ui->passwordLineEdit->text();

    const QByteArray passwordUtf8Bytes = password.toUtf8();
    const char* passwordUtf8 = passwordUtf8Bytes.constData();
    const size_t passwordUtf8Length = strlen(passwordUtf8); // This is OK and perfectly safe because, according to Qt documentation, ".constData()" returns a NUL-terminated buffer.

    const int argon2_timeCost = ui->timeCostHorizontalSlider->value();
    const int argon2_memoryCostMiB = ui->memoryCostHorizontalSlider->value();
    const int argon2_parallelism = ui->parallelismHorizontalSlider->value();
    const int hashLength = ui->hashLengthHorizontalSlider->value();

    const int r = hashFunction(static_cast<uint32_t>(argon2_timeCost), static_cast<uint32_t>(argon2_memoryCostMiB) * 1024, argon2_parallelism, passwordUtf8, passwordUtf8Length, salt, sizeof(salt), static_cast<size_t>(hashLength), encodedHash, sizeof(encodedHash));

    if (r == ARGON2_OK)
    {
        output = QString(encodedHash);
    }
    else
    {
        char error[1024] = { 0x00 };
        snprintf(error, sizeof(error), "Argon2 hash generation failed! \"%s\" function call returned: %d\n", getHashFunctionName(), r);
        fprintf(stderr, "%s", error);
        output = QString(error);
    }

    busy = false;
    ui->encodedHashTextEdit->setText(output);
}

const char* MainWindow::getHashFunctionName() const
{
    switch (ui->hashAlgorithmButtonGroup->checkedId())
    {
        default:
            return "argon2id_hash_encoded";
        case 1:
            return "argon2i_hash_encoded";
        case 2:
            return "argon2d_hash_encoded";
    }
}

void MainWindow::onChangedHashAlgorithm(int id)
{
    switch (id)
    {
        default:
            hashFunction = &argon2id_hash_encoded;
            break;
        case 1:
            hashFunction = &argon2i_hash_encoded;
            break;
        case 2:
            hashFunction = &argon2d_hash_encoded;
            break;
    }
}

void MainWindow::on_timeCostHorizontalSlider_valueChanged(int value)
{
    QString newLabelText = QString("Time cost (%1 iteration%2)").arg(value).arg(Constants::plural[value > 1]);
    ui->timeCostLabel->setText(newLabelText);
    appendEntropy(newLabelText);
}

void MainWindow::on_memoryCostHorizontalSlider_valueChanged(int value)
{
    QString newLabelText = QString("Memory cost (%1 MiB)").arg(value);
    ui->memoryCostLabel->setText(newLabelText);
    appendEntropy(newLabelText);
}

void MainWindow::on_parallelismHorizontalSlider_valueChanged(int value)
{
    QString newLabelText = QString("Parallelism (%1 thread%2)").arg(value).arg(Constants::plural[value > 1]);
    ui->parallelismLabel->setText(newLabelText);
    appendEntropy(newLabelText);
}

void MainWindow::on_hashLengthHorizontalSlider_valueChanged(int value)
{
    QString newLabelText = QString("Hash length (%1 B)").arg(value);
    ui->hashLengthLabel->setText(newLabelText);
    appendEntropy(newLabelText);
}

void MainWindow::on_encodedHashTextEdit_selectionChanged()
{
    appendEntropy(ui->encodedHashTextEdit->textCursor().selectedText());
}

void MainWindow::on_clearVerificationFieldsButton_clicked()
{
    ui->inputTextEdit->clear();
    ui->inputPasswordLineEdit->clear();
    ui->verificationResultLabel->clear();
}

void MainWindow::on_verifyButton_clicked()
{
    ui->verificationResultLabel->clear();
    repaint();

    const QString password = ui->inputPasswordLineEdit->text();
    const QString encodedHash = ui->inputTextEdit->toPlainText().replace(" ", "").replace("\t", "").replace("\n", "").replace("\r\n", "");

    const QByteArray passwordUtf8Bytes = password.toUtf8();
    const char* passwordUtf8 = passwordUtf8Bytes.constData();
    const size_t passwordUtf8Length = strlen(passwordUtf8);

    const QByteArray encodedHashUtf8Bytes = encodedHash.toUtf8();
    const char* encodedHashUtf8 = encodedHashUtf8Bytes.constData();

    const int r = argon2_verify(encodedHashUtf8, passwordUtf8, passwordUtf8Length, argonAlgoFromEncodedHashString(encodedHashUtf8));

    if (r == ARGON2_OK)
    {
        ui->verificationResultLabel->setText("✅  Verification successful.\nArgon2 hash matches the entered password.");
    }
    else
    {
        ui->verificationResultLabel->setText(QString("❌  Verification failed.\n\"argon2_verify\" function call returned error code %1.").arg(r));
    }
}

void MainWindow::on_showInputPasswordButton_pressed()
{
    ui->inputPasswordLineEdit->setEchoMode(QLineEdit::EchoMode::Normal);
    ui->showInputPasswordButton->setText("Hide");
}

void MainWindow::on_showInputPasswordButton_released()
{
    ui->inputPasswordLineEdit->setEchoMode(QLineEdit::EchoMode::Password);
    ui->showInputPasswordButton->setText("Show");
}

void MainWindow::on_factoryResetPushButton_clicked()
{
    QMessageBox msgBox;
    msgBox.setText("Are you absolutely sure?");
    msgBox.setInformativeText("This action is irreversible! Do you really want to restore all default settings?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    const int r = msgBox.exec();
    if (r != QMessageBox::Yes)
    {
        return;
    }

    ui->argon2idRadioButton->setChecked(true);

    ui->saveParametersOnQuitCheckBox->setChecked(Constants::Settings::DefaultValues::saveHashParametersOnQuit);
    ui->saveWindowSizeOnQuitCheckBox->setChecked(Constants::Settings::DefaultValues::saveWindowSizeOnQuit);
    ui->selectTextOnFocusCheckBox->setChecked(Constants::Settings::DefaultValues::selectTextOnFocus);

    ui->timeCostHorizontalSlider->setValue(Constants::Settings::DefaultValues::timeCost);
    ui->memoryCostHorizontalSlider->setValue(Constants::Settings::DefaultValues::memoryCostMiB);
    ui->parallelismHorizontalSlider->setValue(Constants::Settings::DefaultValues::parallelism);
    ui->hashLengthHorizontalSlider->setValue(Constants::Settings::DefaultValues::hashLength);

    ui->passwordLineEdit->clear();
    ui->inputTextEdit->clear();
    ui->inputPasswordLineEdit->clear();

    resize(minimumWidth(), minimumHeight());
}

void MainWindow::onChangedFocus(QWidget*, QWidget* newlyFocusedWidget)
{
    {
        QSettings settings;

        if (!settings.value(Constants::Settings::selectTextOnFocus, QVariant(Constants::Settings::DefaultValues::selectTextOnFocus)).toBool())
        {
            return;
        }
    }

    if (newlyFocusedWidget == ui->inputTextEdit)
    {
        QTimer::singleShot(0, ui->inputTextEdit, &QTextEdit::selectAll);
    }
    else if (newlyFocusedWidget == ui->encodedHashTextEdit)
    {
        QTimer::singleShot(0, ui->encodedHashTextEdit, &QTextEdit::selectAll);
    }
    else if (newlyFocusedWidget == ui->passwordLineEdit)
    {
        QTimer::singleShot(0, ui->passwordLineEdit, &QLineEdit::selectAll);
    }
    else if (newlyFocusedWidget == ui->inputPasswordLineEdit)
    {
        QTimer::singleShot(0, ui->inputPasswordLineEdit, &QLineEdit::selectAll);
    }
}
