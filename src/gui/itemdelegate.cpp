/*
* Copyright (C) 2008-2013 The Communi Project
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
*/

#include "itemdelegate.h"
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QLineEdit>
#include <QPalette>
#include <QPainter>

ItemDelegate::ItemDelegate(QObject* parent) : QStyledItemDelegate(parent)
{
    d.rootIsDecorated = false;
}

bool ItemDelegate::rootIsDecorated() const
{
    return d.rootIsDecorated;
}

void ItemDelegate::setRootIsDecorated(bool decorated)
{
    d.rootIsDecorated = decorated;
}

QSize ItemDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QSize size = QStyledItemDelegate::sizeHint(option, index);
    static int height = 0;
    if (!height) {
        QLineEdit lineEdit;
        lineEdit.setStyleSheet("QLineEdit { border: 1px solid transparent; }");
        height = lineEdit.sizeHint().height();
    }
    size.setHeight(height);
    return size;
}

void ItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    if (d.rootIsDecorated && !index.parent().isValid()) {
        const bool selected = option.state & QStyle::State_Selected;
        const_cast<QStyleOptionViewItem&>(option).state &= ~QStyle::State_Selected;

        QColor c1 = qApp->palette().color(QPalette::Light);
        QColor c2 = qApp->palette().color(QPalette::Button);
        if (selected)
            qSwap(c1, c2);

        QLinearGradient gradient(option.rect.topLeft(), option.rect.bottomLeft());
        gradient.setColorAt(0.0, c1);
        gradient.setColorAt(1.0, c2);
        painter->fillRect(option.rect, gradient);

        QVector<QLine> lines;
        if (index.row() > 0)
            lines += QLine(option.rect.topLeft(), option.rect.topRight());
        lines += QLine(option.rect.bottomLeft(), option.rect.bottomRight());
        QPen oldPen = painter->pen();
        painter->setPen(qApp->palette().color(QPalette::Dark));
        painter->drawLines(lines);
        painter->setPen(oldPen);
    }

    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() == 1) {
        int badge = index.data(BadgeRole).toInt();
        if (badge > 0) {
            QRect rect = option.rect.adjusted(1, 3, -1, -3);

            painter->save();
            painter->setPen(Qt::NoPen);
            painter->setBrush(qvariant_cast<QBrush>(index.data(BadgeColorRole)));
            painter->setRenderHint(QPainter::Antialiasing);
            painter->drawRoundedRect(rect, 40, 80, Qt::RelativeSize);

            QFont font;
            if (font.pointSize() != -1)
                font.setPointSizeF(0.8 * font.pointSizeF());
            painter->setFont(font);

            QString txt;
            if (badge > 999)
                txt = QLatin1String("...");
            else
                txt = QFontMetrics(font).elidedText(QString::number(badge), Qt::ElideRight, rect.width());

            painter->setPen(qApp->palette().color(QPalette::Light));
            painter->drawText(rect, Qt::AlignCenter, txt);
            painter->restore();
        }
    }
}