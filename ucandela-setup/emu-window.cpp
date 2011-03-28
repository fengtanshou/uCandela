#include "emu-window.h"
#include <QtGui>

EmulatorWindow::EmulatorWindow(QWidget *parent)
	: QWidget(parent)
	, slider(0)
	, periodic(0)
{
	if ( QVBoxLayout *layout = new QVBoxLayout() )
	{
		if ( (slider = new QSlider(Qt::Horizontal, this)) )
		{
			slider->setMinimum(0);
			slider->setMaximum(65535);
			connect(slider, SIGNAL(valueChanged(int)),
				this, SIGNAL(valueUpdated(int)));
			layout->addWidget(slider);
		}
		setLayout(layout);
	}
	setWindowTitle(tr("Sensor emulator"));

	// set up timer
	periodic = new QTimer();
	connect(periodic, SIGNAL(timeout()), this, SLOT(tick()));
//	periodic->setInterval(100);
	periodic->start(100);
}

void EmulatorWindow::tick(void)
{
	valueUpdated( slider->value() );
}
