///////////////////////////////////////////////////////////////////////////////
// Name:        wxgui.h
// Purpose:     Header for wxGui classes and implimentations.
// Author:      Mark Clayton <mark_clayton@users.sourceforge.net>
// Created:     Sept, 30 2012
// Copyright:   (c) John Mark Clayton
// Licence:     GPL Ver 3: See the COPYING file that came with this package
///////////////////////////////////////////////////////////////////////////////

#include <wx/scrolwin.h>
#include <wx/dialog.h>
#include <wx/propdlg.h>
#include <wx/generic/propdlg.h>


/*
defaults:
    mapurls
        mapurl http://www.openstreetmap.org/?lat=%f&lon=%f&zoom=15&layers=M
        mapurl http://www.mytopo.com/maps/?lat=%f&lon=%f&z=15
        mapurl http://www.mapquest.com/maps/map.adp?latlongtype=decimal&latitude=%f&longitude=%f
    launchcmds
        "internal"
        "/Applications/Firefox.app/Contents/MacOS/firefox %s"
        "mozilla -remote openURL(%s, new-window)"
        "netscape -remote openURL(%s, new-window)"
        "kfmclient openURL %s"
        "opera -newwindow %s"
        "exo-open --launch WebBrowser %s"
        "open /Applications/Safari.app %s"




*/


