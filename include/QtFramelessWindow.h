﻿#ifndef QTFRAMELESSWINDOW_H
#define QTFRAMELESSWINDOW_H

#include "qsystemdetection.h"
#include <QMainWindow>

// A nice frameless window for both Windows and OS X
// Author: fnMrRice
// Github: https://github.com/fnMrRice/QtFramelessWindow
// Usage: use "QtFramelessWindow" as base class instead of "QWidget", and enjoy

#ifdef Q_OS_WIN
#include <QWidget>
#include <QList>
#include <QMargins>
#include <QRect>

class QtFramelessWindow : public QWidget {
 Q_OBJECT
 public:
    explicit QtFramelessWindow(QWidget *parent = nullptr);

 public:
    //设置是否可以通过鼠标调整窗口大小
    //if resizeable is set to false, then the window can not be resized by mouse
    //but still can be resized programmatically
    void setResizeable(bool resizeable = true);
    bool isResizeable() const { return m_bResizeable; }

    //设置可调整大小区域的宽度，在此区域内，可以使用鼠标调整窗口大小
    //set border width, inside this area, window can be resized by mouse
    void setResizeableAreaWidth(int width = 5);

 protected:
    //设置一个标题栏widget，此widget会被当做标题栏对待
    //set a widget which will be treated as SYSTEM title bar
    void setTitleBar(QWidget *titleBar);

    //在标题栏控件内，也可以有子控件如标签控件“label1”，此label1遮盖了标题栏，导致不能通过label1拖动窗口
    //要解决此问题，使用addIgnoreWidget(label1)
    //generally, we can add widget say "label1" on title bar, and it will cover the title bar under it
    //as a result, we can not drag and move the MainWindow with this "label1" again
    //we can fix this by add "label1" to an ignore list, just call addIgnoreWidget(label1)
    void addIgnoreWidget(QWidget *widget);

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

 private Q_SLOTS:
    void onTitleBarDestroyed();

 public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;

 public slots:
    void showFullScreen();

 private:
    QWidget *m_titleBar;
    QList<QWidget *> m_whiteList;
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;
};

#endif

#endif // QTFRAMELESSWINDOW_H
