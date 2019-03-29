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

#include "qcnetsmtp.h"

QCNetSMTP::QCNetSMTP(bool ShowLog) {
    this->ShowLog= ShowLog;
    CRLF= "\r\n";
    HtmlFormat= false;
    EndCorrectly= false;
    SSLConnection= false;
    Priority= PRIORITYNORMAL;
    Boundary_ALTERNATIVE= "Boundary_ALTERNATIVE";
    Boundary_MIXED= "Boundary_MIXED";
    Boundary_RELATED= "Boundary_RELATED";
    Boundary_TEXTHTML= "Boundary_TEXTHTML";
    Log("QCNetSMTP::QCNetSMTP()");
}

QCNetSMTP::~QCNetSMTP() {
    Log("QCNetSMTP::~QCNetSMTP()");
}

QString QCNetSMTP::GetTimeZoneOffset() {
    QDateTime QDTNow= QDateTime::currentDateTime();
    while (QDTNow.date().day()!= QDTNow.toUTC().date().day()) QDTNow= QDTNow.addSecs(60 * 60);
    int TimeZoneOffset= QDTNow.time().hour()- QDTNow.toUTC().time().hour();
    if (TimeZoneOffset> 9) return "+"+ QString::number(TimeZoneOffset)+ "00";
    else if (TimeZoneOffset< -9) return QString::number(TimeZoneOffset)+ "00";
    else if (TimeZoneOffset>= 0) return "+0"+ QString::number(TimeZoneOffset)+ "00";
    else if (TimeZoneOffset< -9) return "-0"+ QString::number(TimeZoneOffset * -1)+ "00";
    return "";
}

QString QCNetSMTP::QDTToRfc2822(QDateTime QDTDataIn) {
    QString RowOut;
    switch (QDTDataIn.date().dayOfWeek()) {
    case 1: RowOut= "Mon"; break;
    case 2: RowOut= "Tue"; break;
    case 3: RowOut= "Wed"; break;
    case 4: RowOut= "Thu"; break;
    case 5: RowOut= "Fri"; break;
    case 6: RowOut= "Sat"; break;
    case 7: RowOut= "Sun"; break;
    }
    RowOut+= ", "+ QString::number(QDTDataIn.date().day())+ " ";
    switch (QDTDataIn.date().month()) {
    case 1: RowOut+= "Jan"; break;
    case 2: RowOut+= "Feb"; break;
    case 3: RowOut+= "Mar"; break;
    case 4: RowOut+= "Apr"; break;
    case 5: RowOut+= "May"; break;
    case 6: RowOut+= "Jun"; break;
    case 7: RowOut+= "Jul"; break;
    case 8: RowOut+= "Aug"; break;
    case 9: RowOut+= "Sep"; break;
    case 10: RowOut+= "Oct"; break;
    case 11: RowOut+= "Nov"; break;
    case 12: RowOut+= "Dec"; break;
    }
    RowOut+= " "+ QDTDataIn.toString("yyyy hh:mm:ss ")+ GetTimeZoneOffset();
    return RowOut;
}

bool QCNetSMTP::Execute(int Timeout) {
    this->Timeout= Timeout;
    Log("QCNetSMTP::Execute() begin");
    To_count= 0;
    EhloSent= false;
    MailFromSent= false;
    RcptToSent= false;
    DataConfirmed= false;
    MailSent= false;
    AuthLoginSent= false;
    AuthLoginConfirmed= false;
    UserIDConfirmed= false;
    PasswordConfirmed= false;
    EndCorrectly= false;
    EndWithError= false;
    AddressSplitter(To, QSLTo);
    AddressSplitter(Cc, QSLCc);
    AddressSplitter(Bcc, QSLBcc);
    QSslSocket *Tcp= new QSslSocket(); {
        Tcp->connectToHost(Server, Socket);
        if (Tcp->waitForConnected()) {
            if (SSLConnection) Tcp->startClientEncryption();
            while (!EndCorrectly && !EndWithError && Tcp->waitForReadyRead(Timeout)) {
                QStringList QSLHeaderIn;
                while (Tcp->canReadLine()) {
                    QString RowIn= Tcp->readLine().replace(QString(QChar(13)), "").replace(QString(QChar(10)), "");
                    QSLHeaderIn.append(RowIn);
                    Log(RowIn);
                }
                if (QSLHeaderIn.count()> 0 && QSLHeaderIn[0].length()>= 3) {
                    ParseInput(QSLHeaderIn[0].left(3), Tcp);
                    Tcp->flush();
                }
            }
        }
    }{
        delete Tcp;
    }
    Log("QCNetSMTP::Execute() end");
    return EndCorrectly;
}

void QCNetSMTP::AddAttach(QString Attach, QString Id) {
    QSLAttach.append(Attach);
    QSLAttachId.append(Id);
}

void QCNetSMTP::AddressSplitter(QString Address, QStringList &QSLResult) {
    QSLResult.clear();
    Address= Address.replace(QString(" "), "");
    bool Finded= true;
    while (Finded) {
        Finded= false;
        if (Address.indexOf(";")> -1) {
            Finded= true;
            QSLResult.append(Address.left(Address.indexOf(";")));
            Address= Address.right(Address.length()- Address.indexOf(";")- 1);
        }
    }
    if (Address.length()> 0) QSLResult.append(Address);
}

QString QCNetSMTP::RandomString(int Length) {
    QString RandomString= "";
    static const char BaseString[]= "0123456789" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz";
    for (int count= 0; count< Length; count++) RandomString+= BaseString[rand() % (sizeof(BaseString)- 1)];
    return RandomString;
}


// Supported smtp servers:
// All domains hosted by Aruba
// mail.hosting-ic.com with Authentication
// mail.netcatalyst.it with Authentication
// out.alice.it:587 with Authentication
// out.alice.it:25
// smtp.gmail.com (ssl 465)
// smtp.libero.it with Authentication
// smtp.trivenet.it

QByteArray QCNetSMTP::ArrayToBase64(QByteArray ArrayIn) {
    ArrayIn= ArrayIn.toBase64();
    QByteArray ArrayOut;
    while (ArrayIn.length()> 79) {
        ArrayOut+= ArrayIn.left(79)+ '\n';
        ArrayIn= ArrayIn.right(ArrayIn.length()- 79);
    }
    ArrayOut+= ArrayIn;
    return ArrayOut;
}

void QCNetSMTP::ParseInput(QString SMTPCode, QSslSocket *Tcp) {
    Log("QCNetSMTP::ParseInput() ->"+ SMTPCode+ "<-");
    if (!EhloSent && SMTPCode.compare("220")== 0) {
        QString RowOut= "ehlo CiccioPasticcio"+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1());
        EhloSent= true;
    } else if (Authentication && !AuthLoginSent && SMTPCode.compare("250")== 0) {
        QString RowOut= "auth login"+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1());
        AuthLoginSent= true;
    } else if (Authentication && AuthLoginSent && !AuthLoginConfirmed && SMTPCode.compare("334")== 0) {
        AuthLoginConfirmed= true;
        QString RowOut= UserID;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1().toBase64());
        Tcp->write(QString(CRLF).toLatin1());
    } else if (Authentication && AuthLoginSent && AuthLoginConfirmed && !UserIDConfirmed && SMTPCode.compare("334")== 0) {
        UserIDConfirmed= true;
        QString RowOut= Password;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1().toBase64());
        Tcp->write(QString(CRLF).toLatin1());
    } else if (Authentication && AuthLoginSent && AuthLoginConfirmed && UserIDConfirmed && !PasswordConfirmed && SMTPCode.compare("235")== 0) {
        PasswordConfirmed= true;
        QString RowOut= "mail from: <"+ From+ + ">"+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1());
        MailFromSent= true;
    } else if (!MailFromSent && SMTPCode.compare("250")== 0) {
        QString RowOut= "mail from: <"+ From+ + ">"+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
        Tcp->write(RowOut.toLatin1());
        MailFromSent= true;
    } else if (MailFromSent && !RcptToSent && SMTPCode.compare("250")== 0) {
        if (To_count< QSLTo.count()) {
            QString RowOut= "rcpt to: <"+ QSLTo[To_count]+ ">"+ CRLF;
            Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
            Tcp->write(RowOut.toLatin1());
            To_count++;
        } else if (To_count< QSLTo.count()+ QSLCc.count()) {
            QString RowOut= "rcpt to: <"+ QSLCc[To_count- QSLTo.count()]+ ">"+ CRLF;
            Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
            Tcp->write(RowOut.toLatin1());
            To_count++;
        } else if (To_count< QSLTo.count()+ QSLCc.count()+ QSLBcc.count()) {
            QString RowOut= "rcpt to: <"+ QSLBcc[To_count- QSLTo.count()- QSLCc.count()]+ ">"+ CRLF;
            Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
            Tcp->write(RowOut.toLatin1());
            To_count++;
        } else {
            RcptToSent= true;
            QString RowOut= "data"+ CRLF;
            Log("QCNetSMTP::ParseInput() ->"+ RowOut+ "<-");
            Tcp->write(RowOut.toLatin1());
        }
    } else if (MailFromSent && RcptToSent && !DataConfirmed && !MailSent && SMTPCode.compare("354")== 0) {
        DataConfirmed= true;
        QString HeaderOut= "From: "+ From+ CRLF;
        for (int count= 0; count< QSLTo.count(); count++) HeaderOut+= "To: "+ QSLTo[count]+ CRLF;
        for (int count= 0; count< QSLCc.count(); count++) HeaderOut+= "Cc: "+ QSLCc[count]+ CRLF;
        HeaderOut+= "Subject: "+ Subject+ CRLF;
        HeaderOut+= "Date: "+ QDTToRfc2822(QDateTime::currentDateTime())+ CRLF;
        HeaderOut+= "Sender: My custom mail sender"+ CRLF;
        HeaderOut+= "Message-ID: <"+ RandomString(64)+ "@"+ Server+ ">"+ CRLF;
        switch (Priority) {
        case PRIORITYNORMAL: HeaderOut+= "X-PRIORITY: "+ QString::number(Priority)+ " (Normal)"+ CRLF; break;
        case PRIORITYMEDIUM: HeaderOut+= "X-PRIORITY: "+ QString::number(Priority)+ " (Medium)"+ CRLF; break;
        case PRIORITYHIGH: HeaderOut+= "X-PRIORITY: "+ QString::number(Priority)+ " (Highest)"+ CRLF; break;
            HeaderOut+= "X-PRIORITY: "+ QString::number(Priority)+ CRLF; break;
        }
        HeaderOut+= "MIME-Version: 1.0"+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
        Tcp->write(HeaderOut.toLatin1());
        if (HtmlFormat) {
            bool HasAttach= false, HasObject= false;
            for (int count= 0; count< QSLAttach.count(); count++) {
                if (QSLAttachId[count].compare("")== 0) HasAttach= true;
                if (QSLAttachId[count].compare("")!= 0) HasObject= true;
            }
            if (HasAttach && HasObject) {
                HeaderOut= "Content-Type: Multipart/Mixed; Boundary=\""+ Boundary_MIXED+ "\""+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_MIXED+ CRLF;
                HeaderOut+= "Content-Type: Multipart/Related; Boundary=\""+ Boundary_RELATED+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_RELATED+ CRLF;
                HeaderOut+= "Content-Type: Multipart/Alternative; Boundary=\""+ Boundary_TEXTHTML+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= "Content-Diposition: inline"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Plain; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Html; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyHtml.toLatin1()+ CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
                for (int count= 0; count< QSLAttach.count(); count++) {
                    if (QSLAttachId[count].compare("")!= 0) {
                        QFile FileIn(QSLAttach[count]);
                        if (FileIn.open(QIODevice::ReadOnly)) {
                            HeaderOut= CRLF;
                            HeaderOut+= "--"+ Boundary_RELATED+ CRLF;
                            HeaderOut+= "Content-Type: Image/Jpeg;\n name=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                            HeaderOut+= "Content-Id: <"+ QSLAttachId[count]+ ">"+ CRLF;
                            HeaderOut+= "Content-Transfer-Encoding: base64"+ CRLF;
                            HeaderOut+= CRLF;
                            Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                            Tcp->write(HeaderOut.toLatin1());
                            QByteArray ByteOut= FileIn.readAll();
                            Tcp->write(ArrayToBase64(ByteOut));
                            HeaderOut= CRLF;
                            HeaderOut+= CRLF;
                            Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                            Tcp->write(HeaderOut.toLatin1());
                            FileIn.close();
                        }
                    }
                }
                HeaderOut= "--"+ Boundary_RELATED+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
                for (int count= 0; count< QSLAttach.count(); count++) {
                    if (QSLAttachId[count].compare("")== 0) {
                        QFile FileIn(QSLAttach[count]);
                        if (FileIn.open(QIODevice::ReadOnly)) {
                            HeaderOut= CRLF;
                            HeaderOut+= "--"+ Boundary_MIXED+ CRLF;
                            HeaderOut+= "Content-Type: Image/Jpeg;\n name=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                            HeaderOut+= "Content-Transfer-Encoding: base64"+ CRLF;
                            HeaderOut+= "Content-Disposition: attachment; filename=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                            HeaderOut+= CRLF;
                            Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                            Tcp->write(HeaderOut.toLatin1());
                            QByteArray ByteOut= FileIn.readAll();
                            Tcp->write(ArrayToBase64(ByteOut));
                            HeaderOut= CRLF;
                            HeaderOut+= CRLF;
                            Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                            Tcp->write(HeaderOut.toLatin1());
                            FileIn.close();
                        }
                    }
                }
                HeaderOut= "--"+ Boundary_MIXED+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            } else if (HasAttach && !HasObject) {
                HeaderOut= "Content-Type: Multipart/Mixed; Boundary=\""+ Boundary_MIXED+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_MIXED+ CRLF;
                HeaderOut+= "Content-Type: Multipart/Alternative; Boundary=\""+ Boundary_TEXTHTML+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Plain; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Html; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyHtml.toLatin1()+ CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
                for (int count= 0; count< QSLAttach.count(); count++) {
                    QFile FileIn(QSLAttach[count]);
                    if (FileIn.open(QIODevice::ReadOnly)) {
                        HeaderOut= CRLF;
                        HeaderOut+= "--"+ Boundary_MIXED+ CRLF;
                        HeaderOut+= "Content-Type: Image/Jpeg;\n name=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                        //HeaderOut+= "Content-Id: <"+ QSLAttachId[count]+ ">"+ CRLF;
                        HeaderOut+= "Content-Transfer-Encoding: base64"+ CRLF;
                        HeaderOut+= "Content-Disposition: attachment; filename=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        QByteArray ByteOut= FileIn.readAll();
                        Tcp->write(ArrayToBase64(ByteOut));
                        HeaderOut= CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        FileIn.close();
                    }
                }
                HeaderOut= "--"+ Boundary_MIXED+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            } else if (!HasAttach && HasObject) {
                HeaderOut= "Content-Type: Multipart/Related; Boundary=\""+ Boundary_RELATED+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_RELATED+ CRLF;
                HeaderOut+= "Content-Type: Multipart/Alternative; Boundary=\""+ Boundary_TEXTHTML+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= "Content-Diposition: inline"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Plain; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Html; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyHtml.toLatin1()+ CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
                for (int count= 0; count< QSLAttach.count(); count++) {
                    QFile FileIn(QSLAttach[count]);
                    if (FileIn.open(QIODevice::ReadOnly)) {
                        HeaderOut= CRLF;
                        HeaderOut+= "--"+ Boundary_RELATED+ CRLF;
                        HeaderOut+= "Content-Type: Image/Jpeg;\n name=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                        HeaderOut+= "Content-Id: <"+ QSLAttachId[count]+ ">"+ CRLF;
                        HeaderOut+= "Content-Transfer-Encoding: base64"+ CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        QByteArray ByteOut= FileIn.readAll();
                        Tcp->write(ArrayToBase64(ByteOut));
                        HeaderOut= CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        FileIn.close();
                    }
                }
                HeaderOut= "--"+ Boundary_RELATED+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            } else if (!HasAttach && !HasObject) {
                HeaderOut= "Content-Type: Multipart/Alternative; Boundary=\""+ Boundary_TEXTHTML+ "\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= "Content-Diposition: inline"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Plain; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Html; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= BodyHtml.toLatin1()+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ "--"+ CRLF;
                HeaderOut+= CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            }
        } else {
            if (QSLAttach.count()> 0) {
                HeaderOut= "Content-Type: Multipart/Mixed; boundary=\""+ Boundary_TEXTHTML+ "\""+ CRLF;
                HeaderOut+= CRLF;
                HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                HeaderOut+= "Content-Type: Text/Plain; charset=\"us-ascii\""+ CRLF;
                HeaderOut+= "Content-Transfer-Encoding: 7bit"+ CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
                for (int count= 0; count< QSLAttach.count(); count++) {
                    QFile FileIn(QSLAttach[count]);
                    if (FileIn.open(QIODevice::ReadOnly)) {
                        HeaderOut= CRLF;
                        HeaderOut+= "--"+ Boundary_TEXTHTML+ CRLF;
                        HeaderOut+= "Content-Type: Image/Jpeg;\n name=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                        HeaderOut+= "Content-Transfer-Encoding: base64"+ CRLF;
                        HeaderOut+= "Content-Disposition: attachment; filename=\""+ QFileInfo(FileIn).fileName()+ "\""+ CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        QByteArray ByteOut= FileIn.readAll();
                        Tcp->write(ArrayToBase64(ByteOut));
                        HeaderOut= CRLF;
                        HeaderOut+= CRLF;
                        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                        Tcp->write(HeaderOut.toLatin1());
                        Tcp->waitForBytesWritten(Timeout);
                        FileIn.close();
                    }
                }
                HeaderOut= "--"+ Boundary_TEXTHTML+ "--"+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            } else {
                HeaderOut= CRLF;
                HeaderOut+= BodyPlain.toLatin1()+ CRLF;
                Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
                Tcp->write(HeaderOut.toLatin1());
            }
        }
        HeaderOut= "."+ CRLF;
        Log("QCNetSMTP::ParseInput() ->"+ HeaderOut+ "<-");
        Tcp->write(HeaderOut.toLatin1());
        Tcp->waitForBytesWritten(Timeout);
        MailSent= true;
    } else if (MailFromSent && RcptToSent && DataConfirmed && MailSent && SMTPCode.compare("250")== 0) {
        Log("QCNetSMTP::ParseInput() End correctly");
        EndCorrectly= true;
    } else {
        Log("QCNetSMTP::ParseInput() ???");
        EndWithError= true;
    }
}

void QCNetSMTP::Log(QString RowLog) {
    if (ShowLog) qDebug() << RowLog;
}
