#ifndef QTFRAMELESSWINDOW_H
#define QTFRAMELESSWINDOW_H

#include <QWidget>

/**
 * @brief A nice frameless window for Windows
 * @author fnMrRice
 * @date 2023-01-06
 * @details use "QtFramelessWindow" as base class instead of "QWidget", and enjoy
 */
class QtFramelessWindow : public QWidget {
 Q_OBJECT

 public:
    Q_PROPERTY(bool resizeable READ resizeable WRITE setResizeable)
    Q_PROPERTY(bool resizeableAreaWidth READ resizeableAreaWidth WRITE setResizeableAreaWidth)

 public:
    explicit QtFramelessWindow(QWidget *parent = nullptr);
    ~QtFramelessWindow() override;

 public:
    /**
     * @brief set weather the window is resizeable
     * @param resizeable
     * @details If resizeable is set to false, then the window can not be resized by mouse,
     *          but still can be resized programmatically
     */
    void setResizeable(bool resizeable);
    bool resizeable() const;

    /**
     * @brief set the width of the resizeable area
     * @param width
     * @details set the width of the resizeable area, inside this area, window can be resized by mouse
     */
    void setResizeableAreaWidth(int width);
    int resizeableAreaWidth() const;

 protected:
    /**
     * @brief set a widget which will be treated as SYSTEM title bar
     * @param titleBar
     */
    void setTitleBar(QWidget *titleBar);

    /**
     * @brief add a widget in title bar to blacklist
     * @param widget
     * @details In mose cases, we use a custom title bar, it may has some labels or buttons.
     *          In these cases, title bar will be covered by these widgets.
     *          So we need to check the widget under cursor. If it is in the blacklist, then
     *          we should not treat it as title bar. QAbstractButton and inherited classes are
     *          ignored by default.
     */
    void addBlacklistWidget(QWidget *widget);

    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

 public:
    void setContentsMargins(const QMargins &margins);
    void setContentsMargins(int left, int top, int right, int bottom);
    QMargins contentsMargins() const;
    QRect contentsRect() const;
    void getContentsMargins(int *left, int *top, int *right, int *bottom) const;

 public slots:
    void showFullScreen();

 private:
    class QtFramelessWindowPrivate;
    Q_DECLARE_PRIVATE(QtFramelessWindow);
    QtFramelessWindowPrivate *d_ptr;
};

#endif // QTFRAMELESSWINDOW_H
