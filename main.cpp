#include <QApplication>
#include "qfmainform.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QFMainForm w;
    w.show();

    return a.exec();
}
