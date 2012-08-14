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

#include "chmthumbnail.h"

#include <QImage>
#include <QFileInfo>
#include <QStringList>
#include <QDebug>

CHMCreator *CHMCreator::pObject;

extern "C"
{
    KDE_EXPORT ThumbCreator *new_creator()
    {
        return new CHMCreator();
    }
}

CHMCreator::CHMCreator() : chm(NULL)
{
    pObject = this;
}

CHMCreator::~CHMCreator()
{
}

bool CHMCreator::create(const QString& path, int width, int height, QImage& img)
{
    chm = chm_open(qPrintable(path));

    if (chm == NULL)
    {
        qDebug() << "Error: failed to open" << path;
        exit(1);
    }

    qDebug() << "[thumbnailer-chm]";
    qDebug() << "Opening" << path;

    extractedFile = "";

    //search for the index file (*.hhc)
    if (!chm_enumerate(chm, CHM_ENUMERATE_ALL, indexCallBackWrapper, NULL))
        qDebug() << "Error: chm_enumerate failed";

    if (extractedFile.length() == 0)
        qDebug() <<  "Error: index not found";
    else
    {
        getCoverFileName();
        checkCoverFileName();
        qDebug() << "Analizing cover file name:" << coverFileName;

        //is an image or am html file?
        QString fileExt = QFileInfo(coverFileName).completeSuffix().toLower();
        if (fileExt == "htm" || fileExt == "html")
        {
            qDebug() << "it's an html document...";

            QString basePath;
            basePath = QFileInfo(coverFileName).path(); //saves html path to use later to retrieve the image (it has a relative path)

            chmUnitInfo info;
            if (extractFileFromChm(chm, &info, coverFileName.toUtf8().data()) == true)
            {
                QStringList ImagesUrlFromHtml; //to collect the src items from <img...> tags

                if (extractImagesUrlFromHtml(&ImagesUrlFromHtml) == true)
                {
                    for (int a = 0; a < ImagesUrlFromHtml.count(); ++a)
                    {
                        coverFileName = ImagesUrlFromHtml.at(a);
                        if (basePath != "/")
                            coverFileName = basePath + "/" + coverFileName;

                        checkCoverFileName();
                        qDebug() << coverFileName;

                        chmUnitInfo info;
                        if (extractImageFromChm(chm, &info, &coverFileName) == true)
                        {
                            //checks if the image has normal cover proportion (a way to avoid to select stupid button images contained in some chm file)
                            if (coverImage.width()/coverImage.height() < 1)
                            {
                                img = coverImage.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                                qDebug() << "...is the cover: DONE!";
                                break;
                            }
                        }
                    }
                }
                else
                    qDebug() << "Error: cover image not found";
            }
        }
        
        if (fileExt == "jpg" || fileExt == "png" || fileExt == "gif")
        {
            qDebug() << "it's an image...";

            chmUnitInfo info;
            if (extractImageFromChm(chm, &info, &coverFileName) == true)
            {
                img = coverImage.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                qDebug() << "...DONE!";
            }
        }
    }

    if (chm)
        chm_close(chm);
        
    qDebug() << "[/thumbnailer-chm]";

    return !img.isNull();
}

int CHMCreator::indexCallBackWrapper(struct chmFile *chm, struct chmUnitInfo *info, void *context)
{
    return pObject->indexCallBack(chm, info, context);
}

int CHMCreator::indexCallBack(struct chmFile *chm, struct chmUnitInfo *info, void *context)
{
    if (info->flags & CHM_ENUMERATE_FILES)
    {
        QString fileExt = QFileInfo(info->path).completeSuffix().toLower();
        if (fileExt == "hhc") //if it's the index...
        {
            qDebug() << "Found index:" << info->path;
            extractFileFromChm(chm, info);

            return CHM_ENUMERATOR_SUCCESS;
        }
    }
    
    return CHM_ENUMERATOR_CONTINUE;
}

bool CHMCreator::extractFileFromChm(struct chmFile *chm, struct chmUnitInfo *info, char *fileName)
{
    if (fileName == NULL)
        fileName = info->path;

    if (CHM_RESOLVE_SUCCESS == chm_resolve_object(chm, fileName, info))
    {
        unsigned char buffer[info->length];

        LONGINT64 gotLen = chm_retrieve_object(chm, info, buffer, 0, info->length);

        if (gotLen == 0)
        {
            qDebug() <<  "Error: index not retrieved (invalid filesize)";
        }
        else
        {
            extractedFile = QString((const char *)buffer); //to rethink
            return true;
        }
    }
    else
        qDebug() <<  "Error: index not retrieved (chm_resolve_object failed)";

    return false;
}

bool CHMCreator::extractImagesUrlFromHtml(QStringList *ImagesUrlFromHtml)
{
    // Why this solution? Because...
    // * html != xhml so I can't use Qt's XML classes
    // * I don't like the idea to use HTML Tidy Library to convert the html to xhtml
    // * I didn't find a c++ html parser and I don't want to add another library
    // * Regular expressions are a mess with so various htmls
    
    extractedFile = extractedFile.toLower(); //to avoid problems with "img", "IMG" and similar nice things

    int occurrences = extractedFile.count("<img");
    if (occurrences > 0)
    {
        int posBeginImg = 0;
        int posEndImg = 0;

        for (int a = occurrences; a > 0; --a)
        {
            posBeginImg = extractedFile.indexOf("<img", posBeginImg)+4;
            posEndImg = extractedFile.indexOf(">", posBeginImg);

            QString imgString;
            imgString = extractedFile.mid(posBeginImg, posEndImg - posBeginImg);
            qDebug() << "imgString:" << imgString;

            if (imgString.contains("src") == true)
            {
                int posBeginSrc = imgString.indexOf("src=")+4;
                int posEndSrc = imgString.indexOf(" ", posBeginSrc);

                QString imgSrc;
                imgSrc = imgString.mid(posBeginSrc, posEndSrc - posBeginSrc);
                imgSrc.remove('\"');
                imgSrc.remove('\'');
                qDebug() << "imgSrc:" << imgSrc << posBeginImg << posEndImg;
                ImagesUrlFromHtml->append(imgSrc);
            }

            posBeginImg = posEndImg;
        }

    return true;
    }

    return false;
}

bool CHMCreator::extractImageFromChm(struct chmFile *chm, struct chmUnitInfo *info, QString *imageName)
{
    if (CHM_RESOLVE_SUCCESS == chm_resolve_object(chm, imageName->toAscii(), info))
    {
        unsigned char buffer[info->length];

        LONGINT64 gotLen = chm_retrieve_object(chm, info, buffer, 0, info->length);

        if (gotLen == 0)
        {
            qDebug() <<  "Error: image not retrieved (invalid filesize)";
        }
        else
        {
            coverImage.loadFromData((const uchar*)buffer, (int)gotLen);
            qDebug() <<  "...image retrieved...";
            return true;
        }
    }
    else
        qDebug() <<  "Error: image not retrieved (chm_resolve_object failed)";

    return false;
}

void CHMCreator::getCoverFileName() //terrible...I know :(
{
    int posBegin = extractedFile.indexOf("<param name=\"Local\" value=\"", 0, Qt::CaseInsensitive);
    while (posBegin <= extractedFile.length())
    {
        if(posBegin != -1) //if something is found...
        {
            int posEnd = extractedFile.indexOf("\">", posBegin+27);
            coverFileName = extractedFile.mid(posBegin+27, posEnd - posBegin-27);
            qDebug() << "Found cover file name:" << coverFileName;
            if (coverFileName != "")
                break;
            else //tries to retrieve the next matching position
                posBegin = extractedFile.indexOf("<param name=\"Local\" value=\"", posBegin+1, Qt::CaseInsensitive);
        }
    }
}

void CHMCreator::checkCoverFileName()
{
    if (coverFileName.left(1) == ".")
        coverFileName.remove(0, 1);

    if (coverFileName.left(1) != "/")
        coverFileName.prepend("/");

    if (coverFileName.contains("%20") == true)
        coverFileName.replace("%20", " ");
}

ThumbCreator::Flags CHMCreator::flags() const
{
    return None;
}