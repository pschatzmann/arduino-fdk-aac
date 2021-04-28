#pragma once
#ifdef ESP32 

#include "Arduino.h"
#include "AudioTools.h"
#include "esp_http_client.h"
#include "driver/i2s.h"
#include "freertos/queue.h"

/**
 * @brief Input from and Output to I2S using Arduino stream API for the ESP32
 * 
 */
class I2S : public AudioOut {
    public:
        I2S(int port=0, int buffer_size = 512) {
            this->buffer_size_in_bytes = buffer_size;
            this->port = (i2s_port_t)port;

            // set up default value for the i2s_config
            this->i2s_config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT; ;
            //this->i2s_config.communication_format = I2S_COMM_FORMAT_I2S_MSB;
            this->i2s_config.intr_alloc_flags = 0; // default interrupt priority
            this->i2s_config.dma_buf_count = 6;
            this->i2s_config.dma_buf_len = 512;
            this->i2s_config.use_apll = false;
        }
        
        ~I2S(){
            delete buffer_ptr;
            delete out_buffer_ptr;
            i2s_driver_uninstall(port);
        }

        // provides access to the I2S configuration so that we can modify the settings before we call begin()
        virtual i2s_config_t &config(){
            return i2s_config;;
        }

        // setup I2S
        virtual void begin(int mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN, long sampleRate=DEFAULT_SAMLE_RATE, int channels=1, int bitsPerSample=16){
            // setup i2s
            this->channels = channels;
            i2s_config.mode=static_cast<i2s_mode_t>(mode);
            i2s_config.bits_per_sample=static_cast<i2s_bits_per_sample_t>(bitsPerSample);
            i2s_config.sample_rate=sampleRate;
            i2s_driver_install(port, &this->i2s_config, 0, NULL);   //install and start i2s driver

            // setup internal dac PINS
            if (mode & I2S_MODE_DAC_BUILT_IN){
                i2s_set_pin(port, nullptr); //for internal DAC, this will enable both of the internal channels
            }
        }

        virtual void begin(int mode, AudioInfoSource &info){
            i2s_config.mode=static_cast<i2s_mode_t>(mode);
            setAudioInfo(info);
            i2s_driver_install(port, &this->i2s_config, 0, NULL);   //install and start i2s driver

            // setup internal dac PINS
            if (mode & I2S_MODE_DAC_BUILT_IN){
                i2s_set_pin(port, nullptr); //for internal DAC, this will enable both of the internal channels
            }
        }

        // reconfigure some data based on the file info
        virtual void setAudioInfo(AudioInfoSource &info ){
            // setup i2s
            AudioInfo ai = info.audioInfo(); 
            this->channels = ai.channels;
            i2s_config.bits_per_sample=static_cast<i2s_bits_per_sample_t>(ai.bits_per_sample);
            i2s_config.sample_rate=ai.sample_rate;
            logAudioInfo();
        }


        // define the I2S pins - if we use an external DAC
        void setPins(int bck_io_num,int ws_io_num,int data_out_num, int data_in_num) {
            static i2s_pin_config_t pin_config;
            pin_config.bck_io_num = bck_io_num;
            pin_config.ws_io_num = ws_io_num;
            pin_config.data_out_num = data_out_num;
            pin_config.data_in_num = data_in_num;
            i2s_set_pin(port, &pin_config);
        }

        // write single byte (buffered) to I2S - this should be avoided
        virtual size_t write(uint8_t byte) {
            // create buffer on first use - it might be that we do not needs this at all
            if (buffer_ptr==nullptr){
                this->buffer_ptr = new SingleBuffer<uint8_t>(buffer_size_in_bytes);
            }
            if (!buffer_ptr->write(byte)){
                flush();
                buffer_ptr->write(byte);
            }
        }

        // write array to I2S (Direct write w/o buffers!)
        virtual size_t write(const uint8_t *bytes, size_t len) {
            size_t i2s_bytes_written;
            size_t i2s_bytes_to_write;

            if (len==0){
                ESP_LOGE(TAG, "No Data!");
                return 0;
            }

            if (needsConversion()) {
                if (channels==0 || i2s_config.bits_per_sample==0 || i2s_config.sample_rate==0){
                    ESP_LOGE(TAG, "Audio Info missing!");
                    logAudioInfo();
                    return 0;
                }

                // allocate output buffer
                if (len > out_buffer_size){
                    if (out_buffer_ptr!=nullptr) 
                        delete out_buffer_ptr;
                    //len / 2bytes * 2channels
                    out_buffer_ptr = new SingleBuffer<int16_t>(len);
                    out_buffer_size = len;
                } 
                // reset buffer
                out_buffer_ptr->reset(); 

                // copy data to buffer
                int16_t *ptr = (int16_t *) bytes;
                int samples = len / sizeof(int16_t) / channels;
                for (int j=0; j < samples; j+=channels) {
                    int16_t in = ptr[j];
                    int16_t out = scale(in);
                    out_buffer_ptr->write(out);

                    // calculate value for 2nd channel
                    if (channels>1){
                        // we use the data
                        in = ptr[j+1];
                        out = scale(in);
                    } 
                    //Serial.println(out);
                    // we copy the second channel
                    out_buffer_ptr->write(out);    
                }
                
                // write data to I2S
                i2s_bytes_to_write = out_buffer_ptr->sizeBytes();
                i2s_bytes_written = 0;
                if (i2s_bytes_to_write>0 && out_buffer_ptr->data()!=nullptr){
                    i2s_write(port,(void*) out_buffer_ptr->data(), i2s_bytes_to_write, &i2s_bytes_written, portMAX_DELAY);
                }
            } else {
                // write data to I2S
                i2s_bytes_to_write = len;
                i2s_write(port,(void*) bytes, i2s_bytes_to_write, &i2s_bytes_written, portMAX_DELAY);
            }

            if (i2s_bytes_written!=i2s_bytes_to_write){
                ESP_LOGI(TAG, "Did not process all data  %d/%ld", len, i2s_bytes_written);
            }
            return i2s_bytes_written;
        }

        virtual void flush(){
            // buffer is full -> send the data to I2S
            if (buffer_ptr->isFull()){
                write(buffer_ptr->data(), buffer_ptr->availableBytes());
                buffer_ptr->reset();
            }
        }

        // data available in the i2s buffer
        virtual int available() {
            readI2S();
            int result = read_max_pos-read_pos;
            return result<0?0:result;
        }

        virtual int read() {
            int result = peek();
            if (result>0)
                read_pos++;
            return result;
        }

        virtual int peek() {
            int result = -1;
            readI2S();
            if (read_pos<read_max_pos){
                result = buffer_ptr->data()[read_pos];
            }
            return result;
        }

        // stop I2S
        esp_err_t stop(){
            return i2s_stop(port);
        }

        // start I2S after it has been stopped
        esp_err_t start() {
            return i2s_start(port);
        }

    protected:
        int channels;
        i2s_config_t i2s_config; 
        SingleBuffer<uint8_t> *buffer_ptr;
        int buffer_size_in_bytes = 0;

        SingleBuffer<int16_t> *out_buffer_ptr;
        int out_buffer_size = 0;

        i2s_port_t port;
        int read_pos = 0;
        size_t read_max_pos = 0;

        // reads from i2s if the buffer is empty
        void readI2S(TickType_t ticks_to_wait=5) {
            if (read_max_pos==read_pos){
                i2s_read(port, buffer_ptr->data(), buffer_ptr->sizeBytes(), &read_max_pos, ticks_to_wait);
                read_pos = 0;        
            }
        }

        bool needsConversion() {
            if (i2s_config.mode & I2S_MODE_DAC_BUILT_IN) 
                return true;
            if (this->channels!=2){
                return true;
            }
            return false;
        }

        uint16_t scale(int16_t in){
            int16_t result = in;
            if (i2s_config.mode & I2S_MODE_DAC_BUILT_IN) {
                result = static_cast<float>(in) / 32767.0 * 127.0 + 127.0;
                result << 8;
           } 
            return result;
        }

        void logAudioInfo(){
            ESP_LOGI(TAG, "channels:  %d", this->channels);
            ESP_LOGI(TAG, "bits_per_sample:  %d", i2s_config.bits_per_sample);
            ESP_LOGI(TAG, "sample_rate:  %d", i2s_config.sample_rate);
        }

};

/**
 * @brief AnalogSampler 
 * 
 */

// forward declarations
class AnalogSampler; 
AnalogSampler *__sampler;
void IRAM_ATTR onSampleTimer();

class AnalogSampler {
    public:
        AnalogSampler(long sampleRate=DEFAULT_SAMLE_RATE, uint16_t pin = 25, size_t bufferSize=512){
            samplePin = pin;
            this->sampleRate = sampleRate;
            sampleBuffer = new DoubleBuffer<int16_t>(bufferSize);
            __sampler = this;
        }

        ~AnalogSampler(){
            delete sampleBuffer;
        }

        // initilaize timer
        virtual void begin() {
            setupSampler();
        }

        // reads the data from the buffer
        virtual void read(int16_t* &buffer, int &len){
            return sampleBuffer->read(buffer, len);
        }

        // Add a sample from the buffer
        virtual void processSample(){
            int16_t value = analogRead(samplePin);
            sampleBuffer->write(value);
        }      
        
    protected:
        long sampleRate;
        uint16_t samplePin = 25;
        hw_timer_t* timer = NULL;
        DoubleBuffer<int16_t> *sampleBuffer;


        virtual void setupSampler(){
            setupADC();
            setupTimer();
        }

        virtual void setupADC() {
            analogReadResolution(12);
            analogSetCycles(1);
            analogSetSamples(1);
            analogSetPinAttenuation(samplePin, ADC_0db); //     ADC_0db,ADC_2_5db,ADC_6db,ADC_11db

            adcAttachPin(samplePin);
            adcStart(samplePin);
        }
        
        virtual void setupTimer() {
            // Base timer triggered at 80MHZ 
            int div = 80000000l / sampleRate; 
            timer = timerBegin(0, div, true);
            timerAttachInterrupt(timer, &onSampleTimer, true);
            timerAlarmWrite(timer, 1, true);
            timerAlarmEnable(timer); 
        }

};

void IRAM_ATTR onSampleTimer() {
    __sampler->processSample();
}

/**
 * @brief Represents the content of a URL as Stream. We use the ESP32 ESP HTTP Client
 * API
 * 
 */
class UrlStream : public Stream {
    public:
        UrlStream(int readBufferSize=512){
            read_buffer = new uint8_t[readBufferSize];
        }

        ~UrlStream(){
            delete[] read_buffer;
            esp_http_client_close(client);
            esp_http_client_cleanup(client);
        }

        int begin(const char* url) {
            int result = -1;
            config.url = url;
            config.method = HTTP_METHOD_GET;
            AudioLogger.info("UrlStream.begin ",url);

            // cleanup last begin if necessary
            if (client==nullptr){
                client = esp_http_client_init(&config);
            } else {
                esp_http_client_set_url(client,url);
            }

            client = esp_http_client_init(&config);
            if (client==nullptr){
                AudioLogger.error("esp_http_client_init failed");
                return -1;
            }

            int write_buffer_len = 0;
            result = esp_http_client_open(client, write_buffer_len);
            if (result != ESP_OK) {
                AudioLogger.error("http_client_open failed");
                return result;
            }
            size = esp_http_client_fetch_headers(client);
            if (size<=0) {
                AudioLogger.error("esp_http_client_fetch_headers failed");
                return result;
            }

            ESP_LOGI(TAG, "Status = %d, content_length = %d",
                    esp_http_client_get_status_code(client),
                    esp_http_client_get_content_length(client));
        

            return result;
        }

        virtual int available() {
            return size - read_pos;
        }

        virtual size_t readBytes(uint8_t *buffer, size_t length){
            return esp_http_client_read(client, (char*)buffer, length);
        }

        virtual int read() {
            fillBuffer();
            return isEOS() ? -1 : read_buffer[read_pos++];
        }

        virtual int peek() {
            fillBuffer();
            return isEOS() ? -1 : read_buffer[read_pos];
        }

        virtual void flush(){
        }

        size_t write(uint8_t) {
            AudioLogger.error("UrlStream write - not supported");
        }


    protected:
        esp_http_client_handle_t client;
        esp_http_client_config_t config;
        long size;
        // buffered read
        uint8_t *read_buffer;
        uint16_t read_buffer_size;
        uint16_t read_pos;
        uint16_t read_size;


        inline void fillBuffer() {
            if (isEOS()){
                read_size = readBytes(read_buffer,read_buffer_size);
                read_pos = 0;
            }
        }

        inline bool isEOS() {
            return read_pos>=read_size;
        }

};

#endif
