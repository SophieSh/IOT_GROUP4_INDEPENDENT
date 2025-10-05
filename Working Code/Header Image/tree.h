#ifndef LOGO_H // Use a unique include guard based on the filename
#define LOGO_H

// This is required to define the structure LV_IMG_DECLARE uses
#include "lvgl.h" 

// This macro tells the compiler that the structure 'logo_img' 
// is defined elsewhere (in logo.c) and makes it visible for use.
LV_IMG_DECLARE(tree);

#endif /* LOGO_H */