#include "QtFramelessWindow.h"
#include <QApplication>
#include <QPoint>
#include <QSize>
#include <QAbstractButton>

#ifdef Q_OS_WIN

#include <windowsx.h>
#include <dwmapi.h>
#include <objidl.h> // Fixes error C2504: 'IUnknown' : base class undefined
#include <gdiplus.h>

class QtFramelessWindow::QtFramelessWindowPrivate : public QObject {
 public:
    explicit QtFramelessWindowPrivate(QtFramelessWindow *q) : QObject(q), q_ptr(q),
                                                              m_titleBar(Q_NULLPTR),
                                                              m_borderWidth(5),
                                                              m_bJustMaximized(false),
                                                              m_bResizeable(true) {}
    ~QtFramelessWindowPrivate() override = default;

 public:
    Q_SLOT void onTitleBarDestroyed() {
        if (m_titleBar == QObject::sender()) {
            m_titleBar = Q_NULLPTR;
        }
    }

 public:
    QWidget *m_titleBar;
    QList<QWidget *> blacklist;
    int m_borderWidth;

    QMargins m_margins;
    QMargins m_frames;
    bool m_bJustMaximized;

    bool m_bResizeable;

 private:
    Q_DECLARE_PUBLIC(QtFramelessWindow);
    QtFramelessWindow *q_ptr;
};

QtFramelessWindow::QtFramelessWindow(QWidget *parent) : QWidget(parent),
                                                        d_ptr(new QtFramelessWindowPrivate(this)) {
    Q_D(QtFramelessWindow);
    //    setWindowFlag(Qt::Window,true);
    //    setWindowFlag(Qt::FramelessWindowHint, true);
    //    setWindowFlag(Qt::WindowSystemMenuHint, true);
    //    setWindowFlag() is not available before Qt v5.9, so we should use setWindowFlags instead
    setWindowFlags(windowFlags() | Qt::Window | Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    setResizeable(d->m_bResizeable);
}

QtFramelessWindow::~QtFramelessWindow() {
    delete d_ptr;
}

void QtFramelessWindow::setResizeable(bool resizeable) {
    Q_D(QtFramelessWindow);
    bool visible = isVisible();
    d->m_bResizeable = resizeable;
    if (d->m_bResizeable) {
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

bool QtFramelessWindow::resizeable() const {
    Q_D(const QtFramelessWindow);
    return d->m_bResizeable;
}

void QtFramelessWindow::setResizeableAreaWidth(int width) {
    Q_D(QtFramelessWindow);
    if (1 > width) width = 1;
    d->m_borderWidth = width;
}

int QtFramelessWindow::resizeableAreaWidth() const {
    Q_D(const QtFramelessWindow);
    return d->m_borderWidth;
}

void QtFramelessWindow::setTitleBar(QWidget *titleBar) {
    Q_D(QtFramelessWindow);
    d->m_titleBar = titleBar;
    if (!titleBar) return;
    connect(titleBar, &QObject::destroyed, d, &QtFramelessWindowPrivate::onTitleBarDestroyed);
}

void QtFramelessWindow::addBlacklistWidget(QWidget *widget) {
    Q_D(QtFramelessWindow);
    if (!widget) return;
    if (d->blacklist.contains(widget)) return;
    d->blacklist.append(widget);
}

bool QtFramelessWindow::nativeEvent(const QByteArray &eventType, void *message, long *result) {
    Q_D(QtFramelessWindow);
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

            const LONG border_width = d->m_borderWidth;
            RECT winRect;
            GetWindowRect(HWND(winId()), &winRect);

            long x = GET_X_LPARAM(msg->lParam);
            long y = GET_Y_LPARAM(msg->lParam);

            if (d->m_bResizeable) {

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
            if (!d->m_titleBar) return false;

            // support high-dpi
            double dpr = this->devicePixelRatioF();
            QPoint pos = d->m_titleBar->mapFromGlobal(QPoint(x / dpr, y / dpr));

            if (!d->m_titleBar->rect().contains(pos)) return false;
            QWidget *child = d->m_titleBar->childAt(pos);
            if (child) {
                if (d->blacklist.contains(child)) {
                    return false;
                } else if (auto btn = qobject_cast<QAbstractButton *>(child)) {
                    if (btn->isEnabled()) {
                        return false;
                    }
                }
            }
            *result = HTCAPTION;
            return true;
        } //end case WM_NCHITTEST
        case WM_GETMINMAXINFO: {
            if (::IsZoomed(msg->hwnd)) {
                RECT frame = {0, 0, 0, 0};
                AdjustWindowRectEx(&frame, WS_OVERLAPPEDWINDOW, FALSE, 0);

                //record frame area data
                double dpr = this->devicePixelRatioF();

                d->m_frames.setLeft(std::lround(std::abs(frame.left) / dpr));
                d->m_frames.setTop(std::lround(std::abs(frame.bottom) / dpr));
                d->m_frames.setRight(std::lround(std::abs(frame.right) / dpr));
                d->m_frames.setBottom(std::lround(std::abs(frame.bottom) / dpr));

                QWidget::setContentsMargins(d->m_frames.left() + d->m_margins.left(), \
                                            d->m_frames.top() + d->m_margins.top(), \
                                            d->m_frames.right() + d->m_margins.right(), \
                                            d->m_frames.bottom() + d->m_margins.bottom());
                d->m_bJustMaximized = true;
            } else {
                if (d->m_bJustMaximized) {
                    QWidget::setContentsMargins(d->m_margins);
                    d->m_frames = QMargins();
                    d->m_bJustMaximized = false;
                }
            }
            return false;
        }
        default:
            return QWidget::nativeEvent(eventType, message, result);
    }
}

void QtFramelessWindow::setContentsMargins(const QMargins &margins) {
    Q_D(QtFramelessWindow);
    QWidget::setContentsMargins(margins + d->m_frames);
    d->m_margins = margins;
}

void QtFramelessWindow::setContentsMargins(int left, int top, int right, int bottom) {
    Q_D(QtFramelessWindow);
    QWidget::setContentsMargins(left + d->m_frames.left(), \
                                    top + d->m_frames.top(), \
                                    right + d->m_frames.right(), \
                                    bottom + d->m_frames.bottom());
    d->m_margins.setLeft(left);
    d->m_margins.setTop(top);
    d->m_margins.setRight(right);
    d->m_margins.setBottom(bottom);
}

QMargins QtFramelessWindow::contentsMargins() const {
    Q_D(const QtFramelessWindow);
    QMargins margins = QWidget::contentsMargins();
    margins -= d->m_frames;
    return margins;
}

void QtFramelessWindow::getContentsMargins(int *left, int *top, int *right, int *bottom) const {
    Q_D(const QtFramelessWindow);
    auto margins = QWidget::contentsMargins();
    if (isMaximized()) {
        if (left) *left = margins.left() - d->m_frames.left();
        if (top) *top = margins.top() - d->m_frames.top();
        if (right) *right = margins.right() - d->m_frames.right();
        if (bottom) *bottom = margins.bottom() - d->m_frames.bottom();
    } else {
        if (left) *left = margins.left();
        if (top) *top = margins.top();
        if (right) *right = margins.right();
        if (bottom) *bottom = margins.bottom();
    }
}

QRect QtFramelessWindow::contentsRect() const {
    Q_D(const QtFramelessWindow);
    QRect rect = QWidget::contentsRect();
    int width = rect.width();
    int height = rect.height();
    rect.setLeft(rect.left() - d->m_frames.left());
    rect.setTop(rect.top() - d->m_frames.top());
    rect.setWidth(width);
    rect.setHeight(height);
    return rect;
}

void QtFramelessWindow::showFullScreen() {
    Q_D(QtFramelessWindow);
    if (isMaximized()) {
        QWidget::setContentsMargins(d->m_margins);
        d->m_frames = QMargins();
    }
    QWidget::showFullScreen();
}

#endif //Q_OS_WIN
