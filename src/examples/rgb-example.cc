#include <cstdlib>
#include <chrono>
#include <thread>

#include "display.hh"

using namespace std;

int main()
{
  XWindow window( 1280, 720 );
  window.set_name( "RGB example" );

  window.map();

  this_thread::sleep_for( chrono::seconds( 2 ) );

  return EXIT_SUCCESS;
}
