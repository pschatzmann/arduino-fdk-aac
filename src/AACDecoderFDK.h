#pragma once

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include "fdk_log.h"
#include "libAACdec/aacdecoder_lib.h"

namespace aac_fdk {

typedef void (*AACInfoCallbackFDK)(CStreamInfo &info);
typedef void (*AACDataCallbackFDK)(CStreamInfo &info,INT_PCM *pcm_data, size_t len);

/**
 * @brief Audio Decoder which decodes AAC into a PCM stream
 * 
 */
class AACDecoderFDK  {
    public:
		/**
		 * @brief Construct a new AACDecoderFDK object
		 * 
		 * @param output_buffer_size 
		 */
        AACDecoderFDK(int output_buffer_size=2048){
        	LOG(Debug,__FUNCTION__);
            this->output_buffer_size = output_buffer_size;
            this->output_buffer = new INT_PCM[output_buffer_size];
		}

		/**
		 * @brief Construct a new AACDecoderFDK object
		 * 
		 * @param dataCallback 
		 * @param infoCallback (default nullptr)
		 * @param output_buffer_size (default 2048)
		 */
        AACDecoderFDK(AACDataCallbackFDK dataCallback, AACInfoCallbackFDK infoCallback=nullptr, int output_buffer_size=2048){
            this->output_buffer_size = output_buffer_size;
            setDataCallback(dataCallback);
            setInfoCallback(infoCallback);
        }

#ifdef ARDUINO

        AACDecoderFDK(Stream &out_stream, int output_buffer_size=2048){
        	LOG(Debug,__FUNCTION__);
            this->output_buffer_size = output_buffer_size;
            this->output_buffer = new INT_PCM[output_buffer_size];
			setStream(out_stream)
        }

		/// Defines the output stream
		void setStream(Stream &out_stream){
            this->out = &out_stream;
		}

#endif
        ~AACDecoderFDK(){
            end();
        }


        void setInfoCallback(AACInfoCallbackFDK cb){
            this->infoCallback = cb;
        }

        void setDataCallback(AACDataCallbackFDK cb){
            this->pwmCallback = cb;
        }

        void begin(){
			begin(TT_UNKNOWN, 1);
		}

        // opens the decoder
        void begin(TRANSPORT_TYPE transportType, UINT nrOfLayers){
			LOG(Debug,__FUNCTION__);
			int error;
            aacDecoderInfo = aacDecoder_Open(transportType, nrOfLayers);
			if (aacDecoderInfo==NULL){
				LOG(Error,"aacDecoder_Open -> Error");
				return;
			}
			is_open = true;
            return;
        }

        /**
         * @brief Explicitly configure the decoder by passing a raw AudioSpecificConfig (ASC) or a StreamMuxConfig
         * (SMC), contained in a binary buffer. This is required for MPEG-4 and Raw Packets file format bitstreams
         * as well as for LATM bitstreams with no in-band SMC. If the transport format is LATM with or without
         * LOAS, configuration is assumed to be an SMC, for all other file formats an ASC.
         * 
         **/
        AAC_DECODER_ERROR configure(uint8_t *conf, const uint32_t &length) {
            return aacDecoder_ConfigRaw (aacDecoderInfo, &conf, &length );
        }

        // write AAC data to be converted to PCM data
      	virtual size_t write(const void *in_ptr, size_t in_size) {
			LOG(Debug,"write %d bytes", in_size);
			size_t result = 0;
			if (aacDecoderInfo!=nullptr) {
				uint32_t bytesValid = 0;
				AAC_DECODER_ERROR error = aacDecoder_Fill(aacDecoderInfo, (UCHAR **)&in_ptr, (const UINT*)&in_size, &bytesValid); 
				if (error == AAC_DEC_OK) {
					int flags = 0;
					error = aacDecoder_DecodeFrame(aacDecoderInfo, output_buffer, output_buffer_size, flags); 
					// write pcm to output stream
					if (error == AAC_DEC_OK){
						provideResult(output_buffer, output_buffer_size);
					}
					// if not all bytes were used we process them now
					if (bytesValid<in_size){
						const uint8_t *start = static_cast<const uint8_t*>(in_ptr)+bytesValid;
						uint32_t act_len = in_size-bytesValid;
						aacDecoder_Fill(aacDecoderInfo, (UCHAR**) &start, &act_len, &bytesValid);
					}
				}
			}
            return result;
        }

        // provides detailed information about the stream
        CStreamInfo &audioInfo(){
            return *aacDecoder_GetStreamInfo(aacDecoderInfo);
        }

        // release the resources
        void end(){
	 		LOG(Debug,__FUNCTION__);
            if (aacDecoderInfo!=nullptr){
                aacDecoder_Close(aacDecoderInfo); 
                aacDecoderInfo = nullptr;
            }
            if (output_buffer!=nullptr){
                delete[] output_buffer;
                output_buffer = nullptr;
            }
			is_open = false;
        }

       virtual operator boolean() {
		   return is_open;
	   }

    protected:
		bool is_cleanup_stream = false;
        HANDLE_AACDECODER aacDecoderInfo;
        int output_buffer_size = 0;
        INT_PCM* output_buffer = nullptr;
		bool is_open = false;
		CStreamInfo aacFrameInfo;
        AACDataCallbackFDK pwmCallback = nullptr;
        AACInfoCallbackFDK infoCallback = nullptr;

#ifdef ARDUINO
        Stream *out = nullptr;
#endif


        /// return the result PWM data
        void provideResult(INT_PCM *data, size_t len){
            LOG(Debug, "provideResult: %d samples",len);
             if (len>0){
				 CStreamInfo info = audioInfo();
            	// provide result
                if(pwmCallback!=nullptr){
                    // output via callback
                    pwmCallback(info, data,len);
                } else {
                    // output to stream
                    if (info.aacSampleRate!=aacFrameInfo.aacSampleRate && infoCallback!=nullptr){
                        infoCallback(info);
                    }
#ifdef ARDUINO
                    out->write((uint8_t*)pwm_buffer, data, len*sizeof(INT_PCM));
#endif
                }
                aacFrameInfo = info;
            }
        }            

};

}
