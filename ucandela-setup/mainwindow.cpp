#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "chartwidget.h"
#include <QtGui>

#define HIDDEV_VALUE_MAX 65535

MainWindow::MainWindow()
	: QMainWindow()
	, last_process_value(0)
	, pui(new Ui::MainWindow)
{
	pui->setupUi(this);

	recalcGraphics();
}

void MainWindow::recalcGraphics()
{
	// initialize gradient
	QRadialGradient gradient(0.5f, 1.0f, 1.0f);
#if 1
	gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
#else
	gradient.setCoordinateMode(QGradient::StretchToDeviceMode);
#endif
	enum Qt::GlobalColor colors[] = {Qt::black, Qt::red, Qt::yellow, Qt::white};
	const size_t num_colors = sizeof(colors)/sizeof(colors[0]);
	const qreal abs_position =  (num_colors-1) * (qreal)processValue() / HIDDEV_VALUE_MAX;
	const size_t numc1 = floor(abs_position);
	const size_t numc2 = ceil(abs_position);
	const qreal rel_position = abs_position - numc1;
	gradient.setColorAt(std::min(1.0, rel_position*2.0f - 0.0f), colors[numc1]);
	gradient.setColorAt(std::max(0.0, rel_position*2.0f - 1.0f), colors[numc2]);

	// assign gradient to chart widget
	QPalette pal = pui->chart->palette();
	pal.setBrush(QPalette::Background, QBrush(gradient));
	pui->chart->setPalette(pal);
}

void MainWindow::valueChanged(int value)
{
	static unsigned long tick = 0;

	setProcessValue(value);
	recalcGraphics();

	pui->chart->addDataPoint(tick++, value);
}
