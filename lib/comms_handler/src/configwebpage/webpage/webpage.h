
#ifndef WEBPAGE_GZIP_H
#define WEBPAGE_GZIP_H

/**
 * @brief Length variable for the HTML_GZ_PROGMEM array
 * 
 */
extern const size_t HTML_GZ_PROGMEM_LEN;

/**
 * @brief HTML_GZ_PROGMEM array containing the gzipped version of the webpage
 *          from the python preprocessor
 * 
 */
extern const uint8_t HTML_GZ_PROGMEM[] PROGMEM;

#endif
