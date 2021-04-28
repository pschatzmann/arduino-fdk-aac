/**
 * @brief Some Generic Tools which might be useful: 
 * We provide 
 * - a Buffer and Double Buffer that can be used e.g. to manage PCM data
 * - a stream copy functionality
 * - an in memory Stream
 */

#pragma once
#include "Arduino.h"
#define DEFAULT_SAMLE_RATE 44100


/**
 * @brief Sound information which is available in the WAV header
 * 
 */
struct AudioInfo {
    int format;
    int sample_rate;
    int bits_per_sample;
    int channels;
    int byte_rate;
    int block_align;
    bool is_streamed;
    bool is_valid;
    uint32_t data_length;
    uint32_t file_size;
};

class AudioInfoSource {
    public:
         virtual AudioInfo &audioInfo() = 0;
};

class AudioInfoTarget {
    public:
         virtual void setAudioInfo(AudioInfoSource &ai) = 0;
};

class AudioOut : public Stream, public AudioInfoTarget {

};


/**
 * @brief Logger
 * 
 */
class AudioLogCls {
    public:
        void begin(Stream &stream){
            stream_ptr = &stream;
        }
        void info(const char* msg, const char *msg1=nullptr){
            if (stream_ptr!=nullptr){
                stream_ptr->print("Info: ");
                stream_ptr->print(msg);
                if (msg1!=nullptr)
                    stream_ptr->print(msg1);
                stream_ptr->println();
            }
        }
        void info(const char* msg, int code){
            if (stream_ptr!=nullptr){
                stream_ptr->print("Info: ");
                stream_ptr->print(msg);
                stream_ptr->print(code);
                stream_ptr->println();
            }
        }
        void error(const char* msg, const char* msg1=nullptr){
            if (stream_ptr!=nullptr){
                stream_ptr->print("Error: ");
                stream_ptr->print(msg);
                if (msg1!=nullptr)
                    stream_ptr->print(msg1);
                stream_ptr->println();
            }
        }
        void error(const char* msg, int code){
            if (stream_ptr!=nullptr){
                stream_ptr->print("Error: ");
                stream_ptr->print(msg);
                stream_ptr->print(code);
                stream_ptr->println();
            }
        }

    protected:
        Stream *stream_ptr;
} AudioLogger;

/**
 * @brief a simple single buffer
 * 
 * @tparam T 
 */
template <class T>
class SingleBuffer {
  public:
    SingleBuffer(size_t size) {
        if (size>0){
            max_size = size;
            buffer = new T[size];
        }
    }

    ~SingleBuffer(){
        if (buffer!=nullptr)
            delete[] buffer;
    }

    bool write(T value){
        bool result = false;
        buffer[buffer_pos]=value;
        if (buffer_pos<max_size) {
            buffer_pos++;
            result = true;
        } else {
            AudioLogger.error("Buffer Overflow");
        }
        return result;
    }

    void read(T* &buffer, int &len){
        len = buffer_pos;
        buffer = buffer;
    }

    // number of entries
    size_t size() {
        return max_size;
    }

    // size in bytes
    size_t sizeBytes() {
        return max_size*sizeof(T);
    }

    // reset the buffer position
    void reset(){
        buffer_pos=0;
    }

    // provides direct access to the buffer data
    T* data(){
        return buffer;
    }

    size_t available() {
        return buffer_pos;
    }

    size_t availableBytes() {
        return buffer_pos*sizeof(T);
    }

    bool isFull() {
        return buffer_pos>=max_size;
    }

  protected:
    size_t max_size = 1024;
    size_t buffer_pos;
    T* buffer = nullptr;

};

/**
 * @brief A double buffer
 * 
 * @tparam T 
 */
template <class T>
class DoubleBuffer {
    public:
        DoubleBuffer(size_t size){
            buffer[0] = new SingleBuffer<T>(size);
            buffer[1] = new SingleBuffer<T>(size);
        }

        ~DoubleBuffer(){
            delete buffer[0];
            delete buffer[1];
        }

        bool write(T value){
            lock();
            bool result = buffer[selected_buffer]->write(value);
            unlock();
            return result;
        }

        void read(T* &data, int &len){
            buffer[selected_buffer]->read(data,len);
            changeBuffer();
        }

    protected:
        SingleBuffer<T> *buffer[2];
        int selected_buffer;
        portMUX_TYPE bufferMux = portMUX_INITIALIZER_UNLOCKED;

        void changeBuffer(){
            lock();
            selected_buffer = !selected_buffer;  //switch between 0 and 1
            buffer[selected_buffer]->reset();    //we restart to write from the beginning
            unlock();
        }

        inline void lock(){
            portENTER_CRITICAL_ISR(&bufferMux);
        }

        inline void unlock(){
            portEXIT_CRITICAL_ISR(&bufferMux);
        }

};

/**
 * @brief Helper class which Copies an input stream to a output stream
 * 
 */
class StreamCopy {
    public:
        StreamCopy(Stream &from, Stream &to, int buffer_size){
            this->from = &from;
            this->to = &to;
            this->buffer_size = buffer_size;
            buffer = new uint8_t[buffer_size];
        }

        ~StreamCopy(){
            delete[] buffer;
        }

        /// copies the available bytes from the input stream to the ouptut stream
        size_t copy() {
            size_t total_bytes = available();
            size_t result = total_bytes;
            while (total_bytes>0){
                size_t bytes_to_read = min(total_bytes,static_cast<size_t>(buffer_size) );
                from->readBytes(buffer, bytes_to_read);
                to->write(buffer, bytes_to_read);
                total_bytes -= bytes_to_read;
            }
            return result;
        }

        int available() {
            return from->available();
        }

    protected:
        Stream *from;
        Stream *to;
        uint8_t *buffer;
        int buffer_size;

};

/**
 * @brief A simple Stream implementation which is backed by allocated memory
 * 
 */
class MemoryStream : public Stream {
    public: 
        MemoryStream(int buffer_size = 512){
            this->buffer_size = buffer_size;
            this->buffer = new uint8_t[buffer_size];
            this->owns_buffer = true;
        }

        MemoryStream(const uint8_t *buffer, int buffer_size){
            this->buffer_size = buffer_size;
            this->write_pos = buffer_size;
            this->buffer = (uint8_t*)buffer;
            this->owns_buffer = false;
        }

        ~MemoryStream(){
            if (owns_buffer)
                delete[] buffer;
        }

        virtual size_t write(uint8_t byte) {
            int result = 0;
            if (write_pos<buffer_size){
                result = 1;
                buffer[write_pos] = byte;
                write_pos++;
            }
            return result;
        }

        virtual int available() {
            return write_pos - read_pos;
        }

        virtual int read() {
            int result = peek();
            if (result>0){
                read_pos++;
            }
            return result;
        }

        virtual int peek() {
            int result = -1;
            if (available()>0 && read_pos < write_pos){
                result = buffer[read_pos];
            }
            return result;
        }

        virtual void flush(){
        }

        void clear(bool reset=false){
            write_pos = 0;
            read_pos = 0;
            if (reset){
                // we clear the buffer data
                memset(buffer,0,buffer_size);
            }
        }


    protected:
        int write_pos;
        int read_pos;
        int buffer_size;
        uint8_t *buffer;
        bool owns_buffer;

};

/**
 * @brief Convert autio data 
 * 
 */
template <class InT, class OutT>
class Filter {
   public:
    Filter(float scale=1.0, float offset=0){
        this->scale = scale;
        this->offset = offset;
    }

    virtual OutT apply(InT value) {
        return value * scale + offset;
    };

   protected:
    float scale=1.0;
    float offset=0;
};

/**
 * @brief Output class which applys a filter before wriging the data to the final
 * stream
 * 
 * @tparam InT 
 * @tparam OutT 
 */

template <class InT, class OutT>
class FilteredStream {
    public:
        FilteredStream(Stream &out, Filter<InT,OutT> &filter){
            this->out = &out;
            this->filter = filter;
        }

        void write(void* data, int size){
            if (sizeof(InT)<=sizeof(OutT)){
                InT *in_data_ptr = data;
                OutT *out_data_ptr = data;
                int len = size / sizeof(InT);
                for (int j=0;j<len;j++){
                    out_data_ptr[j] = filter->apply(in_data_ptr[j]);
                }
                out->write((uint8_t*)data, size * sizeof(InT)/sizeof(OutT));
            } else {
                AudioLogger.error("Filtering not possible - The input size must be >= output size");
                out->write((uint8_t*)data, size);
            }
        }

    protected:
        Stream *out;
        Filter<InT,OutT> *filter;

};

/**
 * @brief Generates a Sound with the help of sin() function.
 * 
 */
template <class T>
class SineWaveGenerator : public AudioInfoSource {
    public:
        // the scale defines the max value which is generated
        SineWaveGenerator(double scale=1.0) {
            this->scale = scale;
        }

        void begin(uint16_t sample_rate=44100, uint16_t frequency=0){
            this->frequency = frequency;
            ai.sample_rate = sample_rate;
            ai.channels = 1;
            ai.bits_per_sample = sizeof(T)*8;
        }

        void setFrequency(uint16_t frequency) {
            this->frequency = frequency;
        }

        virtual size_t readBytes(uint8_t *buffer, size_t length){
            T *data = (T*) buffer;
            int size = length / sizeof(T);

            for (int j=0;j<size;j++){
                T value = sin(2.0 * PI * sample_step++ * frequency / ai.sample_rate) * scale;
                data[j] = value;
            }
            size_t result = size * sizeof(T);
            AudioLogger.info("size: ",result);
          
            return result;
        }
        
        AudioInfo &audioInfo() {
            return ai;
        }

    protected:
        AudioInfo ai;
        uint32_t sample_step;
        uint16_t frequency;
        double scale;
};
