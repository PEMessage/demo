# commit 589e2690996bd5cfd628df6d728d4bdfd76acbee
# Author: PEMessage <1165739182@qq.com>
# Date:   2025-01-23 19:30:50 +0800
#
#     c++/refbase: tweak demo for track all
#
#     Detail:
#         g++ main.cpp refbase.cpp -DDEBUG_REFBASE -DPRINT_TRACK_AT_ONCE -DTRACK_ALL -Og -g3
#
#     Debug macro level:
#         TRACK_ALL --> track all object(do not need to manual enable)
#         PRINT_TRACK_AT_ONCE --> once ref change, print it
#
# commit ba090e86ff815794701d2f6153844925e7744829
# Author: PEMessage <1165739182@qq.com>
# Date:   2025-01-23 19:20:45 +0800
#
#     c++/refbase: remove {public} in log
#
#     Detail:
#         cat refbase.cpp| sed 's@{public}@@g' > refbase2.cpp
#
# commit 631dfcf66a207e54b9e89caaf480c1ca87259ac1
# Author: PEMessage <1165739182@qq.com>
# Date:   2025-01-23 19:19:22 +0800
#
#     c++/refbase: make utils_log using printf
#
# commit e4918821ed40c9db4e922d75d57622121b1b897c
# Author: PEMessage <1165739182@qq.com>
# Date:   2025-01-23 18:42:10 +0800
#
#     c++: add refbase demo from OH5.0

 g++ main.cpp refbase.cpp -DDEBUG_REFBASE -DPRINT_TRACK_AT_ONCE -DTRACK_ALL -Og -g3

