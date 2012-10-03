///////////////////////////////////////////////////////////////////////////////
// Name:        wxgui.h
// Purpose:     Header for wxGui classes and implimentations.
// Author:      Mark Clayton <mark_clayton@sourceforge.com>
// Created:     Sept, 30 2012
// Copyright:   (c) John Mark Clayton
// Licence:     GPL Ver 3: See the COPYING file that came with this package
///////////////////////////////////////////////////////////////////////////////

#include <wx/scrolwin.h>
#include <wx/dialog.h>
#include <wx/propdlg.h>
#include <wx/generic/propdlg.h>


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


    int xH, yH;

private:
//    wxBitmap m_bmpSmileXpm;
//    wxIcon   m_iconSmileXpm;

    DECLARE_EVENT_TABLE()
};

class RenderTimer : public wxTimer
{
    MyCanvas* canvas;
    
public:
    RenderTimer(MyCanvas* canvas);
    void Notify();
    void start();
};



class SettingsDialog: public wxPropertySheetDialog
{
DECLARE_CLASS(SettingsDialog)
public:
    SettingsDialog(wxWindow* parent, int dialogType);
    ~SettingsDialog();

#if 0
    wxPanel* CreateGeneralSettingsPage(wxWindow* parent);
    wxPanel* CreateAestheticSettingsPage(wxWindow* parent);
#endif

protected:
    enum {
        ID_SHOW_TOOLTIPS = 100,
        ID_AUTO_SAVE,
        ID_AUTO_SAVE_MINS,
        ID_LOAD_LAST_PROJECT,

        ID_APPLY_SETTINGS_TO,
        ID_BACKGROUND_STYLE,
        ID_FONT_SIZE
    };

//    wxImageList*    m_imageList;

DECLARE_EVENT_TABLE()
};


class MyBasicDialog: public wxDialog
{
public:
 
    MyBasicDialog ( wxWindow * , wxWindowID , wxString const & , 
                        wxPoint const & , wxSize const & , long );

    wxTextCtrl * dialogText;
    wxString GetText();
 
private:
 
    void OnOk( wxCommandEvent & event );
 
    DECLARE_EVENT_TABLE()
};


