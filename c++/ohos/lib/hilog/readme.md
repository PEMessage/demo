

```c
.
├── innerkits
│   ├── BUILD.gn
│   ├── include
│   │   ├── hilog
│   │   │   ├── log.h // include both log_c.h and log_cpp.h,
│   │   │   ├── log_c.h // HiLogPrint and HILOG_* macro, wrapper of HiLogPrint(hilog_printf.cpp)
│   │   │   └── log_cpp.h // HiLog class and HiLog* macro, same as HILOG_*
│   │   ├── hilog_base
│   │   │   └── log_base.h // HiLogBasePrint and HILOG_BASE_*, HiLogBasePrint are socket method
│   │   │                  
│   │   │
│   │   └── hilog_trace.h // allow register hook by HiLogRegisterGetIdFun and HiLogUnregisterGetIdFun
│   └── libhilog.map
└── kits
    ├── include
    │   └── hilog
    │       └── log.h
    └── libhilog.ndk.json

```
```c
.
├── include
│   └── hilog_inner.h // HiLogPrintArgs ***!!! the key function !!!***
└── libhilog
    ├── base
    │   └── hilog_base.c // HiLogBasePrint implement, seem like using for client(socket)???
    │   
    └─ include
        ├── hilog_base.h //  message from libhilog to hilogd
        ├── hilog_cmd.h // hilog ioctl
        └── hilog_common.h // general config(buffer size) log


```
```c
./frameworks/libhilog -- directly print
├── hilog.cpp // HiLog::Info / HiLog::Error .... Implement, simple wrapper of ::HiLogPrintArgs
├── hilog_printf.cpp // Implement of !!! HiLogPrintArgs !!!
├── include
│   ├── hilog_base.h
│   ├── hilog_cmd.h
│   └── hilog_common.h
├── param
│   ├── include
│   │   └── properties.h
│   └── properties.cpp
├── utils
│   ├── include
│   │   ├── log_print.h
│   │   ├── log_timestamp.h
│   │   └── log_utils.h
│   ├── log_print.cpp
│   └── log_utils.cpp // a KVMap data struct, and Split
└── vsnprintf // used by HiLogPrintArgs
    ├── include
    │   ├── vsnprintf_s_p.h
    │   └── vsprintf_p.h
    ├── output_p.inl
    ├── vsnprintf_s_p.c
    └── vsprintf_p.c
```



1. NDK Print
    See: https://developer.huawei.com/consumer/cn/doc/harmonyos-guides/hilog-guidelines-ndk
```cpp
OH_LOG_INFO -> OH_LOG_Print -> HiLogPrintArgs(hilog_printf.cpp)
```

