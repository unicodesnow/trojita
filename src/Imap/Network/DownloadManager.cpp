/* Copyright (C) 2006 - 2010 Jan Kundrát <jkt@gentoo.org>

   This file is part of the Trojita Qt IMAP e-mail client,
   http://trojita.flaska.net/

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or the version 3 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "DownloadManager.h"
#include "Imap/Model/MailboxTree.h"

#include <QDesktopServices>
#include <QDir>

namespace Imap {

namespace Network {

DownloadManager::DownloadManager( QObject* parent,
                                Imap::Network::MsgPartNetAccessManager* _manager,
                                Imap::Mailbox::TreeItemPart* _part):
    QObject( parent ), manager(_manager), part(_part), reply(0), saved(false)
{
}

QString DownloadManager::toRealFileName( Imap::Mailbox::TreeItemPart* part )
{
    QString name = part->fileName().isEmpty() ?
                   tr("msg_%1_%2").arg( part->message()->uid() ).arg( part->partId() )
                       : part->fileName();
    return QDir( QDesktopServices::storageLocation( QDesktopServices::DocumentsLocation )
                 ).filePath( name );
}

void DownloadManager::slotDownloadNow()
{
    Q_ASSERT( part );
    QString saveFileName = toRealFileName( part );
    emit fileNameRequested( &saveFileName );
    if ( saveFileName.isEmpty() )
        return;

    saving.setFileName( saveFileName );

    QNetworkRequest request;
    QUrl url;
    url.setScheme( QLatin1String("trojita-imap") );
    url.setHost( QLatin1String("msg") );
    url.setPath( part->pathToPart() );
    request.setUrl( url );
    reply = manager->get( request );
    connect( reply, SIGNAL(finished()), this, SLOT(slotDataTransfered()) );
    connect( reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(slotTransferError()) );
    connect( manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(slotDeleteReply(QNetworkReply*)) );
}

void DownloadManager::slotDataTransfered()
{
    Q_ASSERT( reply );
    if ( reply->error() == QNetworkReply::NoError ) {
        saving.open( QIODevice::WriteOnly );
        saving.write( reply->readAll() );
        saving.close();
        saved = true;
        emit succeeded();
    }
}

void DownloadManager::slotTransferError()
{
    Q_ASSERT( reply );
    emit transferError( reply->errorString() );
}

void DownloadManager::slotDeleteReply(QNetworkReply* reply)
{
    if ( reply == this->reply ) {
        if ( ! saved )
            slotDataTransfered();
        delete reply;
        this->reply = 0;
    }
}

}
}