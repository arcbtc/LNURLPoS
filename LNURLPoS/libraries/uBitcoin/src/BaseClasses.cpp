#include "uBitcoin_conf.h"
#include "BaseClasses.h"

#if USE_STD_STRING
using std::string;
#define String string
#endif

size_t SerializeStream::serialize(const Streamable * s, size_t offset){
    return s->to_stream(this, offset);
}

size_t ParseStream::parse(Streamable * s){
    return s->from_stream(this);
}
/************ Parse Byte Stream Class ************/

ParseByteStream::ParseByteStream(const uint8_t * arr, size_t length, encoding_format f){
    last = -1;
    format = f;
    cursor = 0;
    len = (arr == NULL) ? 0 : length;
    buf = arr;
}
// TODO: call prev constructor
ParseByteStream::ParseByteStream(const char * arr, encoding_format f){
    last = -1;
    format = f;
    cursor = 0;
    len = (arr == NULL) ? 0 : strlen(arr);
    buf = (const uint8_t *) arr;
}
ParseByteStream::~ParseByteStream(){
    buf = NULL;
}
size_t ParseByteStream::available(){
    if(format == HEX_ENCODING){
        return (len - cursor)/2;
    }else{
        return len-cursor;
    }
};
int ParseByteStream::read(){
    if(format == HEX_ENCODING){
        if(cursor < len-1){
            uint8_t c1 = hexToVal(buf[cursor]);
            uint8_t c2 = hexToVal(buf[cursor+1]);
            if(c1 < 0x10 && c2 < 0x10){
                cursor +=2;
                last = (c1<<4) + c2;
                return last;
            }
        }
    }else{
        if(cursor < len){
            uint8_t c = buf[cursor];
            cursor ++;
            last = c;
            return c;
        }
    }
    return -1;
}
int ParseByteStream::getLast(){
    return last;
}
size_t ParseByteStream::read(uint8_t *arr, size_t length){
    size_t cc = 0;
    while(cc<length){
        int b = read();
        if(b<0){
            return cc;
        }
        arr[cc] = (uint8_t)b & 0xFF;
        cc++;
    }
    return cc;
}

/************ Serialize Byte Stream Class ************/

SerializeByteStream::SerializeByteStream(uint8_t * arr, size_t length, encoding_format f){
    format = f; cursor = 0; buf = arr; len = length;
    memset(arr, 0, length);
}
// TODO: should length be here? See above - we used strlen
SerializeByteStream::SerializeByteStream(char * arr, size_t length, encoding_format f){
    format = f; cursor = 0; buf = (uint8_t *)arr; len = length;
    memset(arr, 0, length);
};
size_t SerializeByteStream::available(){
    size_t a = len-cursor;
    if(format == HEX_ENCODING){
        a = a/2;
    }
    return a;
};
size_t SerializeByteStream::write(uint8_t b){
    if(available() > 0){
        if(format == HEX_ENCODING){
            buf[cursor] = ((b >> 4) & 0x0F) + '0';
            if(buf[cursor] > '9'){
                    buf[cursor] += 'a'-'9'-1;
            }
            cursor++;
            buf[cursor] = (b & 0x0F) + '0';
            if(buf[cursor] > '9'){
                    buf[cursor] += 'a'-'9'-1;
            }
            cursor++;
        }else{
            buf[cursor] = b;
            cursor++;
        }
        return 1;
    }
    return 0;
};
size_t SerializeByteStream::write(const uint8_t *arr, size_t length){
    size_t l = 0;
    while(available()>0 && l < length){
        write(arr[l]);
        l++;
    }
    return l;
};

/************ Readable Class ************/

#ifdef ARDUINO
size_t Readable::printTo(Print& p) const{
    size_t len = this->stringLength()+1;
    char * arr = (char *)calloc(len, sizeof(char));
    toString(arr, len);
    p.print(arr);
    free(arr);
    return len-1;
}
#endif
#if USE_ARDUINO_STRING || USE_STD_STRING
String Readable::toString() const{
    size_t len = this->stringLength()+1;
    char * arr = (char *)calloc(len, sizeof(char));
    toString(arr, len);
    String s = arr;
    free(arr);
    return s;
};
#endif

/************ Streamable Class ************/

#if USE_ARDUINO_STRING || USE_STD_STRING
String Streamable::serialize(size_t offset, size_t len) const{
    if(len == 0){
        len = (length()-offset);
    }
    char * arr = (char *)calloc(2*len+1, sizeof(char));
    serialize(arr, 2*len, offset, HEX_ENCODING);
    String s = arr;
    free(arr);
    return s;
};
#endif

size_t Streamable::serialize(uint8_t * arr, size_t len, size_t offset, encoding_format format) const{
    SerializeByteStream s(arr, len, format);
    return to_stream(&s, offset);
}

