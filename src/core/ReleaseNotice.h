#pragma once
#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QTimer>
#include <QUrl>

namespace te {
// Asks the site which version it serves and reports a newer one. The request carries no data about
// the computer or the user; a failure is silent and the next attempt comes with the timer.
class ReleaseNotice final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString version READ version NOTIFY changed)
    Q_PROPERTY(QString current READ current CONSTANT)
    Q_PROPERTY(QUrl page READ page CONSTANT)
public:
    explicit ReleaseNotice(QObject* parent = nullptr, const QUrl& endpoint = defaultEndpoint());
    QString version() const { return dismissed_ == version_ ? QString() : version_; }
    QString current() const;
    QUrl page() const { return QUrl(QStringLiteral("https://tfx-site-ashy.vercel.app/#download")); }
    void start();
    Q_INVOKABLE void refresh();   // no more than once in half an hour
    Q_INVOKABLE void dismiss();   // until the program is started again
    // The version in the answer of the site when it is newer than the installed one.
    static QString newer(const QByteArray& answer, const QString& platform, const QString& installed);
    static QUrl defaultEndpoint() { return QUrl(QStringLiteral("https://tfx-site-ashy.vercel.app/api/releases")); }
signals:
    void changed();
    void checked();
private:
    void check();
    QNetworkAccessManager network_;
    QUrl endpoint_;
    QTimer timer_;
    QElapsedTimer last_;
    QString version_, dismissed_;
    bool busy_ = false, started_ = false;
};
}
