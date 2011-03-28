#ifndef CHART_H_INC
#define CHART_H_INC

#include <QWidget>
#include "plot2d.h"

class ChartWidget : public QWidget
{
	Q_OBJECT;
public:
	explicit ChartWidget(QWidget *parent);
	~ChartWidget();

	void addDataPoint(unsigned long tick, int value);

private:
	Plot::Plot2D *plot;
	Plot::Plot2DView *plotview;
	Plot::Plot2DCurveRR *curve;
};

#endif /* CHART_H_INC */
