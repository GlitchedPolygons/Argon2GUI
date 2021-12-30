#include "mainwindow.h"
#include "./ui_mainwindow.h"

#ifdef _WIN32
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <bcrypt.h>
#endif

#include <argon2.h>
#include <QDateTime>
#include <QSettings>
#include <QStyleFactory>
#include <QCryptographicHash>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static int (*hashFunction) ( /// This is the function pointer that holds a reference to the selected Argon2 algorithm variant selected by the user. ///////
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

// Tiny lookup table for english plural suffix (accessible via a boolean check as an index).
static const char* plural[] = { "", "s" };

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
                fprintf(stderr, "Argon2UI: Warning! Only %llu bytes out of %llu have been read from /dev/urandom\n", n, outputBufferSize);
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

MainWindow::MainWindow(QWidget* parent) //////
    : QMainWindow(parent) ////////////////////
    , ui(new Ui::MainWindow) /////////////////
{
    ui->setupUi(this);

    uint8_t initialEntropy[32];
    dev_urandom(initialEntropy, sizeof(initialEntropy));
    userEntropy = QString(reinterpret_cast<char*>(initialEntropy));
    appendEntropy(QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date));

    ui->hashAlgorithmButtonGroup->setId(ui->argon2idRadioButton, 0);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2iRadioButton, 1);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2dRadioButton, 2);

    QObject::connect(ui->hashAlgorithmButtonGroup, SIGNAL(idClicked(int)), this, SLOT(onChangedHashAlgorithm(int)));

    on_tabWidget_currentChanged(0);

#ifdef Q_OS_WIN
    QSettings settings("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", QSettings::NativeFormat);
    if (settings.value("AppsUseLightTheme") == 0)
    {
        qApp->setStyle(QStyleFactory::create("Windows"));
        QPalette darkPalette;
        QColor darkColor = QColor(45, 45, 45);
        QColor disabledColor = QColor(127, 127, 127);
        darkPalette.setColor(QPalette::Window, darkColor);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, QColor(18, 18, 18));
        darkPalette.setColor(QPalette::AlternateBase, darkColor);
        darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, disabledColor);
        darkPalette.setColor(QPalette::Button, darkColor);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, disabledColor);
        darkPalette.setColor(QPalette::BrightText, Qt::red);
        darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));

        darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        darkPalette.setColor(QPalette::HighlightedText, Qt::black);
        darkPalette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabledColor);

        qApp->setPalette(darkPalette);

        qApp->setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; border: 1px solid white; }");
    }
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
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
    QString newLabelText = QString("Time cost (%1 iteration%2)").arg(value).arg(plural[value > 1]);
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
    QString newLabelText = QString("Parallelism (%1 thread%2)").arg(value).arg(plural[value > 1]);
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
    const QString encodedHash = ui->inputTextEdit->toPlainText()
            .replace(" ", "")
            .replace("\t", "")
            .replace("\n", "")
            .replace("\r\n", "");

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
