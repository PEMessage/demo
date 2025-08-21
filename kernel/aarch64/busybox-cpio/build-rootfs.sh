#!/bin/bash

# example : fakeroot -s rootfs.fakeroot bash build-rootfs.sh busybox-src/build/_install rootfs
# Credit: https://zhuanlan.zhihu.com/p/626683569
ROOTFS_OUT="$1"
ROOTFS_IMG_NAME="$(basename "$ROOTFS_OUT")"
shift

set -e
runcmd() {
    echo "Runing: $*"
    "$@"
}

cp_rootfs() {
    runcmd mkdir -p "$ROOTFS_OUT"
    for x in "$@" 
    do
        if [ -d "$x" ] ; then
            runcmd cp -raf $x/* "$ROOTFS_OUT"
        else
            echo "-- $x is not a dir, skip..."
        fi
    done
}

# Thanks to https://gist.github.com/m13253/e4c3e3a56a23623d2e7e6796678b9e58
do_init() {
    echo '#!/bin/busybox sh
# 1. prepare dir
busybox mkdir -p /etc/init.d /proc /root /sbin /sys /usr/bin /usr/sbin /dev
busybox mount -t proc proc /proc
busybox mount -t sysfs sys /sys

# 2. inittab(will used by linuxrc)
echo ::sysinit:/etc/init.d/rcS > /etc/inittab
# this is key !!!!
echo ::askfirst:-/bin/sh >> /etc/inittab
echo tty1::respawn:/sbin/getty 0 tty1 >> /etc/inittab

# 3. rcS(sysinit stage will call this)
echo #!/bin/sh > etc/init.d/rcS
echo export PATH=/sbin:/bin:/usr/bin:/usr/sbin >> etc/init.d/rcS
busybox chmod a+x etc/init.d/rcS

# 4. others
    # echo Login with root and no password. > /etc/issue
    # echo >> /etc/issue
    # echo root::0:0:root:/root:/bin/sh > /etc/passwd
busybox mdev -s
    # we already do it in host by make install
    # busybox --install
hostname localhost
ip link set lo up
exec /linuxrc' > init
runcmd chmod a+x init
}

prepare() {
    (
        runcmd cd "$ROOTFS_OUT"

        do_init
        echo "Now packing cpio..."
        find . | cpio -o -H newc | gzip > ../$ROOTFS_IMG_NAME.cpio.gz
    )
}

 cp_rootfs "$@" &&
 prepare
