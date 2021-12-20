#ifndef __BITCOIN_BASE_CLASSES_H__
#define __BITCOIN_BASE_CLASSES_H__

#include "uBitcoin_conf.h"
#include "Conversion.h"

#include <stdint.h>
#include <string.h>

enum encoding_format{
    RAW = 0,
    HEX_ENCODING = 16
    // TODO: would be nice to have base64 encoding here
};

enum parse_status{
    PARSING_DONE = 0,
    PARSING_INCOMPLETE = 1,
    PARSING_FAILED = 2
};
// error codes struct / class?

class Streamable;

class ParseStream{
public:
    virtual size_t available(){ return 0; };
    virtual int read(){ return -1; };
    virtual size_t read(uint8_t *arr, size_t length){ return 0; };
    virtual int getLast(){ return -1; };
    size_t parse(Streamable * s);
};

// TODO: skip leading non-hex if it's hex format
class ParseByteStream: public ParseStream{
private:
    const uint8_t * buf;
    size_t cursor;
    size_t len;
    encoding_format format;
    int last;
public:
    ParseByteStream(const uint8_t * arr, size_t length, encoding_format f=RAW);
    ParseByteStream(const char * arr, encoding_format f=HEX_ENCODING);
    ~ParseByteStream();
    size_t available();
    int read();
    size_t read(uint8_t *arr, size_t length);
    virtual int getLast();
};

class SerializeStream{
public:
    virtual size_t available(){ return 0; };
    virtual size_t write(uint8_t b){ return 0; };
    virtual size_t write(const uint8_t *arr, size_t len){ return 0; };
    size_t serialize(const Streamable * s, size_t offset);
};

class SerializeByteStream: public SerializeStream{
private:
    uint8_t * buf;
    size_t cursor;
    size_t len;
    encoding_format format;
public:
    SerializeByteStream(uint8_t * arr, size_t length, encoding_format f=RAW);
    explicit SerializeByteStream(char * arr, size_t length, encoding_format f=HEX_ENCODING);
    size_t available();
    size_t write(uint8_t b);
    size_t write(const uint8_t *arr, size_t length);
};

/** Abstract Readable class that can be encoded as a string and displayed to the user
 *  Can be converted to and from a string (char *, Arduino String and std::string)
 *  In Arduino it can be directly printed to the serial port, display or other Print device
 */
#ifdef ARDUINO
class Readable: public Printable{
#else
class Readable{
#endif
private:
protected:
    /* override these functions in your class */
    virtual size_t to_str(char * buf, size_t len) const = 0;
    virtual size_t from_str(const char * buf, size_t len) = 0;
public:
    /* override these function in your class */
    virtual size_t stringLength() const = 0;

    size_t toString(char * buf, size_t len) const{ return this->to_str(buf, len); };
    size_t fromString(const char * buf, size_t len){ return this->from_str(buf, len); };
    size_t fromString(const char * buf){ return this->from_str(buf, strlen(buf)); };
#ifdef ARDUINO
    size_t printTo(Print& p) const;
#endif
#if USE_ARDUINO_STRING
    String toString() const;
    operator String(){ return this->toString(); };
#endif
#if USE_STD_STRING
    std::string toString() const;
    operator std::string(){ return this->toString(); };
#endif
};

/** Abstract Streamable class that can be serialized to/from a sequence of bytes
 *  and sent to Stream (File, Serial, Bluetooth) without allocating extra memory
 *  Class can be parsed and serialized in raw and hex formats
 */
class Streamable: public Readable{
    friend class SerializeStream;
    friend class ParseStream;
private:
protected:
    virtual size_t from_stream(ParseStream *s) = 0;
    virtual size_t to_stream(SerializeStream *s, size_t offset) const = 0;
    virtual size_t to_str(char * buf, size_t len) const{
        return serialize(buf, len);
    }
    virtual size_t from_str(const char * buf, size_t len){
        return parse(buf, len);
    }
    parse_status status;
    size_t bytes_parsed;
public:
    Streamable() { status = PARSING_DONE; bytes_parsed = 0; };
    virtual void reset(){ status = PARSING_DONE; bytes_parsed = 0; }; // used to reset parsing and mb object
    virtual size_t length() const = 0;
    virtual size_t stringLength() const{ return 2*length(); };
    /** \brief Gets parsing status. 
     *         PARSING_DONE - everything is ok, 
     *         PARSING_INCOMPLETE - some data is still missing
     *         PARSING_FAILED - something went wrong, the data is probably incorrect.
     */
    parse_status getStatus(){ return status; };
    /** \brief Sets parsing status. Should be used with care. */
    void setStatus(parse_status s){ status = s; };
    size_t parse(const uint8_t * arr, size_t len, encoding_format format=RAW){
        ParseByteStream s(arr, len, format);
        return from_stream(&s);
    }
#if USE_ARDUINO_STRING
    size_t parse(String arr, encoding_format format=HEX_ENCODING){
        return parse(arr.c_str(), strlen(arr.c_str()), format);
    }
#endif
#if USE_STD_STRING
    size_t parse(std::string arr, encoding_format format=HEX_ENCODING){
        return parse(arr.c_str(), strlen(arr.c_str()), format);
    }
#endif
#if !(USE_ARDUINO_STRING  || USE_STD_STRING)
    size_t parse(const char * arr, encoding_format format=HEX_ENCODING){
        return parse(arr, strlen(arr), format);
    }
#endif
    size_t serialize(uint8_t * arr, size_t len, size_t offset = 0, encoding_format format=RAW) const;
    size_t parse(const char * arr, size_t len, encoding_format format=HEX_ENCODING){
        return parse((const uint8_t *) arr, len, format);
    }
    size_t serialize(char * arr, size_t len, size_t offset = 0, encoding_format format=HEX_ENCODING) const{
        return serialize((uint8_t *)arr, len, offset, format);
    }
#if USE_ARDUINO_STRING
    String serialize(size_t offset=0, size_t len=0) const;
#endif
#if USE_STD_STRING
    std::string serialize(size_t offset=0, size_t len=0) const;
    std::string serialize(int offset, int len) const{
        return serialize((size_t)offset, (size_t)len);
    };
    std::string serialize(size_t offset, int len) const{
        return serialize((size_t)offset, (size_t)len);
    };
#endif
};

#endif // __BITCOIN_BASE_CLASSES_H__
