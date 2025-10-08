 *******************************************************************************
 *          		~- Full Project V1 -~


 * VERY IMPORTANT, MUST READ!!!!!
 * Before compiling go to lv_conf.h change 0 to 1 in line 341:
 * #define LV_FONT_MONTSERRAT_32 0



 * The rest is not very important, just some explanations.

 * High Level description:
 
 * A working game with image, letter buttons and timer bar
 * The timer bar is set to 30 seconds, by which you'll need
 * to click the letter corresponding to the first letter of
 * the image. Or else, you failed, a handler is called, the
 * timer reset and image skipped. If you succseed, you pass
 * on to the following image and the timer is resets.

 * Images are loaded from the MicroSDCard and you only need
 * to add an image to the S:/images folder, with no updates
 * to the code. Make sure the file/image name is lowercase.



 * Implementation detail:
 
 * The global variable "game" of type class Game represents
 * the object which contains:
 *  1) lv_obj_t *current_image_obj_m - current image object
 *  2) lv_obj_t *timer_bar_obj_m - the timer (progress bar)
 *  3) vector<string> image_name_list_m - list of all image
       names on SD Card in directory S:/images
 *  4) int image_pos_m - the current image name from vector
 *     with which you can get the required first letter



 * Next few steps to do:

 *  1) Pop-up message for failure or success on a  question
 *  2) Speaker with buttons for reading name & first letter
 *  3) Image & timer bar handlers and Game class methods in
       separate header files, as to clean code and features
 ******************************************************************************
