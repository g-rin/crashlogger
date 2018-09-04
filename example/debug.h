#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

#define DEBUG(str) qDebug() << this->thread() << str

#endif  // DEBUG_H
