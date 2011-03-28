#include "chartwidget.h"
#include <QtGui>

#define DEFAULT_HISTORY 200 // ticks

ChartWidget::ChartWidget(QWidget *parent)
	: QWidget(parent)
	, plot(0), plotview(0), curve(0)
{
	setAutoFillBackground(true);

	if ( QVBoxLayout *l = new QVBoxLayout() )
	{
		if ( (plot = new Plot::Plot2D())
		     && (plotview = new Plot::Plot2DView(this)) )
		{
			plotview->setPlot(plot);
			l->addWidget(plotview);

			curve = new Plot::Plot2DCurveRR();
			curve->setHistory(DEFAULT_HISTORY);
			plot->addCurve(curve);
		}
		setLayout(l);
	}

}

ChartWidget::~ChartWidget()
{
	delete plot;
	delete plotview;
	delete curve;
}

void ChartWidget::addDataPoint(unsigned long tick, int value)
{
	Plot::DataPoint point(tick, value/65536.0f);
	curve->appendData(point);
}
