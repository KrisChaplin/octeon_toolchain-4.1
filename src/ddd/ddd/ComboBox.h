// $Id: ComboBox.h,v 1.1.1.1 2006/04/11 21:46:36 cchavva Exp $ -*- C++ -*-
// Create a combo box

// Copyright (C) 1998 Technische Universitaet Braunschweig, Germany.
// Written by Andreas Zeller <zeller@gnu.org>.
// 
// This file is part of DDD.
// 
// DDD is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
// 
// DDD is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public
// License along with DDD -- see the file COPYING.
// If not, write to the Free Software Foundation, Inc.,
// 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// 
// DDD is the data display debugger.
// For details, see the DDD World-Wide-Web page, 
// `http://www.gnu.org/software/ddd/',
// or send a mail to the DDD developers <ddd@gnu.org>.

#ifndef _DDD_ComboBox_h
#define _DDD_ComboBox_h

#include <X11/Intrinsic.h>
#include "StringA.h"

// Create a combo box
Widget CreateComboBox(Widget parent, const _XtString name, ArgList args, Cardinal arg);

// Set the combo box value list
void ComboBoxSetList(Widget name, const StringArray& items);

// Access ComboBox members
Widget ComboBoxList(Widget text);
Widget ComboBoxText(Widget text);
Widget ComboBoxButton(Widget text);
Widget ComboBoxTop(Widget text);

#endif // _DDD_ComboBox_h
// DON'T ADD ANYTHING BEHIND THIS #endif
