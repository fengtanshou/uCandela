#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class ChartWidget;

namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow
{
	Q_OBJECT;
public:
	typedef int process_type;

	// constructors
	MainWindow();

	// getters
	inline process_type processValue() const
	{
		return last_process_value;
	}


public slots:
	void valueChanged(int value);

protected:
	inline void setProcessValue(process_type new_value)
	{
		last_process_value = new_value;
	}

	void recalcGraphics(void);

protected: // events
	//void resizeEvent(QResizeEvent *event);

private: // data
	process_type last_process_value;

private: // internal mechanics
	Ui::MainWindow *pui;
};

#endif /* MAINWINDOW_H */
