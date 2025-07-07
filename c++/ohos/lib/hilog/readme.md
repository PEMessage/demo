

```c
.
├── innerkits
│   ├── BUILD.gn
│   ├── include
│   │   ├── hilog
│   │   │   ├── log.h // include both log_c.h and log_cpp.h
│   │   │   ├── log_c.h // HiLogPrint and HILOG_* macro
│   │   │   └── log_cpp.h // HiLog class and HiLog* macro, same as HILOG_*
│   │   ├── hilog_base
│   │   │   └── log_base.h // HiLogBasePrint and HILOG_BASE_*
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
    └── include
        ├── hilog_base.h //  message from libhilog to hilogd
        ├── hilog_cmd.h // hilog ioctl
        └── hilog_common.h // general config(buffer size) log


```

