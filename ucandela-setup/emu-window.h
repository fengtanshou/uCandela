#ifndef EMU_WINDOW_H_INC
#define EMU_WINDOW_H_INC

#include <QWidget>

class QSlider;
class QTimer;

class EmulatorWindow : public QWidget
{
	Q_OBJECT;
public:
	EmulatorWindow(QWidget *parent=0);

signals:
	void valueUpdated(int value);

private slots:
	void tick();

private:
	QSlider *slider;
	QTimer *periodic;
};

#endif // EMU_WINDOW_H_INC

