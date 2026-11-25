#pragma once
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTemporaryDir>
#include <QFile>
#include <QCryptographicHash>
#include <QObject>
#ifdef Q_OS_MACOS
#include <QProcess>
#endif
#include <memory>

namespace te {
class ReleaseUpdater final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(QString notes READ notes NOTIFY changed)
    Q_PROPERTY(bool busy READ busy NOTIFY changed)
    Q_PROPERTY(bool needsToken READ needsToken NOTIFY changed)
    Q_PROPERTY(bool available READ available NOTIFY changed)
public:
    explicit ReleaseUpdater(QObject* parent = nullptr);
    QString message() const { return message_; }
    QString notes() const { return notes_; }
    bool busy() const { return busy_; }
    bool needsToken() const { return needsToken_; }
    bool available() const { return asset_.isValid() && !busy_; }
    Q_INVOKABLE void check(const QString& token);
    Q_INVOKABLE void install();
signals:
    void changed();
private:
    void finish(const QString& message);
    void download(const QUrl& url, int redirects);
    void launchInstaller();
#ifdef Q_OS_MACOS
    void finishMacPreparation(int exitCode, QProcess::ExitStatus status);
#endif
    QNetworkRequest request(const QUrl& url, const QByteArray& accept) const;
    QNetworkAccessManager network_;
    QString message_, notes_, token_;
    QUrl asset_;
    QByteArray digest_;
    qint64 expected_ = 0;
    bool busy_ = false, needsToken_ = false;
    std::unique_ptr<QTemporaryDir> stage_;
    QFile package_;
    QCryptographicHash hash_{QCryptographicHash::Sha256};
#ifdef Q_OS_MACOS
    std::unique_ptr<QTemporaryDir> updateWorkspace_;
    QProcess prepare_;
#endif
};
}
