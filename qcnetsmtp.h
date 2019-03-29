/***************************************************************************
*   Copyright (C) 2014-2020 by Denis Gottardello                          *
*   info@denisgottardello.it                                              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 3 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
*   This program is distributed in the hope that it will be useful,       *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU General Public License for more details.                          *
*                                                                         *
*   You should have received a copy of the GNU General Public License     *
*   along with this program; if not, write to the                         *
*   Free Software Foundation, Inc.,                                       *
*   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
***************************************************************************/

#ifndef QCNETSMTP_H
#define QCNETSMTP_H

#include "QStringList"
#include "QFile"
#include "QDateTime"
#include "QFileInfo"
#include "QSslSocket"

class QCNetSMTP : public QObject
{
    Q_OBJECT

public:
    QCNetSMTP(bool ShowLog= false);
    ~QCNetSMTP();
    const static int PRIORITYNORMAL= 5;
    const static int PRIORITYMEDIUM= 3;
    const static int PRIORITYHIGH= 1;
    bool HtmlFormat, Authentication, SSLConnection;
    int Socket, To_count, Priority, Timeout;
    QString From, To, Subject, BodyPlain, BodyHtml, Server, Cc, Bcc, UserID, Password;
    bool Execute(int Timeout);
    void AddAttach(QString Attach, QString Id= "");

private:
    bool EhloSent, AuthenticationSent, AuthLoginSent, AuthLoginConfirmed, UserIDConfirmed, PasswordConfirmed, Begin, MailFromSent, RcptToSent, DataConfirmed, MailSent, MailFormConfirmed, RcptToConfirmed, DataSent, EndCorrectly, EndWithError;
    bool ShowLog;
    QString CRLF;
    QString Boundary_ALTERNATIVE, Boundary_MIXED, Boundary_RELATED, Boundary_TEXTHTML;
    QStringList QSLAttach, QSLTo, QSLCc, QSLBcc, QSLAttachId;
    void AddressSplitter(QString Address, QStringList &QSLResult);
    void Log(QString RowLog);
    void ParseInput(QString SMTPCode, QSslSocket *Tcp);
    void SendRctpTo(QString Address);
    QByteArray ArrayToBase64(QByteArray ArrayIn);
    QString GetTimeZoneOffset();
    QString QDTToRfc2822(QDateTime QDTDataIn);
    QString RandomString(int Length);

};

#endif // QCNETSMTP_H
