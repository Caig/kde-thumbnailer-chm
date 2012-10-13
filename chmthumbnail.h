/*
This file is part of kde-thumbnailer-chm
Copyright (C) 2012 Caig <giacomosrv@gmail.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef CHMTHUMBNAIL_H
#define CHMTHUMBNAIL_H

#include <QObject>
#include <QImage>
#include <kio/thumbcreator.h>

#include <chm_lib.h>

class CHMCreator : public QObject, public ThumbCreator
{
    Q_OBJECT
     
    public:
        explicit CHMCreator();
        virtual ~CHMCreator();
        virtual bool create(const QString& path, int width, int height, QImage& img);
        virtual Flags flags() const;
        
    private:
        chmFile *chm;

        int indexCallBack(struct chmFile *chm, struct chmUnitInfo *info, void *context);
        static int indexCallBackWrapper(struct chmFile *chm, struct chmUnitInfo *info, void *context);
        static CHMCreator *pObject;
        
        bool extractFileFromChm(struct chmFile *chm, struct chmUnitInfo *info, char *fileName = NULL);
        bool extractImageFromChm(struct chmFile *chm, struct chmUnitInfo *info, QString *imageName);
        
        void getCoverFileName(int indexType);
        void checkCoverFileName();
        bool extractImagesUrlFromHtml(QStringList *ImagesUrlFromHtml);

        QByteArray extractedFileByte; //the content of a file retrieved by extractFileFromChm (raw bytes, for #URLSTR files)
        QString extractedFileString; //the content of a file retrieved by extractFileFromChm (for .hhc and html files)
        QString coverFileName; //usually it's a htm document, sometimes directly the cover image
        QImage coverImage;
};

#endif // CHMTHUMBNAIL_H
