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
#include <QCryptographicHash>

static inline void dev_urandom(uint8_t* outputBuffer, const size_t outputBufferSize)
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

static int(*hashFunction)(const uint32_t, const uint32_t,
                          const uint32_t, const void*,
                          const size_t, const void*,
                          const size_t, const size_t,
                          char*, const size_t) = &argon2id_hash_encoded;

static const char* plural[] = {
    "",
    "s"
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    uint8_t initialEntropy [32];
    dev_urandom(initialEntropy,sizeof(initialEntropy));
    userEntropy = QString(reinterpret_cast<char*>(initialEntropy));
    appendEntropy(QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date));

    ui->hashAlgorithmButtonGroup->setId(ui->argon2idRadioButton, 0);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2iRadioButton, 1);
    ui->hashAlgorithmButtonGroup->setId(ui->argon2dRadioButton, 2);

    QObject::connect(ui->hashAlgorithmButtonGroup, SIGNAL(idClicked(int)), this, SLOT(onChangedHashAlgorithm(int)));

    ui->passwordLineEdit->setFocus();
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

    uint8_t salt[32] = {0x00};
    char encodedHash[1024] = {0x00};

    dev_urandom(salt, 16);
    memcpy(salt + 16, userEntropy.constData(), 16);

    QString output;
    const QString password = ui->passwordLineEdit->text();

    const QByteArray passwordUtf8Bytes = password.toUtf8();
    const char* passwordUtf8 =  passwordUtf8Bytes.constData();
    const size_t passwordUtf8Length = strlen(passwordUtf8); // This is OK and perfectly safe because, according to Qt documentation, ".constData()" returns a NUL-terminated buffer.

    const int argon2_timeCost = ui->timeCostHorizontalSlider->value();
    const int argon2_memoryCostMiB = ui->memoryCostHorizontalSlider->value();
    const int argon2_parallelism = ui->parallelismHorizontalSlider->value();
    const int hashLength = ui->hashLengthHorizontalSlider->value();

    const int r = hashFunction
    (
        static_cast<uint32_t>(argon2_timeCost),
        static_cast<uint32_t>(argon2_memoryCostMiB) * 1024,
        argon2_parallelism,
        passwordUtf8,
        passwordUtf8Length,
        salt,
        sizeof(salt),
        static_cast<size_t>(hashLength),
        encodedHash,
        sizeof(encodedHash)
    );

    if (r == ARGON2_OK)
    {
        output = QString(encodedHash);
    }
    else
    {
        char error[1024] = {0x00};
        snprintf(error, sizeof(error), "Argon2 hash generation failed! \"%s\" function call returned: %d\n", getHashFunctionName(), r);
        fprintf(stderr, "%s", error);
        output = QString(error);
    }

    busy = false;
    ui->encodedHashTextEdit->setText(output);
}

const char* MainWindow::getHashFunctionName() const
{
    switch(ui->hashAlgorithmButtonGroup->checkedId())
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
    switch(id)
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

