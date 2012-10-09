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
        "mozilla -remote openURL(%s, new-window)"
        "netscape -remote openURL(%s, new-window)"
        "kfmclient openURL %s"
        "opera -newwindow %s"
        "exo-open --launch WebBrowser %s"
        "open /Applications/Safari.app %s"




*/

class MyFrame;

//-----------------------------------------------------------------------------
// MyCanvas
//-----------------------------------------------------------------------------

class MyCanvas: public wxPanel
{
public:
    MyCanvas( wxWindow *parent, wxWindowID, const wxPoint &pos, 
              const wxSize &size );
    ~MyCanvas();

    void OnPaint( wxPaintEvent &event );
    void CreateAntiAliasedBitmap();

    wxBitmap  body_jpg;
    wxBitmap  bottom1_jpg;
    wxBitmap  bottom2_jpg;
    wxBitmap  bottom3_jpg;
    wxBitmap  bottom4_jpg;
    wxBitmap  bottom5_jpg;
    wxBitmap  bottom6_jpg;
    wxBitmap  bottom7_jpg;
    wxBitmap  bottom8_jpg;
    wxBitmap  rh_png;
    wxBitmap  top1_jpg;
    wxBitmap  top2_jpg;
    wxBitmap  top3_jpg;

    MyFrame *m_frame;
    int xH, yH;

private:
//    wxBitmap m_bmpSmileXpm;
//    wxIcon   m_iconSmileXpm;

    DECLARE_EVENT_TABLE()
};

class OwwlReaderTimer : public wxTimer
{
public:
    OwwlReaderTimer(MyFrame * canvas);
    void Notify();
    void start();

private:
    MyFrame * m_frame;

};

class RenderTimer : public wxTimer
{
public:
    RenderTimer(MyFrame * canvas);
    void Notify();
    void start();
private:
    MyFrame * m_frame;

};


