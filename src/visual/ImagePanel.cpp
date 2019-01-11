#include "ImagePanel.h"
 
BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
EVT_PAINT(ImagePanel::paintEvent)
END_EVENT_TABLE()
 

ImagePanel::ImagePanel(wxPanel * parent, wxString file, wxBitmapType format) :
	wxPanel(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxFULL_REPAINT_ON_RESIZE) {
	image.LoadFile(file, format);
}

void ImagePanel::paintEvent(wxPaintEvent & /* evt */) {
    wxPaintDC dc(this);
    render(dc);
}
 

void ImagePanel::paintNow() {
    wxClientDC dc(this);
    render(dc);
}
 

void ImagePanel::render(wxDC&  dc) {

	double imagew = image.GetWidth();
	double imageh = image.GetHeight();

	wxSize destSize = dc.GetSize();

	double destw = destSize.GetWidth();
	double desth = destSize.GetHeight();

	double sf = 1.0, wf, hf;

	wf = destw / imagew;
	hf = desth / imageh;

	sf = (wf < hf)?wf:hf;

	double resulth = imageh * sf;
	double resultw = imagew * sf;

	dc.SetUserScale(sf, sf);
    dc.DrawBitmap( image, (destw/2 - resultw/2)/sf, (desth/2 - resulth/2)/sf, false );
}

 
 