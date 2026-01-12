#include <UI/Dialogs/DialogUploader.h>
#include <Workers/UploaderWorker.h>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileInfo>

DialogUploader::DialogUploader(const QUrl &uploadUrl, const QString &otp, const QByteArray &data, QWidget *parent) : QDialog(parent)
{
    setWindowTitle("上传中…");
    resize(400, 150);
    this->setProperty("Main", "base");

    progressBar   = new QProgressBar(this);
    progressBar->setRange(0, 100);
    statusLabel   = new QLabel("准备上传…", this);
    speedLabel    = new QLabel("速度：0 KB/s", this);
    cancelButton  = new QPushButton("取消", this);

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(statusLabel);
    layout->addWidget(progressBar);
    layout->addWidget(speedLabel);
    layout->addWidget(cancelButton);
    setLayout(layout);

    workerThread = new QThread(this);
    worker = new UploaderWorker(uploadUrl, otp, data);
    worker->moveToThread(workerThread);

    connect(workerThread, &QThread::started,     worker, &UploaderWorker::start);
    connect(cancelButton, &QPushButton::clicked, worker, &UploaderWorker::cancel);

    connect(worker, &UploaderWorker::progress, this, [this](const qint64 sent, const qint64 total) {
        int percent = total > 0 ? static_cast<int>((sent * 100) / total) : 0;
        progressBar->setValue(percent);
        statusLabel->setText(QString("已上传：%1 / %2 MB").arg(sent / (1024 * 1024)).arg(total / (1024 * 1024)));
    });

    connect(worker, &UploaderWorker::speedUpdated, this, [this](const double kbps) {
        speedLabel->setText(QString("速度：%1 KB/s").arg(kbps, 0, 'f', 2));
    });

    connect(worker, &UploaderWorker::finished, this, [this]() {
        if (!worker->IsError()) {
            progressBar->setValue(100);
            statusLabel->setText("上传完成");
            speedLabel->setVisible(false);
            this->accept();
        }
        this->reject();
    });

    connect(worker, &UploaderWorker::failed, this, [this](const QString &msg) {
        QMessageBox::critical(this, "上传错误", msg);
        this->reject();
    });

    connect(workerThread, &QThread::finished, worker,       &QObject::deleteLater);
    connect(workerThread, &QThread::finished, workerThread, &QObject::deleteLater);

    workerThread->start();
}

DialogUploader::~DialogUploader()
{
    if (workerThread && workerThread->isRunning()) {
        worker->cancel();
        workerThread->quit();
        workerThread->wait();
    }
}
