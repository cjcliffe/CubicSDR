#include <wx/wx.h>
#include <wx/sizer.h>
 
class ImagePanel : public wxPanel {
	wxBitmap image;

public:
	ImagePanel(wxPanel* parent, wxString file, wxBitmapType format);

	void paintEvent(wxPaintEvent & evt);
	void paintNow();

	void render(wxDC& dc);

	DECLARE_EVENT_TABLE()
};
