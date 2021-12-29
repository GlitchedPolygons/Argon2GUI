#include "mainwindow.h"
#include "./ui_mainwindow.h"

extern "C" {
#include <argon2.h>
}

#ifdef _WIN32
#define WIN32_NO_STATUS
#include <windows.h>
#undef WIN32_NO_STATUS
#include <bcrypt.h>
#endif

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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    uint8_t initialEntropy [32];
    dev_urandom(initialEntropy,sizeof(initialEntropy));
    userEntropy = QString(reinterpret_cast<char*>(initialEntropy));
    appendEntropy(QDateTime::currentDateTime().toString(Qt::DateFormat::RFC2822Date));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::appendEntropy(const QString& additionalEntropy)
{
    QCryptographicHash sha256(QCryptographicHash::Algorithm::Sha256);
    sha256.addData((userEntropy + additionalEntropy).toUtf8());
    userEntropy = QString(sha256.result());
}

void MainWindow::on_clearButton_clicked()
{
    ui->passwordLineEdit->clear();
    ui->encodedHashTextEdit->clear();
}


void MainWindow::on_hashButton_clicked()
{
    if (busy){
        return;
    }

    busy = true;
    ui->encodedHashTextEdit->setText("Working on it... CPU go brr!");
    ui->hashButtonsHorizontalLayout->setEnabled(false);

    uint8_t salt[32] = {0x00};
    char encodedHash[1024] = {0x00};

    dev_urandom(salt, 16);
    memcpy(salt + 16, userEntropy.constData(), 16);

    QString output;
    const QString password = ui->passwordLineEdit->text();

    const QByteArray passwordUtf8Bytes = password.toUtf8();
    const char* passwordUtf8 =  passwordUtf8Bytes.constData();
    const size_t passwordUtf8Length = strlen(passwordUtf8); // This is OK because according to Qt documentation, .constData() returns a NUL-terminated buffer.

    const int argon2_timeCost = ui->timeCostHorizontalSlider->value();
    const int argon2_memoryCostMiB = ui->memoryCostHorizontalSlider->value();
    const int argon2_parallelism = ui->parallelismHorizontalSlider->value();
    const int hashLength = ui->hashLengthHorizontalSlider->value();

    int r = argon2id_hash_encoded(static_cast<uint32_t>(argon2_timeCost), static_cast<uint32_t>(argon2_memoryCostMiB) * 1024, argon2_parallelism, passwordUtf8, passwordUtf8Length, salt, sizeof(salt), static_cast<size_t>(hashLength), encodedHash, sizeof(encodedHash));
    if (r != ARGON2_OK)
    {
        fprintf(stderr, "Argon2UI: argon2id failure! \"argon2id_hash_raw\" returned: %d\n", r);
        goto exit;
    }

    output = QString(encodedHash);
    ui->encodedHashTextEdit->setText(output);

    exit:
    busy = false;
}

