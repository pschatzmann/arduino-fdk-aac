/**
 * Transport type identifiers.
 */
typedef enum {
  TT_UNKNOWN = -1, /**< Unknown format.            */
  TT_MP4_RAW = 0,  /**< "as is" access units (packet based since there is
                      obviously no sync layer) */
  TT_MP4_ADIF = 1, /**< ADIF bitstream format.     */
  TT_MP4_ADTS = 2, /**< ADTS bitstream format.     */

  TT_MP4_LATM_MCP1 = 6, /**< Audio Mux Elements with muxConfigPresent = 1 */
  TT_MP4_LATM_MCP0 = 7, /**< Audio Mux Elements with muxConfigPresent = 0, out
                           of band StreamMuxConfig */

  TT_MP4_LOAS = 10, /**< Audio Sync Stream.         */

  TT_DRM = 12 /**< Digital Radio Mondial (DRM30/DRM+) bitstream format. */

} TRANSPORT_TYPE;