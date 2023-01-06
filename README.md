# Qt-Nice-Frameless-Window

Qt Frameless Window for Windows, support Aero Snap, drop shadow on Windows. Based on QWidget.

基于 QMainWindow 实现的效果很好的 Qt 无边框窗口,支持 Windows 系统。在 Windows 上，支持窗口阴影、Aero 效果等。

# How to use
Just use class "QtFramelessWindow" as the base class instead of QWidget, and Enjoy!

# Windows Specific
- The window have no title bar by default, so we can not move the window around with mouse. Inorder to make the window moveable, protected member function **``` setTitleBar(QWidget* titlebar) ```** should be called in the MainWindow's Constructor, the widget "titlebar" should be a child widget of MainWindow, and it will act exactly same as SYSTEM Title Bar.

- Widget "titlebar" may has its own child widget, such as "close button" and "max button", and we can NOT move the window with "close button", which is what we want. However, a label widget "label1" on "titlebar" should not cover the moveable functionality, the protected member function **```addBlacklistWidget(QWidget* widget)```** is designed to deal with this kind of situation, just call **```addBlacklistWidget(ui->label1)```** in MainWindow's Constructor.

- **```setResizeableAreaWidth(int width = 5)```** can set width of an invisible border aera, inside this aera, window can be resized by mouse.

- **```setResizeable(bool resizeable)```** can set whether the window can be resized by mouse or not.

- Exampleforwindows shows the general usage.

- **WARNING**: resize()/geometry()/setGeometry()/frameGeometry()/frameSize()/x()/y()/.etc function will not work right when the Window is Maximized, try not to use any of these function when the Window is Maximized (If you really want to fix these bugs, feel free to contact [Bringer-of-Light](https://github.com/Bringer-of-Light)) . But these function setContentsMargins()/contentsMargins()/getContentsMargins/contentsRect() can always works right, so you can use them anytime. Generally , it's always a good idea to use Qt Layout Management System instead of hard-code layout Management to avoid potential size-like/pos-like issue.

# Platform
- Tested with Qt5.15.2.
- Tested on Windows 10, Windows 11 (with MINGW 8.1.0 and MINGW 11.2.0).
