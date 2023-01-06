#include "QtFramelessWindow.h"
#include <QApplication>
#include <QPoint>
#include <QSize>

#ifdef Q_OS_WIN

#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h> // Fixes error C2504: 'IUnknown' : base class undefined
#include <gdiplus.h>

QtFramelessWindow::QtFramelessWindow(QWidget *parent) : QWidget(parent),
                                                        m_titleBar(Q_NULLPTR),
                                                        m_borderWidth(5),
                                                        m_bJustMaximized(false),
                                                        m_bResizeable(true) {
    //    setWindowFlag(Qt::Window,true);
    //    setWindowFlag(Qt::FramelessWindowHint, true);
    //    setWindowFlag(Qt::WindowSystemMenuHint, true);
    //    setWindowFlag() is not available before Qt v5.9, so we should use setWindowFlags instead
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setResizeable(m_bResizeable);
}

void QtFramelessWindow::setResizeable(bool resizeable) {
    bool visible = isVisible();
    m_bResizeable = resizeable;
    if (m_bResizeable) {
        setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint);
        //        setWindowFlag(Qt::WindowMaximizeButtonHint);

        //此行代码可以带回Aero效果，同时也带回了标题栏和边框,在nativeEvent()会再次去掉标题栏
        //
        //this line will get title bar/thick frame/Aero back, which is exactly what we want
        //we will get rid of title bar and thick frame again in nativeEvent() later
        HWND hwnd = (HWND) this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style | WS_MAXIMIZEBOX | WS_THICKFRAME | WS_CAPTION);
    } else {
        setWindowFlags(windowFlags() & ~Qt::WindowMaximizeButtonHint);
        //        setWindowFlag(Qt::WindowMaximizeButtonHint,false);

        HWND hwnd = (HWND) this->winId();
        DWORD style = ::GetWindowLong(hwnd, GWL_STYLE);
        ::SetWindowLong(hwnd, GWL_STYLE, style & ~WS_MAXIMIZEBOX & ~WS_CAPTION);
    }

    //保留一个像素的边框宽度，否则系统不会绘制边框阴影
    //
    //we better left 1 pixel width of border untouched, so OS can draw nice shadow around it
    const MARGINS shadow = {1, 1, 1, 1};
    DwmExtendFrameIntoClientArea(HWND(winId()), &shadow);

    setVisible(visible);
}

void QtFramelessWindow::setResizeableAreaWidth(int width) {
    if (1 > width) width = 1;
    m_borderWidth = width;
}

void QtFramelessWindow::setTitleBar(QWidget *titleBar) {
    m_titleBar = titleBar;
    if (!titleBar) return;
    connect(titleBar, &QObject::destroyed, this, &QtFramelessWindow::onTitleBarDestroyed);
}

void QtFramelessWindow::onTitleBarDestroyed() {
    if (m_titleBar == QObject::sender()) {
        m_titleBar = Q_NULLPTR;
    }
}

void QtFramelessWindow::addIgnoreWidget(QWidget *widget) {
    if (!widget) return;
    if (m_whiteList.contains(widget)) return;
    m_whiteList.append(widget);
}

bool QtFramelessWindow::nativeEvent(const QByteArray &eventType, void *message, long *result) {
    //Workaround for known bug -> check Qt forum : https://forum.qt.io/topic/93141/qtablewidget-itemselectionchanged/13
#if (QT_VERSION == QT_VERSION_CHECK(5, 11, 1))
    MSG* msg = *reinterpret_cast<MSG**>(message);
#else
    MSG *msg = reinterpret_cast<MSG *>(message);
#endif

    switch (msg->message) {
        case WM_NCCALCSIZE: {
            NCCALCSIZE_PARAMS &params = *reinterpret_cast<NCCALCSIZE_PARAMS *>(msg->lParam);
            if (params.rgrc[0].top != 0)
                params.rgrc[0].top -= 1;

            //this kills the window frame and title bar we added with WS_THICKFRAME and WS_CAPTION
            *result = WVR_REDRAW;
            return true;
        }
        case WM_NCHITTEST: {
            *result = 0;

            const LONG border_width = m_borderWidth;
            RECT winRect;
            GetWindowRect(HWND(winId()), &winRect);

            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);

            if (m_bResizeable) {

                bool resizeWidth = minimumWidth() != maximumWidth();
                bool resizeHeight = minimumHeight() != maximumHeight();

                if (resizeWidth) {
                    //left border
                    if (x >= winRect.left && x < winRect.left + border_width) {
                        *result = HTLEFT;
                    }
                    //right border
                    if (x < winRect.right && x >= winRect.right - border_width) {
                        *result = HTRIGHT;
                    }
                }
                if (resizeHeight) {
                    //bottom border
                    if (y < winRect.bottom && y >= winRect.bottom - border_width) {
                        *result = HTBOTTOM;
                    }
                    //top border
                    if (y >= winRect.top && y < winRect.top + border_width) {
                        *result = HTTOP;
                    }
                }
                if (resizeWidth && resizeHeight) {
                    //bottom left corner
                    if (x >= winRect.left && x < winRect.left + border_width &&
                        y < winRect.bottom && y >= winRect.bottom - border_width) {
                        *result = HTBOTTOMLEFT;
                    }
                    //bottom right corner
                    if (x < winRect.right && x >= winRect.right - border_width &&
                        y < winRect.bottom && y >= winRect.bottom - border_width) {
                        *result = HTBOTTOMRIGHT;
                    }
                    //top left corner
                    if (x >= winRect.left && x < winRect.left + border_width &&
                        y >= winRect.top && y < winRect.top + border_width) {
                        *result = HTTOPLEFT;
                    }
                    //top right corner
                    if (x < winRect.right && x >= winRect.right - border_width &&
                        y >= winRect.top && y < winRect.top + border_width) {
                        *result = HTTOPRIGHT;
                    }
                }
            }
            if (0 != *result) return true;

            // *result still equals 0, that means the cursor locate OUTSIDE the frame area,
            // but it may locate in title bar area
            if (!m_titleBar) return false;

            //support high-dpi
            double dpr = this->devicePixelRatioF();
            QPoint pos = m_titleBar->mapFromGlobal(QPoint(x / dpr, y / dpr));

            if (!m_titleBar->rect().contains(pos)) return false;
            QWidget *child = m_titleBar->childAt(pos);
            if (!child) {
                *result = HTCAPTION;
                return true;
            } else {
                if (m_whiteList.contains(child)) {
                    *result = HTCAPTION;
                    return true;
                }
            }
            return false;
        } //end case WM_NCHITTEST
        case WM_GETMINMAXINFO: {
            if (::IsZoomed(msg->hwnd)) {
                RECT frame = {0, 0, 0, 0};
                AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

                //record frame area data
                double dpr = this->devicePixelRatioF();

                m_frames.setLeft(std::lround(std::abs(frame.left) / dpr));
                m_frames.setTop(std::lround(std::abs(frame.bottom) / dpr));
                m_frames.setRight(std::lround(std::abs(frame.right) / dpr));
                m_frames.setBottom(std::lround(std::abs(frame.bottom) / dpr));

                QWidget::setContentsMargins(m_frames.left() + m_margins.left(), \
                                            m_frames.top() + m_margins.top(), \
                                            m_frames.right() + m_margins.right(), \
                                            m_frames.bottom() + m_margins.bottom());
                m_bJustMaximized = true;
            } else {
                if (m_bJustMaximized) {
                    QWidget::setContentsMargins(m_margins);
                    m_frames = QMargins();
                    m_bJustMaximized = false;
                }
            }
            return false;
        }
        default:
            return QWidget::nativeEvent(eventType, message, result);
    }
}

void QtFramelessWindow::setContentsMargins(const QMargins &margins) {
    QWidget::setContentsMargins(margins + m_frames);
    m_margins = margins;
}

void QtFramelessWindow::setContentsMargins(int left, int top, int right, int bottom) {
    QWidget::setContentsMargins(left + m_frames.left(), \
                                    top + m_frames.top(), \
                                    right + m_frames.right(), \
                                    bottom + m_frames.bottom());
    m_margins.setLeft(left);
    m_margins.setTop(top);
    m_margins.setRight(right);
    m_margins.setBottom(bottom);
}

QMargins QtFramelessWindow::contentsMargins() const {
    QMargins margins = QWidget::contentsMargins();
    margins -= m_frames;
    return margins;
}

void QtFramelessWindow::getContentsMargins(int *left, int *top, int *right, int *bottom) const {
    auto margins = QWidget::contentsMargins();
    if (isMaximized()) {
        if (left) *left = margins.left() - m_frames.left();
        if (top) *top = margins.top() - m_frames.top();
        if (right) *right = margins.right() - m_frames.right();
        if (bottom) *bottom = margins.bottom() - m_frames.bottom();
    } else {
        if (left) *left = margins.left();
        if (top) *top = margins.top();
        if (right) *right = margins.right();
        if (bottom) *bottom = margins.bottom();
    }
}

QRect QtFramelessWindow::contentsRect() const {
    QRect rect = QWidget::contentsRect();
    int width = rect.width();
    int height = rect.height();
    rect.setLeft(rect.left() - m_frames.left());
    rect.setTop(rect.top() - m_frames.top());
    rect.setWidth(width);
    rect.setHeight(height);
    return rect;
}

void QtFramelessWindow::showFullScreen() {
    if (isMaximized()) {
        QWidget::setContentsMargins(m_margins);
        m_frames = QMargins();
    }
    QWidget::showFullScreen();
}

#endif //Q_OS_WIN
