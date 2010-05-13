/*
 * src/main.cpp
 *
 * Copyright (c) 2009 Technische Universität Berlin, 
 * Stranski-Laboratory for Physical und Theoretical Chemistry
 *
 * This file is part of qSLDcalc.
 *
 * qSLDcalc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * qSLDcalc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with qSLDcalc. If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * Author(s) of this file:
 * Ingo Bressler (ingo at cs.tu-berlin.de)
 */

#include <QTranslator>
#include <cfp/cfp.h>
#include "mainwindow.h"
#include "elementdatabase.h"

int main(int argc, char *argv[])
{
	// loading data from embedded ressource file system
	ElementDatabase db;
	db.addFromDirectory(":/data");

	// create, show and execute the main window
	QApplication app(argc, argv);
	MainWindow mw(app, db);
	mw.show();
	return app.exec();
}
