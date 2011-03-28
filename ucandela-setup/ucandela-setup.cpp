#include <QApplication>
#include <cstdlib>
#include "mainwindow.h"
#include "emu-window.h"

int main(int argc, char **argv)
{
	Q_INIT_RESOURCE(ucandela_setup);
	QApplication app(argc,argv);
	MainWindow wnd;

#define EMULATE_SENSOR
#ifdef  EMULATE_SENSOR
	EmulatorWindow emu;
	QObject::connect(&emu, SIGNAL(valueUpdated(int)), &wnd, SLOT(valueChanged(int)));
//	QObject::connect(&wnd, SIGNAL(close()), &app, SIGNAL(closeAllWindows()));
//	QObject::connect(&emu, SIGNAL(close()), &app, SIGNAL(closeAllWindows()));
#endif

	wnd.show();
	emu.show();
	return app.exec();
}
