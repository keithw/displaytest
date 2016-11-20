#include <cstdlib>
#include <chrono>
#include <thread>
#include <iostream>
#include <sstream>

#include "display.hh"

using namespace std;

int main()
{
  /* construct a window */
  XWindow window( 1280, 720 );
  window.set_name( "RGB example" );

  /* put the window on the screen */
  window.map();

  /* construct a picture (pixmap) */
  XPixmap picture( window );


  auto visual_type = picture.xcb_visual();
  cout << "Color masks:" << hex
       << " red=" << visual_type->red_mask
       << " green=" << visual_type->green_mask
       << " blue=" << visual_type->blue_mask
       << "\n";

  /* make sure it's R'G'B' 8-bits-per-channel */
  if ( visual_type->bits_per_rgb_value != 8 ) {
    throw runtime_error( string( "Needed 8 bits-per-color, got " )
			 + to_string( visual_type->bits_per_rgb_value )
			 + " instead" );
  }

  /* make sure the colors are where we expect them */
  if ( visual_type->red_mask != 0xFF0000
       or visual_type->green_mask != 0x00FF00
       or visual_type->blue_mask != 0x0000FF ) {
    ostringstream color_layout;
    color_layout << "Unexpected color layout: ";
    color_layout << hex << "red=" << visual_type->red_mask;
    color_layout << hex << ", green=" << visual_type->green_mask;
    color_layout << hex << ", blue=" << visual_type->blue_mask;
    throw runtime_error( color_layout.str() );
  }

  /* now draw something on the pixmap */
  /* probably will need an xcb_image_t and then draw it with
     xcb_image_put */

  return EXIT_SUCCESS;
}
