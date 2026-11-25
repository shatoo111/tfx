#include "core/ReleaseNotice.h"
#include "core/ReleaseUpdater.h"
#include "core/TelegramClients.h"
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QtTest>

class UpdateTests final : public QObject {
    Q_OBJECT
private slots:
    void comparesVersions() {
        const QByteArray answer = "{\"mac\":{\"version\":\"0.5.24\",\"size\":1},\"win\":{\"version\":\"0.5.19\"}}";
        QCOMPARE(te::ReleaseNotice::newer(answer, "mac", "0.5.23"), QStringLiteral("0.5.24"));
        QCOMPARE(te::ReleaseNotice::newer(answer, "mac", "0.5.9"), QStringLiteral("0.5.24"));
        QCOMPARE(te::ReleaseNotice::newer(answer, "mac", "0.5.24"), QString());
        QCOMPARE(te::ReleaseNotice::newer(answer, "mac", "0.6.0"), QString());
        QCOMPARE(te::ReleaseNotice::newer(answer, "win", "0.5.19"), QString());
        QCOMPARE(te::ReleaseNotice::newer(answer, "linux", "0.5.19"), QString());
        for (const QByteArray& bad : {QByteArray(), QByteArray("<html>"), QByteArray("{\"mac\":\"0.9.0\"}"),
                 QByteArray("{\"mac\":{\"version\":\"9.9.9<b>\"}}"), QByteArray("{\"mac\":{\"version\":\"99999.0.0\"}}")})
            QCOMPARE(te::ReleaseNotice::newer(bad, "mac", "0.5.23"), QString());
    }
    void noticesNewVersion() {
        QTcpServer server;
        if (!server.listen(QHostAddress::LocalHost)) QSKIP("Local ports are not available here");
        QByteArray body = "{\"mac\":{\"version\":\"0.5.25\"},\"win\":{\"version\":\"0.5.25\"}}", asked;
        connect(&server, &QTcpServer::newConnection, &server, [&] {
            auto* socket = server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, socket, [&, socket] {
                asked += socket->readAll();
                socket->write("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\nContent-Length: " + QByteArray::number(body.size()) + "\r\n\r\n" + body);
                socket->disconnectFromHost();
            });
        });
        QCoreApplication::setApplicationVersion("0.5.24-preview");
        te::ReleaseNotice notice(nullptr, QUrl(QStringLiteral("http://127.0.0.1:%1/api/releases").arg(server.serverPort())));
        QCOMPARE(notice.current(), QStringLiteral("0.5.24"));
        QSignalSpy changed(&notice, &te::ReleaseNotice::changed);
        notice.start();
        QTRY_COMPARE_WITH_TIMEOUT(notice.version(), QStringLiteral("0.5.25"), 9000);
        QVERIFY(asked.startsWith("GET /api/releases HTTP/1.1"));
        QVERIFY(!asked.contains("Cookie") && !asked.contains("0.5.24"));
        QCOMPARE(changed.size(), 1);
        notice.dismiss();
        QCOMPARE(notice.version(), QString());
        QCOMPARE(changed.size(), 2);
    }
    void asksTheSite() {
        if (!qEnvironmentVariableIsSet("TFX_TEST_LIVE_SITE")) QSKIP("Set TFX_TEST_LIVE_SITE to ask the real site");
        QCoreApplication::setApplicationVersion("0.1.0");
        te::ReleaseNotice notice;
        notice.start();
        QTRY_VERIFY_WITH_TIMEOUT(!notice.version().isEmpty(), 25000);
        qInfo("The site serves %s", qPrintable(notice.version()));
    }
    void knowsTelegramClients() {
        for (const char* name : {"Telegram.exe", "AyuGram.exe", "ayugram.EXE", "64Gram.exe", "Kotatogram.exe", "materialgram.exe"})
            QVERIFY2(te::isTelegramExecutable(QString::fromLatin1(name)), name);
        for (const char* name : {"", "chrome.exe", "NotTelegram.exe", "Telegram"})
            QVERIFY2(!te::isTelegramExecutable(QString::fromLatin1(name)), name);
        QVERIFY(te::isTelegramBundle("one.ayugram.AyuGramDesktop") && te::isTelegramBundle("ru.keepcoder.Telegram"));
        QVERIFY(!te::isTelegramBundle("com.apple.Safari"));
        QCOMPARE(te::telegramLinkHandler("\"C:\\Users\\me\\AppData\\Roaming\\My Gram\\MyGram.exe\" -- \"%1\""), QStringLiteral("MyGram.exe"));
        for (const char* other : {"\"C:\\Program Files\\Browser\\browser.exe\" \"%1\"", "C:\\a.exe -- \"%1\"", "", "\"broken", "\"C:\\x\\run.bat\" -- \"%1\""})
            QCOMPARE(te::telegramLinkHandler(QString::fromLatin1(other)), QString());
    }
    void privateReleaseCheck() {
        const auto token = qEnvironmentVariable("TFX_TEST_RELEASE_READ");
        if (token.isEmpty()) QSKIP("Requires read access to the project's private releases");
        QCoreApplication::setApplicationVersion("9999.0.0");
        te::ReleaseUpdater updater;
        updater.check({});
        QTRY_VERIFY_WITH_TIMEOUT(!updater.busy(), 50000);
        QVERIFY(updater.needsToken() || updater.message().contains("9999.0.0"));
        QVERIFY(!updater.available());
        updater.check(token);
        QTRY_VERIFY_WITH_TIMEOUT(!updater.busy(), 50000);
        QVERIFY2(!updater.needsToken(), qPrintable(updater.message()));
        QVERIFY2(updater.message().contains("9999.0.0"), qPrintable(updater.message()));
        QVERIFY(!updater.notes().isEmpty());
        QVERIFY(!updater.available());
    }
};
QTEST_GUILESS_MAIN(UpdateTests)
#include "update_tests.moc"
