#ifndef __UBITCOIN_CONF_H__
#define __UBITCOIN_CONF_H__

/* Change this if you want to have other network by default */
#define DEFAULT_NETWORK Mainnet

/* Change this config file to adjust to your framework */
#ifndef ARDUINO
#define MBED
#include <mbed.h>
#else
#include <Arduino.h>
#endif

/* If you don't have a Stream class in your framework you can implement one
 * by yourself and use it to parse transactions and hash on the fly.
 * Arduino and Mbed are using slightly different API, choose one.
 * TODO: describe the interface.
 */

/* settings for Arduino */
#ifdef ARDUINO
#define USE_ARDUINO_STRING 1 /* Arduino String implementation (WString.h) */
#define USE_ARDUINO_STREAM 1 /* Arduino Stream class */
#define USE_STD_STRING     0 /* Standard library std::string */
#define USE_MBED_STREAM    0 /* Mbed Stream class */
#endif

#ifdef MBED
#define USE_ARDUINO_STRING 0 /* Arduino String implementation (WString.h) */
#define USE_ARDUINO_STREAM 0 /* Arduino Stream class */
#define USE_STD_STRING     1 /* Standard library std::string */
#define USE_MBED_STREAM    1 /* Mbed Stream class */
#endif

#if USE_STD_STRING
#include <string>
// using std::string;
#endif

#endif //__UBITCOIN_CONF_H__
