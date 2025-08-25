#ifndef INCLUDEmisc_UNISTD_OHOS_MOCKunistd_ohos_mock_h_
#define INCLUDEmisc_UNISTD_OHOS_MOCKunistd_ohos_mock_h_


// See: https://github.com/openharmony/third_party_musl/blob/0af7cd556aa43f824f9b11fa30dcf8617cb7a7c3/docs/status.md?plain=1#L17
// OHOS add extra api, since we dont use namespace, just replace it with normal one


#ifndef getprocpid
#define getprocpid getpid
#endif
#ifndef getproctid
#define getproctid gettid
#endif



#endif  // INCLUDEmisc_UNISTD_OHOS_MOCKunistd_ohos_mock_h_
