#pragma once
#include <QFileInfo>
#include <QString>
#include <QStringList>
#include <algorithm>

// Telegram and its remakes for the desktop. The names come from the build files of the projects.
namespace te {
// Windows executables. Telegram.exe also covers Forkgram, exDesktop and Unigram.
inline bool isTelegramExecutable(const QString& name) {
    static const QStringList known{
        QStringLiteral("Telegram.exe"), QStringLiteral("AyuGram.exe"), QStringLiteral("64Gram.exe"),
        QStringLiteral("Kotatogram.exe"), QStringLiteral("materialgram.exe"), QStringLiteral("rabbitGram.exe"),
        QStringLiteral("freshGram.exe"), QStringLiteral("SereinGram.exe"), QStringLiteral("Forkgram.exe")};
    return known.contains(name, Qt::CaseInsensitive);
}
// macOS bundles. Any other client is recognised by the tg:// links it opens.
inline bool isTelegramBundle(const QString& bundle) {
    static const QStringList known{
        QStringLiteral("com.tdesktop.Telegram"), QStringLiteral("com.tdesktop.TelegramBeta"),
        QStringLiteral("org.telegram.desktop"), QStringLiteral("ru.keepcoder.Telegram"),
        QStringLiteral("one.ayugram.AyuGramDesktop"), QStringLiteral("io.github.tdesktop_x64.TDesktop"),
        QStringLiteral("io.github.kotatogram"), QStringLiteral("io.github.kukuruzka165.materialgram"),
        QStringLiteral("io.github.snowy-fluffy.freshgram"), QStringLiteral("io.github.eltavine.SereinGram")};
    return known.contains(bundle, Qt::CaseInsensitive);
}
// The program that the system starts for tg:// links, if it is built on Telegram Desktop:
// all of those register the command as "C:\...\Client.exe" -- "%1".
inline QString telegramLinkHandler(const QString& command) {
    const QString text = command.trimmed();
    if (!text.startsWith(u'"')) return {};
    const qsizetype quote = text.indexOf(u'"', 1);
    if (quote < 0 || !text.sliced(quote + 1).trimmed().startsWith(QLatin1String("--"))) return {};
    const QString path = text.sliced(1, quote - 1);
    return path.endsWith(QLatin1String(".exe"), Qt::CaseInsensitive)
        ? path.sliced(std::max(path.lastIndexOf(u'\\'), path.lastIndexOf(u'/')) + 1) : QString();
}
}
