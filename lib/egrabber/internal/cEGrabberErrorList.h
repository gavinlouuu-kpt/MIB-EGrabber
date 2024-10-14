/* Copyright Euresys 2021 */

#ifndef CEGRABBER_ERROR_LIST_HEADER_FILE
#define CEGRABBER_ERROR_LIST_HEADER_FILE

#ifdef __cplusplus
extern "C" {
namespace EURESYS_NAMESPACE {
namespace Internal {
namespace cEGrabber {
#endif

/** This enumeration defines error codes returned by the EGrabber library. */
enum CEGRABBER_ERROR_LIST {
    CEGRABBER_ERROR_SUCCESS           = 0,
    CEGRABBER_ERROR_BASE              = -2147483647,
    CEGRABBER_CTI_LOADING_ERROR       = CEGRABBER_ERROR_BASE + 1,
    CEGRABBER_MISSING_GENTL_SYMBOL    = CEGRABBER_ERROR_BASE + 2,
    CEGRABBER_UNEXPECTED_DATA_TYPE    = CEGRABBER_ERROR_BASE + 3,
    CEGRABBER_UNEXPECTED_DATA_SIZE    = CEGRABBER_ERROR_BASE + 4,
    CEGRABBER_CLIENT_ERROR            = CEGRABBER_ERROR_BASE + 5,
    CEGRABBER_GENTL_ERROR             = CEGRABBER_ERROR_BASE + 6,
    CEGRABBER_GENAPI_ERROR            = CEGRABBER_ERROR_BASE + 7,
    CEGRABBER_THREAD_ERROR            = CEGRABBER_ERROR_BASE + 8,
    CEGRABBER_INTERNAL_ERROR          = CEGRABBER_ERROR_BASE + 9,
    CEGRABBER_NOT_ALLOWED             = CEGRABBER_ERROR_BASE + 10,
    CEGRABBER_STD_RUNTIME_ERROR       = CEGRABBER_ERROR_BASE + 11,
    CEGRABBER_STD_LOGIC_ERROR         = CEGRABBER_ERROR_BASE + 12,
    CEGRABBER_STD_EXCEPTION           = CEGRABBER_ERROR_BASE + 13,
    CEGRABBER_CALLBACK_CRITICAL_ERROR = CEGRABBER_ERROR_BASE + 14,
};
typedef int32_t CEGRABBER_ERROR;

#ifdef __cplusplus
} // namespace cEGrabber
} // namespace Internal
} // namespace EURESYS_NAMESPACE
} // extern "C"
#endif

#endif
