#ifndef KFILE_PLUGIN_SPC_H
#define KFILE_PLUGIN_SPC_H

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

#include <kfilemetainfo.h>

class QStringList;

class spcPlugin : public KFilePlugin
{
    Q_OBJECT
    public:

    spcPlugin(QObject *parent, const char *name, const QStringList &args);

    virtual bool readInfo(KFileMetaInfo &info, uint what);
    virtual bool writeInfo(const KFileMetaInfo &info) const;

    virtual QValidator *createValidator(const QString &mimeType,
                                        const QString &group,
                                        const QString &key,
                                        QObject *parent,
                                        const char *name
                                       ) const;
};

#endif /* KFILE_PLUGIN_SPC_H */

// vim: set et ts=8 sw=4:
