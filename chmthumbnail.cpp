/*
This file is part of kde-thumbnailer-chm
Copyright (C) 2012-2016 Giacomo Barazzetti <giacomosrv@gmail.com>

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

#include <QtGui/QImage>
#include <QtCore/QFileInfo>
#include <QtCore/QStringList>
#include <QtCore/QDebug>

CHMCreator *CHMCreator::pObject;

enum EIndexType
{
    NO_INDEX,     // chm index not found
    HHC_INDEX,    // *.hhc index found
    URLSTR_INDEX  // #URLSTR index found
};

struct Scontext
{
    bool searchForHhc; // true to search for *.hhc, false for #URLSTR
};

extern "C"
{
    Q_DECL_EXPORT ThumbCreator *new_creator()
    {
        return new CHMCreator();
    }
}

CHMCreator::CHMCreator() : chm(NULL)
{
    pObject = this;
}

bool CHMCreator::create(const QString &path, int width, int height, QImage &img)
{
    chm = chm_open(qPrintable(path));

    qDebug() << "[thumbnailer-chm]" << "Opening" << path;
    
    if (chm == NULL){
        qDebug() << "[thumbnailer-chm]" << "Error: unable to open the file.";
        exit(1);
    }

    // the same instance of the thumbnailer plugin is used
    // for all the CHMs in a directory, so better to clean
    // some important variables every time...
    extractedFileByte = "";
    extractedFileString = "";
    coverFileName = "";

    // search for the index file *.hhc or #URLSTR
    int indexSearchResult = NO_INDEX;
    
    // search for *.hhc file
    Scontext searchContext;
    searchContext.searchForHhc = true;
    chm_enumerate(chm, CHM_ENUMERATE_ALL, indexCallBackWrapper, (void *)&searchContext);

    if (extractedFileByte.length() != 0) {
        indexSearchResult = HHC_INDEX;
    } else {
        qDebug() << "[thumbnailer-chm]" << "Index (*.hhc) not found";

        // search for #URLSTR file
        searchContext.searchForHhc = false;
        chm_enumerate(chm, CHM_ENUMERATE_ALL, indexCallBackWrapper, (void *)&searchContext);
        if (extractedFileByte.length() != 0) {
            indexSearchResult = URLSTR_INDEX;
        } else {
            qDebug() << "[thumbnailer-chm]" << "Index (#URLSTR) not found";
        }
    }        

    if (indexSearchResult != NO_INDEX) {
        // an #URLSTR file should be something like this:
        //
        // °°°°°°°°°1.htm°°°°°°°°°2.htm°°°°°°°°°3.htm° [...]
        //
        // where '°' represents a '\0' character.
        // The '\0' characters don't allow to convert from QByteArray to QString
        // ( see QString::QString (const QByteArray & ba) ) successfully.
        // So better to replace '\0' with another character.
        if (indexSearchResult == URLSTR_INDEX) {
            extractedFileByte.replace('\0', '\a');
        }

        extractedFileString = QString(extractedFileByte);

        getCoverFileName(indexSearchResult);
        
        checkCoverFileName();
        
        qDebug() << "[thumbnailer-chm]" << "Analizing:" << coverFileName;

        // it's an image or an html file?
        QString fileExt = QFileInfo(coverFileName).suffix().toLower();
        
        if (fileExt == "htm" || fileExt == "html") {
            qDebug() << "[thumbnailer-chm]" << "It's an html document...";

            QString basePath;
            basePath = QFileInfo(coverFileName).path(); // saves html path to use later to retrieve the image (it has a relative path)

            chmUnitInfo info;
            if (extractFileFromChm(chm, &info, coverFileName.toUtf8().data()) == true) {
                extractedFileString = QString(extractedFileByte);
                
                // collects the src items from <img...> tags and tests the retrieved images
                QStringList ImagesUrlFromHtml;
                if (extractImagesUrlFromHtml(&ImagesUrlFromHtml) == false) {
                    qDebug() << "[thumbnailer-chm]" << "Cover image not found";
                } else {
                    for (int a = 0; a < ImagesUrlFromHtml.count(); ++a) {
                        coverFileName = ImagesUrlFromHtml.at(a);
                        if (basePath != "/") {
                            coverFileName = basePath + "/" + coverFileName;
                        }

                        checkCoverFileName();
                        qDebug() << "[thumbnailer-chm]" << "Testing the image" << coverFileName;

                        chmUnitInfo info;
                        if (extractImageFromChm(chm, &info, &coverFileName) == true) {
                            // checks if the image has normal* cover ratio
                            // (a way to avoid to select stupid button images
                            // seen in some chm files).
                            //
                            // *Usually a cover has height > width and a button
                            // has width > height.
                            if (coverImage.width()/coverImage.height() < 1) {
                                img = coverImage.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                                qDebug() << "[thumbnailer-chm]" << "...is the cover: DONE!";
                                break;
                            } else {
                                qDebug() << "[thumbnailer-chm]" << "...isn't the cover.";
                            }
                        }
                    }
                }
            }
        } // end of the html case procedure
        
        if (fileExt == "jpg" || fileExt == "jpeg" || fileExt == "png" || fileExt == "gif") {
            qDebug() << "[thumbnailer-chm]" << "It's an image...";

            chmUnitInfo info;
            if (extractImageFromChm(chm, &info, &coverFileName) == true) {
                img = coverImage.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                qDebug() << "[thumbnailer-chm]" << "...DONE!";
            }
        }
    }

    if (chm) {
        chm_close(chm);
    }

    return !img.isNull();
}

int CHMCreator::indexCallBackWrapper(struct chmFile *chm, struct chmUnitInfo *info, void *context)
{
    return pObject->indexCallBack(chm, info, context);
}

int CHMCreator::indexCallBack(struct chmFile *chm, struct chmUnitInfo *info, void *context)
{
    Scontext *tContext = (Scontext *)context;
    
    if (info->flags & CHM_ENUMERATE_FILES) {
        if (tContext->searchForHhc == true) {
            QString fileExt = QFileInfo(info->path).suffix().toLower();
            if (fileExt == "hhc") {
                qDebug() << "[thumbnailer-chm]" << "Found index (*.hhc):" << info->path;
                extractFileFromChm(chm, info);

                return CHM_ENUMERATOR_SUCCESS;
            }
        } else {
            if (QString::fromLatin1(info->path) == "/#URLSTR") {
                qDebug() << "[thumbnailer-chm]" << "Found index #URLSTR.";
                extractFileFromChm(chm, info);

                return CHM_ENUMERATOR_SUCCESS;
            }
        }
    }
    
    return CHM_ENUMERATOR_CONTINUE;
}

bool CHMCreator::extractFileFromChm(struct chmFile *chm, struct chmUnitInfo *info, char *fileName)
{
    if (fileName == NULL) {
        fileName = info->path;
    }

    if (CHM_RESOLVE_SUCCESS == chm_resolve_object(chm, fileName, info)) {
        //unsigned char buffer[info->length];
        extractedFileByte.resize(info->length);

        LONGINT64 gotLen = chm_retrieve_object(chm, info, (unsigned char*)extractedFileByte.data(), 0, info->length);

        if (gotLen == 0) {
            qDebug() << "[thumbnailer-chm]" <<  "Error: index not retrieved (invalid filesize)";
        } else {
            return true;
        }
    } else {
        qDebug() << "[thumbnailer-chm]" <<  "Error: index not retrieved (chm_resolve_object failed)";
    }

    return false;
}

bool CHMCreator::extractImagesUrlFromHtml(QStringList *ImagesUrlFromHtml)
{
    // Why this solution? Because...
    // * html != xhtml so I can't use Qt's XML classes
    // * I don't like the idea to use HTML Tidy Library to convert the html to xhtml
    // * I didn't find a c++ html parser and I don't want to add another library
    // * Regular expressions are a mess with so various htmls
    
    extractedFileString = extractedFileString.toLower(); // to avoid problems with "img", "IMG" and similar nice things

    int occurrences = extractedFileString.count("<img");
    if (occurrences > 0) {
        int posBeginImg = 0;
        int posEndImg = 0;

        for (int a = occurrences; a > 0; --a) {
            posBeginImg = extractedFileString.indexOf("<img", posBeginImg)+4;
            posEndImg = extractedFileString.indexOf(">", posBeginImg);

            QString imgString;
            imgString = extractedFileString.mid(posBeginImg, posEndImg - posBeginImg);

            if (imgString.contains("src") == true) {
                int posBeginSrc = imgString.indexOf("src=")+4;
                int posEndSrc = imgString.indexOf(" ", posBeginSrc);

                QString imgSrc;
                imgSrc = imgString.mid(posBeginSrc, posEndSrc - posBeginSrc);
                imgSrc.remove('\"');
                imgSrc.remove('\'');
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
    if (CHM_RESOLVE_SUCCESS == chm_resolve_object(chm, imageName->toLatin1(), info)) {
        unsigned char buffer[info->length];

        LONGINT64 gotLen = chm_retrieve_object(chm, info, buffer, 0, info->length);

        if (gotLen == 0) {
            qDebug() << "[thumbnailer-chm]" << "Error: image not retrieved (invalid filesize)";
        } else {
            coverImage.loadFromData((const uchar*)buffer, (int)gotLen);
            qDebug() << "[thumbnailer-chm]" <<  "...image retrieved...";
            return true;
        }
    } else {
        qDebug() << "[thumbnailer-chm]" << "Error: image not retrieved (chm_resolve_object failed)";
    }

    return false;
}

void CHMCreator::getCoverFileName(int indexType)
{
    if (indexType == HHC_INDEX) { // terrible...I know :(
        int posBegin = extractedFileString.indexOf("<param name=\"Local\" value=\"", 0, Qt::CaseInsensitive);
        while (posBegin <= extractedFileString.length()) {
            if (posBegin != -1) { // if something is found...
                int posEnd = extractedFileString.indexOf("\">", posBegin+27);
                coverFileName = extractedFileString.mid(posBegin+27, posEnd - posBegin-27);
                qDebug() << "[thumbnailer-chm]" << "Found cover file name:" << coverFileName;
                if (coverFileName != "") {
                    break;
                } else { //tries to retrieve the next matching position
                    posBegin = extractedFileString.indexOf("<param name=\"Local\" value=\"", posBegin+1, Qt::CaseInsensitive);
                }
            } else {
                break;
            }
        }
    } else if (indexType == URLSTR_INDEX) {
        extractedFileString.replace(QRegExp("\a+"), "\a"); // better to have only a character as separator for the splitting operation
        QStringList items = extractedFileString.split("\a", QString::SkipEmptyParts);
        if (items.count() != 0) {
            coverFileName = items.at(0);
            qDebug() << "[thumbnailer-chm]" << "Found cover file name:" << coverFileName;
        }
    }
}

void CHMCreator::checkCoverFileName()
{
    if (coverFileName.left(1) == ".") {
        coverFileName.remove(0, 1);
    }

    if (coverFileName.left(1) != "/") {
        coverFileName.prepend("/");
    }

    if (coverFileName.contains("%20") == true) {
        coverFileName.replace("%20", " ");
    }
    
    if (coverFileName.contains("&amp;") == true) {
        coverFileName.replace("&amp;", "&");
    }
}

ThumbCreator::Flags CHMCreator::flags() const
{
    return None;
}
