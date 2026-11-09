#include "src/frame_timing.h"
#include <stdexcept>
#include <iostream>
bool foreground=true; int GetForegroundWindow(){return foreground?1:2;}
double frame(double spent,bool powerSaver,bool& previousMeasured){int window=1;
   const bool pacingFocused=GetForegroundWindow()==window;
   if(!pacingFocused)previousMeasured=false; // Exclude a background wait even if focus returns before the next frame.
   const double delay=timing::frameDelay(spent,powerSaver,pacingFocused);
return delay;}
int main(){bool measured=true;foreground=false;if(frame(.005,false,measured)!=.095||measured)throw std::runtime_error("Background delay must invalidate prior sample"); foreground=true;if(frame(.005,false,measured)!=0||measured)throw std::runtime_error("Recovery cannot relabel background interval");measured=true;if(frame(.01,true,measured)!=.04||!measured)throw std::runtime_error("Foreground saver policy");std::cout<<"Actual pacing statements: background wait invalidates measurement; focus recovery skips interval; foreground saver preserved. CPU stand-in only.\n";}
