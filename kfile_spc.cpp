/*
 * kfile_spc - A KFileMetaInfo plugin to read/write SNES SPC files.
 * Copyright (c) 2004 Michael Pyne <michael.pyne@kdemail.net>
 *
 * Ideas taken from SNESAmp http://www.alpha-ii.com/Source/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <kgenericfactory.h>
#include <kstringvalidator.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/netaccess.h>

#include <qvalidator.h>
#include <qfile.h>
#include <qwidget.h>
#include <qcstring.h>
#include <qstringlist.h>
#include <qdatetime.h>
#include <qregexp.h>

#include <cstdlib>

#include "kfile_spc.h"

// Not used, but will be here in case people wonder.
const char * const version = "kfile_spc version 0.5.1";

using KIO::NetAccess;

typedef KGenericFactory<spcPlugin> spcFactory;

K_EXPORT_COMPONENT_FACTORY(kfile_spc, spcFactory("kfile_spc"));

// This string (or at least the non-version part) should be at the beginning
// of every valid SPC file.
const char *const SPCMagic = "SNES-SPC700 Sound File Data v0.30";

// Assume little-endian for all non-char non-string types
typedef struct {
    char spcMagic[33]; // Header
    char spcMagicTerminator[3];
    char version;
    char spcRegister[7];
    Q_INT16 __reserved1;
    char title[32];
    char game[32];
    char dumper[16];
    char comments[32];
    char dumpDate[11];
    char length[3];
    char fadeLength[5]; // In milliseconds
    char artist[32];
    char defaultChannels;
    char emulator;
    char __reserved2[45];
} __attribute__((packed)) spcTextTagBlock;

// Assume little-endian for all non-char non-string types
typedef struct {
    char spcMagic[33]; // Header
    char spcMagicTerminator[3];
    char version;
    char spcRegister[7];
    Q_INT16 __reserved1;
    char title[32];
    char game[32];
    char dumper[16];
    char comments[32];

    // Different from text tag starting here.
    Q_UINT16 dumpDateYear;
    Q_UINT8 dumpDateMonth;
    Q_UINT8 dumpDateDay;
    char __unused[7];
    Q_UINT16 length;
    char __unused2;
    Q_UINT32 fadeLength; // In milliseconds
    char artist[32];
    char defaultChannels;
    char emulator;
    char __reserved2[46];
} __attribute__((packed)) spcBinaryTagBlock;

/*
 * Used to limit the string length of some of the various tags.
 */
class MaxLengthValidator : public QValidator
{
    public:
    MaxLengthValidator(uint max, QObject *parent = 0, const char *name = 0)
     : QValidator(parent, name), m_max(max)
    {
    }

    virtual State validate(QString &input, int &pos) const
    {
        if(input.length() > m_max) {
            pos = m_max;
            return Invalid;
        }

        return Acceptable;
    }

    private:
    uint m_max;
};

/*
 * The plugin class.
 */
spcPlugin::spcPlugin(QObject *parent, const char *name, const QStringList &args)
 : KFilePlugin(parent, name, args)
{
    KFileMimeTypeInfo *info = addMimeTypeInfo("audio/x-spc");
    KFileMimeTypeInfo::GroupInfo *group = 0L;

    group = addGroupInfo(info, "spcInfo", i18n("SPC Information"));

    KFileMimeTypeInfo::ItemInfo *item = 0L;

    item = addItemInfo(group, "Title", i18n("Title"), QVariant::String);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);

    item = addItemInfo(group, "Game", i18n("Game"), QVariant::String);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);

    item = addItemInfo(group, "Artist", i18n("Composer"), QVariant::String);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);

    item = addItemInfo(group, "Dumped by", i18n("Dumped From Game By"), QVariant::String);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);

    item = addItemInfo(group, "Program", i18n("Dumped With"), QVariant::String);

    item = addItemInfo(group, "Comments", i18n("Comments"), QVariant::String);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);

    item = addItemInfo(group, "Date", i18n("Date"), QVariant::Date);
    setAttributes(item, KFileMimeTypeInfo::Modifiable | KFileMimeTypeInfo::Addable);

    item = addItemInfo(group, "Length", i18n("Song Length"), QVariant::UInt);
    setAttributes(item, KFileMimeTypeInfo::Modifiable); // Can be modified, believe it or not
    setHint(item, KFileMimeTypeInfo::Length);
    setUnit(item, KFileMimeTypeInfo::Seconds);

    item = addItemInfo(group, "Fadeout", i18n("Fadeout Time"), QVariant::Double);
    setAttributes(item, KFileMimeTypeInfo::Modifiable);
    setHint(item, KFileMimeTypeInfo::Length);
    setUnit(item, KFileMimeTypeInfo::Seconds);
}

/*
 * Returns true if the block of memory pointed to by s (and l bytes long)
 * appears to be textual.  Every byte in the block is checked, even if a \0
 * is encountered.
 */
bool isText(const char *s, int l)
{
    int i = 0;

    while(i < l && (s[i] == 0 || (s[i] >= 0x30 && s[i] <= 0x39) || s[i] == '/'))
        ++i;

    if(s[i] == 0 || i == l)
        return true;
    return false;
}

/*
 * Returns true if the block of memory pointed to by s (and l bytes long)
 * appears to have been set (i.e., there is a non-zero value in the block
 * somewhere.)
 */
bool isSet(char *s, int l)
{
    for(int i = 0; i < l; ++i)
        if(s[i])
            return true;
    return false;
}

bool spcPlugin::readInfo(KFileMetaInfo &info, uint what)
{
    char strBuf[34] = { 0 };
    QString fileName;
    QWidget *parentWidget = 0;
    bool isTextTag = false;

    // Union used to alias the two variables to the same area of memory.
    union {
        spcTextTagBlock tag;
        spcBinaryTagBlock binaryTag;
    } u;
    
    if(parent()->isWidgetType())
        parentWidget = static_cast<QWidget *>(parent());

    if(!NetAccess::download(info.url(), fileName, parentWidget)) {
        kdDebug() << "Unable to download file" << endl;
        return false;
    }

    QFile f(fileName);
    if(!f.open(IO_ReadOnly)) {
        kdDebug() << "Can't open file" << endl;
        NetAccess::removeTempFile(fileName);
        return false;
    }

    // Unlink this now, we won't need it later.
    NetAccess::removeTempFile(fileName);

    if(f.readBlock((char*) (&u.tag), sizeof(spcTextTagBlock)) == -1) {
        kdDebug() << "Can't read header block from file" << endl;
        return false;
    }

    memcpy(strBuf, u.tag.spcMagic, 33);
    strBuf[33] = 0;
    if(!QString(strBuf).startsWith("SNES-SPC700 Sound File Data") || 
       u.tag.spcMagicTerminator[2] != 26)
    {
        if(u.tag.spcMagicTerminator[2] == 26)
            kdDebug() << "This isn't an SNES SPC file" << endl;
        else
            kdDebug() << "This file has no ID666 tag" << endl;
        return false;
    }

    KFileMetaInfoGroup group = appendGroup(info, "spcInfo");

    // TODO: Find out how i18n is handled with ID666.  Probably there is no
    // set standard.
    memcpy(strBuf, u.tag.title, 32);
    appendItem(group, "Title", QString(strBuf));

    memcpy(strBuf, u.tag.game, 32);
    appendItem(group, "Game", QString(strBuf));

    memcpy(strBuf, u.tag.dumper, 16);
    strBuf[16] = 0;
    appendItem(group, "Dumped by", QString(strBuf));

    memcpy(strBuf, u.tag.comments, 32);
    appendItem(group, "Comments", QString(strBuf));

    // The offsets to the various information is now dependant upon whether
    // this is a text or binary tag.
    if(isSet(u.tag.length, 3)) {
        isTextTag = isText(u.tag.length, 3);
        kdDebug() << "Length is text? " << isTextTag << endl;
    }
    else if(isSet(u.tag.dumpDate, 6)) {
        isTextTag = isText(u.tag.dumpDate, 6);
        kdDebug() << "Date is text? " << isTextTag << endl;
    }

    if(isTextTag) {
        memcpy(strBuf, u.tag.artist, 32);
        appendItem(group, "Artist", QString(strBuf));

        if(u.tag.emulator == 1)
            appendItem(group, "Program", "zSNES");
        else if(u.tag.emulator == 2)
            appendItem(group, "Program", "Snes9x");
        else
            appendItem(group, "Program", i18n("Unknown"));

        memcpy(strBuf, u.tag.length, 3);
        strBuf[3] = 0;
        appendItem(group, "Length", QCString(strBuf).toUInt());

        int d, y, m;
        memcpy(strBuf, u.tag.dumpDate, 2);
        strBuf[2] = 0;
        m = QString(strBuf).toInt();

        memcpy(strBuf, u.tag.dumpDate + 3, 2);
        d = QString(strBuf).toInt();

        memcpy(strBuf, u.tag.dumpDate + 6, 4);
        y = QString(strBuf).toInt();

        if(m && d && y)
            appendItem(group, "Date", QDate(y, m, d));

        memcpy(strBuf, u.tag.fadeLength, 5);
        strBuf[5] = 0;
        appendItem(group, "Fadeout", QCString(strBuf).toUInt() / 1000.0);
    }
    else {
        kdDebug() << "Reading binary tag.\n";
        memcpy(strBuf, u.binaryTag.artist, 32);
        appendItem(group, "Artist", QString(strBuf));

        if(u.binaryTag.emulator == 1)
            appendItem(group, "Program", "zSNES");
        else if(u.binaryTag.emulator == 2)
            appendItem(group, "Program", "Snes9x");
        else
            appendItem(group, "Program", i18n("Unknown"));

        appendItem(group, "Length", u.binaryTag.length);
        appendItem(group, "Fadeout", u.binaryTag.fadeLength / 1000.0);

        int y = u.binaryTag.dumpDateYear;
        int m = u.binaryTag.dumpDateMonth;
        int d = u.binaryTag.dumpDateDay;
        if(m && d && y)
            appendItem(group, "Date", QDate(y, m, d));
    }

    return true;
}

bool spcPlugin::writeInfo(const KFileMetaInfo &info) const
{
    spcBinaryTagBlock dataBlock = { 0 };

    QCString title, game, dumper, comments, composer;
    QDate date;

    // Open the file and read in the bytes in order to avoid destroying
    // the header information.
    QFile f(info.path());
    if(!f.open(IO_ReadWrite)) {
        kdError() << "Unable to open " << info.path() << endl;
        return false;
    }

    f.at((char*) &dataBlock.spcRegister - (char*) &dataBlock);
    if(f.readBlock((char*) &dataBlock.spcRegister, sizeof dataBlock.spcRegister) == -1) {
        kdError() << "Unable to read from " << info.path() << endl;
        return false;
    }

    f.reset();

    // Instead of worrying about modifications, just get the values
    // and write them all out at once.
 
    const KFileMetaInfoGroup &group(info["spcInfo"]);

    title = group["Title"].value().toCString().left(31);
    game = group["Game"].value().toCString().left(31);
    dumper = group["Dumped by"].value().toCString().left(15);
    comments = group["Comments"].value().toCString().left(31);
    composer = group["Artist"].value().toCString().left(31);
    date = group["Date"].value().toDate();
    QString proggie = group["Program"].value().toString();

    qstrcpy(dataBlock.title, title);
    qstrcpy(dataBlock.game, game);
    qstrcpy(dataBlock.dumper, dumper);
    qstrcpy(dataBlock.comments, comments);
    qstrcpy(dataBlock.artist, composer);

    dataBlock.length = group["Length"].value().toUInt();
    dataBlock.__unused2 = 0;
    dataBlock.fadeLength = (Q_INT32) (group["Fadeout"].value().toDouble() * 1000);

    // I'm taking a guess as to the Date format here.
    if(group["Date"].isValid()) {
        dataBlock.dumpDateYear = (Q_UINT16) date.year();
        dataBlock.dumpDateMonth = (Q_UINT8) date.month();
        dataBlock.dumpDateDay = (Q_UINT8) date.day();
    }

    if(proggie == "zSNES")
        dataBlock.emulator = 1;
    else if(proggie == "Snes9x")
        dataBlock.emulator = 2;

    qstrcpy(dataBlock.spcMagic, "SNES-SPC700 Sound File Data v0.30");
    dataBlock.spcMagicTerminator[0] = 26;
    dataBlock.spcMagicTerminator[1] = 26;
    dataBlock.spcMagicTerminator[2] = 26;
    dataBlock.version = 30;

    // The rest of the dataBlock is identical to how it was earlier.

    f.writeBlock((char*) &dataBlock, sizeof dataBlock);
    f.close();

    return true;
}

QValidator *spcPlugin::createValidator(const QString &mimeType,
                                       const QString &group,
                                       const QString &key,
                                       QObject *parent,
                                       const char *name
                                      ) const
{
    if(key=="Fadeout")
        return new QDoubleValidator(0.0, 5.0, 2, parent, name);

    if(key=="Length")
        return new QIntValidator(0, 65535, parent, name);

    if(key=="Dumped by")
        return new MaxLengthValidator(15, parent, name);

    if(key=="Program") {
        QStringList programs;
        programs << "Unknown" << "zSNES" << "Snes9x";
        return new KStringListValidator(programs, false, false, parent, name);
    }

    if(key!="Date")
        return new MaxLengthValidator(31, parent, name);

    return KFilePlugin::createValidator(mimeType, group, key, parent, name);
}

#include "kfile_spc.moc"

// vim: set et ts=8 sw=4:
