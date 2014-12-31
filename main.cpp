/*
 * Copyright (c) 2014, Jumpnow Technologies, LLC
 *
 */

#include "qtsonycam.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	QtSonyCam w;
	w.show();
	return a.exec();
}
